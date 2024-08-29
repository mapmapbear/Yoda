#include "world.h"
#include "components/transform_components.h"
#include "core/logger.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "mesh.h"
#include "render/node.h"
#include "render/world.h"
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <string>
#include <ufbx.h>

#include <stb_image.h>
#include <unordered_map>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/version.h>

namespace Yoda {

bool parse_mesh2(ufbx_scene *scene, World &world) {
  std::vector<std::shared_ptr<Mesh>> &meshes = world.mesh_group;
  const aiScene *aiscene = aiImportFile(
      "module/sphere.fbx", aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_FixInfacingNormals);

  if (!aiscene || !aiscene->HasMeshes()) {
    LOG_ERROR("Unable to load data FBX !!!");
    exit(255);
  }
  for (uint32_t i = 0; i < aiscene->mNumMeshes; ++i) {
    auto mesh = aiscene->mMeshes[i];
    Mesh mesh_node;
    uint32_t numVertices = mesh->mNumVertices;
    if (mesh->mNumVertices > 0) {
      for (size_t j = 0; j < numVertices; ++j) {

        glm::vec3 normal = glm::vec3(1.0);
        glm::vec4 tangent = glm::vec4(1.0);
        glm::vec4 vertex_color = glm::vec4(1.0);
        if (mesh->HasNormals()) {
          normal = glm::vec3(mesh->mNormals[j].x, mesh->mNormals[j].y,
                             mesh->mNormals[j].z);
        }
        if (mesh->HasTangentsAndBitangents()) {
          tangent = glm::vec4(mesh->mTangents[j].x, mesh->mTangents[j].y,
                              mesh->mTangents[j].z, 1.0);
        }
        mesh_node.positions_stream.push_back(glm::vec3(
            mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z));
        mesh_node.UVs_stream.push_back(glm::vec2(mesh->mTextureCoords[0][j].x,
                                                 mesh->mTextureCoords[0][j].y));
        mesh_node.normals_stream.push_back(normal);
        mesh_node.tangents_stream.push_back(tangent);
        mesh_node.colors_stream.push_back(vertex_color);
      }
    }
    uint32_t numFaces = mesh->mNumFaces;
    uint32_t numIndices = numFaces * 3;
    mesh_node.indices.resize(numIndices);
    if (numFaces > 0) {
      for (size_t i = 0; i < numFaces; ++i) {
        mesh_node.indices[i * 3] = mesh->mFaces[i].mIndices[0];
        mesh_node.indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
        mesh_node.indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
      }
    }
    std::shared_ptr<Mesh> mesh_ptr = std::make_shared<Mesh>(mesh_node);
    world.mesh_map.insert({mesh_node.mesh_name, mesh_ptr});
    meshes.push_back(mesh_ptr);
  }
  return true;
}

bool parse_material(ufbx_scene *scene, World &world) {
  std::vector<std::shared_ptr<Material>> &mats = world.mat_group;
  std::unordered_map<std::string, std::shared_ptr<TextureData>> tex_cache;
  for (const ufbx_material *material : scene->materials) {
    Material mat_node;
    mat_node.base_color = glm::vec4(material->pbr.base_color.value_vec4.x,
                                    material->pbr.base_color.value_vec4.y,
                                    material->pbr.base_color.value_vec4.z,
                                    material->pbr.base_color.value_vec4.w);
    if (material->pbr.base_color.texture) {
      std::string albedo_name = std::string(
          material->pbr.base_color.texture->relative_filename.data,
          material->pbr.base_color.texture->relative_filename.length);
      if (!tex_cache.contains(albedo_name)) {
        int x, y, n;
        uint8_t *albedo_raw_data =
            stbi_load(albedo_name.c_str(), &x, &y, &n, 4);
        nvrhi::TextureDesc desc;
        desc.debugName = "Albedo Texture";
        desc.width = x;
        desc.height = y;
        desc.format = nvrhi::Format::RGBA8_UNORM;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        std::shared_ptr<TextureData> data_ptr = std::make_shared<TextureData>();
        data_ptr->raw_texture_data = (void *)albedo_raw_data;
        data_ptr->texture_desc = desc;
        data_ptr->texture_file_channel = n;
        mat_node.texture_dataes.emplace_back(data_ptr);
        tex_cache.insert({albedo_name, data_ptr});
      } else {
        mat_node.texture_dataes.emplace_back(tex_cache[albedo_name]);
      }
      mats.emplace_back(std::make_shared<Material>(mat_node));
    }
  }
  return true;
}

std::unordered_map<const ufbx_node *, std::shared_ptr<Node>> node_looked_map;
std::shared_ptr<Node> parse_node(const ufbx_node *parent_node) {
  Node render_node;
  const ufbx_node *fbx_node = parent_node;
  render_node.node_name =
      std::string(fbx_node->name.data, fbx_node->name.length);
  glm::mat4 translate_mat = glm::mat4(1.0);
  glm::mat4 rotation_mat = glm::mat4(1.0);
  glm::mat4 scale_mat = glm::mat4(1.0);

  render_node.m_local_transform.translation =
      glm::vec3(fbx_node->local_transform.translation.x,
                fbx_node->local_transform.translation.y,
                fbx_node->local_transform.translation.z);

  render_node.m_local_transform.rotation =
      glm::vec4(fbx_node->local_transform.rotation.x,
                fbx_node->local_transform.rotation.y,
                fbx_node->local_transform.rotation.z,
                fbx_node->local_transform.rotation.w);

  render_node.m_local_transform.scale = glm::vec3(
      fbx_node->local_transform.scale.x, fbx_node->local_transform.scale.y,
      fbx_node->local_transform.scale.z);

  translate_mat =
      glm::translate(translate_mat, render_node.m_local_transform.translation);
  scale_mat = glm::scale(scale_mat, render_node.m_local_transform.scale);
  rotation_mat =
      glm::mat4_cast(glm::quat(render_node.m_local_transform.rotation));
  render_node.m_transform = scale_mat * rotation_mat * translate_mat;
  if (parent_node->children.count > 0) {
    for (int i = 0; i < parent_node->children.count; ++i) {
      ufbx_node *son_node = parent_node->children[i];
      render_node.child_nodes.emplace_back(parse_node(son_node));
    }
  }
  return std::make_shared<Node>(render_node);
}

bool parse_scene_node(ufbx_scene *scene, World &world) {
  for (int node_i = 0; node_i < static_cast<int>(scene->nodes.count);
       node_i++) {
    const ufbx_node *fbx_node = scene->nodes[node_i];
    if (!fbx_node->is_root) {
      auto iter = node_looked_map.find(fbx_node->parent);
      if (iter != node_looked_map.end()) {
        node_looked_map.insert({fbx_node, nullptr});
      } else {
        std::shared_ptr<Node> current_node = parse_node(fbx_node);
        node_looked_map.insert({fbx_node, current_node});
        world.node_tree.emplace_back(current_node);
      }
    }
  }
  for (int node_i = 0; node_i < static_cast<int>(scene->nodes.count);
       node_i++) {
  }
  return true;
}

std::unordered_map<const ufbx_node *, std::shared_ptr<NewNode>>
    newnode_looked_map;
bool World::load_scene1(std::string &path, World &world) {
  ufbx_load_opts opts = {}; // Optional, pass NULL for defaults
  ufbx_error error; // Optional, pass NULL if you don't care about errors
  ufbx_scene *scene = ufbx_load_file(path.c_str(), &opts, &error);
  if (!scene) {
    LOG_ERROR("Failed to load: {}", error.description.data);
    exit(1);
  }
  for (int node_i = 0; node_i < static_cast<int>(scene->nodes.count);
       node_i++) {
    const ufbx_node *fbx_node = scene->nodes[node_i];

    if (fbx_node->is_root)
      continue;
    if (fbx_node->parent && !fbx_node->is_root) {

      auto entity = world.m_registry.create();
      TransformComp local_transform;
      local_transform.transform_mat = glm::mat4(1.0);
      local_transform.position =
          glm::vec3(fbx_node->local_transform.translation.x,
                    fbx_node->local_transform.translation.y,
                    fbx_node->local_transform.translation.z);
      local_transform.rotation =
          glm::vec4(fbx_node->local_transform.rotation.x,
                    fbx_node->local_transform.rotation.y,
                    fbx_node->local_transform.rotation.z,
                    fbx_node->local_transform.rotation.w);
      local_transform.scale = glm::vec3(fbx_node->local_transform.scale.x,
                                        fbx_node->local_transform.scale.y,
                                        fbx_node->local_transform.scale.z);
      world.m_registry.emplace<TransformComp>(entity, local_transform);
      std::shared_ptr<NewNode> current_node =
          std::make_shared<NewNode>(world, entity);
      if (newnode_looked_map.find(fbx_node) != newnode_looked_map.end()) {
        std::shared_ptr<NewNode> present_node = newnode_looked_map[fbx_node];
        current_node->parent = present_node;
        present_node->childs.emplace_back(current_node);
        newnode_looked_map.insert({fbx_node, current_node});
      } else {
        current_node->parent = nullptr;
        newnode_looked_map.insert({fbx_node, current_node});
      }
    }
  }
  return true;
}
bool World::load_scene2(std::string &path, World &world) {
  ufbx_load_opts opts = {}; // Optional, pass NULL for defaults
  ufbx_error error; // Optional, pass NULL if you don't care about errors
  ufbx_scene *scene = ufbx_load_file(path.c_str(), &opts, &error);
  if (!scene) {
    fprintf(stderr, "Failed to load: %s\n", error.description.data);
    exit(1);
  }
  parse_mesh2(scene, world);
  parse_material(scene, world);
  parse_scene_node(scene, world);
  ufbx_free_scene(scene);
  return true;
}
} // namespace Yoda
