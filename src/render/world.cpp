#include "world.h"
#include "core/logger.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "mesh.h"
#include "render/node.h"
#include "render/world.h"
#include <memory>
#include <string>
#include <ufbx.h>

#include <stb_image.h>

namespace Yoda {
static std::shared_ptr<Mesh> process_mesh(aiMesh *mesh, const aiScene *scene) {
  Mesh mesh_node;
  for (UINT i = 0; i < mesh->mNumVertices; i++) {
    glm::vec3 position;
    position.x = mesh->mVertices[i].x;
    position.y = mesh->mVertices[i].y;
    position.z = mesh->mVertices[i].z;
    glm::vec2 uv = glm::vec2(0.0f);
    if (mesh->mTextureCoords[0]) {
      uv.x = (float)mesh->mTextureCoords[0][i].x;
      uv.y = (float)mesh->mTextureCoords[0][i].y;
    }
    glm::vec3 normal = glm::vec3(0.0);
    glm::vec4 tangent = glm::vec4(0.0);
    if (mesh->mNormals) {
      normal.x = mesh->mNormals[i].x;
      normal.y = mesh->mNormals[i].y;
      normal.z = mesh->mNormals[i].z;
    }

    if (mesh->mTangents) {
      tangent.x = mesh->mTangents[i].x;
      tangent.y = mesh->mTangents[i].y;
      tangent.z = mesh->mTangents[i].z;
      tangent.w = 1.0;
    }

    glm::vec4 color = glm::vec4(0.0);
    if (mesh->mColors[0]) {
      color.x = mesh->mColors[0][i].r;
      color.y = mesh->mColors[0][i].g;
      color.z = mesh->mColors[0][i].b;
      color.w = mesh->mColors[0][i].a;
    }

    mesh_node.positions_stream.emplace_back(position);
    mesh_node.normals_stream.emplace_back(normal);
    mesh_node.tangents_stream.emplace_back(tangent);
    mesh_node.UVs_stream.emplace_back(uv);
    mesh_node.colors_stream.emplace_back(color);
  }
  for (UINT i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];

    for (UINT j = 0; j < face.mNumIndices; j++)
      mesh_node.indices.push_back(face.mIndices[j]);
  }
  return std::make_shared<Mesh>(mesh_node);
}

glm::mat4 AssimpMatrixToGLMMatrix(const aiMatrix4x4 &from) {
  glm::mat4 to;

  // 转置复制
  to[0][0] = from.a1;
  to[1][0] = from.a2;
  to[2][0] = from.a3;
  to[3][0] = from.a4;
  to[0][1] = from.b1;
  to[1][1] = from.b2;
  to[2][1] = from.b3;
  to[3][1] = from.b4;
  to[0][2] = from.c1;
  to[1][2] = from.c2;
  to[2][2] = from.c3;
  to[3][2] = from.c4;
  to[0][3] = from.d1;
  to[1][3] = from.d2;
  to[2][3] = from.d3;
  to[3][3] = from.d4;

  return to;
}

static void process_material(const aiMesh *mesh, const aiScene *scene) {
  Material mat_node;
  aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
  for (UINT i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); i++) {
    aiString baseColor_tex;
    mat->GetTexture(aiTextureType_DIFFUSE, i, &baseColor_tex);
    int x = 1;
  }
}

static void process_node(aiNode *node, const aiScene *scene, World &world) {
  for (uint32_t i = 0; i < node->mNumMeshes; i++) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    std::shared_ptr<Mesh> active_mesh = process_mesh(mesh, scene);
    process_material(mesh, scene);
    std::shared_ptr<Material> test_mat = std::make_shared<Material>(Material());
    std::shared_ptr<Node> active_node =
        std::make_shared<Node>(active_mesh, test_mat);
    active_node->m_transform = AssimpMatrixToGLMMatrix(node->mTransformation);
    active_node->node_name = node->mName.C_Str();
    world.node_tree.emplace_back(active_node);
  }

  for (uint32_t i = 0; i < node->mNumChildren; i++) {
    process_node(node->mChildren[i], scene, world);
  }
}

bool World::load_scene1(std::string &path, World &world) {
  Assimp::Importer importer;

  const aiScene *pScene = importer.ReadFile(
      path.c_str(), aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);
  if (pScene == nullptr) {
    LOG_ERROR("Failed to load mesh file scene by Assimp !!");
    return false;
  }

  process_node(pScene->mRootNode, pScene, world);
  return true;
}

bool parse_mesh(ufbx_scene *scene, World &world) {
  std::vector<std::shared_ptr<Mesh>> &meshes = world.mesh_group;
  // Let's just list all objects within the scene for example:
  for (ufbx_node *node : scene->nodes) {
    if (node->is_root)
      continue;

    printf("Object: %s\n", node->name.data);
    if (node->mesh) {
      ufbx_mesh *mesh = node->mesh;
      Mesh mesh_node;
      if (mesh)
        mesh_node.mesh_name = std::string(node->name.data, node->name.length);
      for (const ufbx_mesh_part &fbx_mesh_part : mesh->material_parts) {
        uint32_t num_indices = fbx_mesh_part.num_triangles * 3;
        std::vector<uint32_t> tri_indices(mesh->max_face_triangles * 3);
        for (uint32_t face_index : fbx_mesh_part.face_indices) {
          ufbx_face face = mesh->faces[face_index];

          uint32_t num_triangles = ufbx_triangulate_face(
              tri_indices.data(), tri_indices.size(), mesh, face);
          for (size_t i = 0; i < num_triangles * 3; i++) {
            const uint32_t index = tri_indices[i];
            const uint32_t vertex = mesh->vertex_indices[index];

            ufbx_vec3 u_position = mesh->vertex_position[index];
            ufbx_vec3 u_normal = mesh->vertex_normal[index];
            ufbx_vec2 u_uv = mesh->vertex_uv[index];
            glm::vec4 color = glm::vec4(1.0f);
            glm::vec4 tangent = glm::vec4(1.0f);
            if (mesh->vertex_tangent.exists) {
              tangent = glm::vec4(mesh->vertex_tangent[index].x,
                                  mesh->vertex_tangent[index].y,
                                  mesh->vertex_tangent[index].z, 1.0f);
            }

            if (mesh->vertex_color.exists) {
              color = glm::vec4(
                  mesh->vertex_color[index].x, mesh->vertex_color[index].y,
                  mesh->vertex_color[index].z, mesh->vertex_color[index].w);
            }

            glm::vec3 position =
                glm::vec3(u_position.x, u_position.y, u_position.z);
            glm::vec3 normal = glm::vec3(u_normal.x, u_normal.y, u_normal.z);
            glm::vec2 uv = glm::vec2(u_uv.x, u_uv.y);

            mesh_node.positions_stream.emplace_back(position);
            mesh_node.normals_stream.emplace_back(normal);
            mesh_node.tangents_stream.emplace_back(tangent);
            mesh_node.UVs_stream.emplace_back(uv);
            mesh_node.colors_stream.emplace_back(color);
          }
        }
        mesh_node.indices.resize(num_indices);
        std::vector<ufbx_vertex_stream> streams;
        if (!mesh_node.positions_stream.empty()) {
          streams.push_back({mesh_node.positions_stream.data(),
                             mesh_node.positions_stream.size(),
                             sizeof(mesh_node.positions_stream[0])});
        }
        if (!mesh_node.normals_stream.empty()) {
          streams.push_back({mesh_node.normals_stream.data(),
                             mesh_node.normals_stream.size(),
                             sizeof(mesh_node.normals_stream[0])});
        }
        if (!mesh_node.UVs_stream.empty()) {
          streams.push_back({mesh_node.UVs_stream.data(),
                             mesh_node.UVs_stream.size(),
                             sizeof(mesh_node.UVs_stream[0])});
        }
        if (!mesh_node.colors_stream.empty()) {
          streams.push_back({mesh_node.colors_stream.data(),
                             mesh_node.colors_stream.size(),
                             sizeof(mesh_node.colors_stream[0])});
        }
        if (!mesh_node.tangents_stream.empty()) {
          streams.push_back({mesh_node.tangents_stream.data(),
                             mesh_node.tangents_stream.size(),
                             sizeof(mesh_node.tangents_stream[0])});
        }
        const size_t num_vertices = ufbx_generate_indices(
            streams.data(), streams.size(), mesh_node.indices.data(),
            mesh_node.indices.size(), nullptr, nullptr);
        std::shared_ptr<Mesh> mesh_ptr = std::make_shared<Mesh>(mesh_node);
        world.mesh_map.insert({mesh_node.mesh_name, mesh_ptr});
        meshes.emplace_back(mesh_ptr);
      }
    }
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

bool parse_node(ufbx_scene *scene, World &world) {
  for (int node_i = 0; node_i < static_cast<int>(scene->nodes.count);
       node_i++) {
    Node render_node;
    const ufbx_node *fbx_node = scene->nodes[node_i];
    render_node.node_name =
        std::string(fbx_node->name.data, fbx_node->name.length);
    glm::mat4 translate_mat = glm::mat4(1.0);
    glm::mat4 rotation_mat = glm::mat4(1.0);
    glm::mat4 scale_mat = glm::mat4(1.0);

    translate_mat = glm::translate(
        translate_mat, glm::vec3(fbx_node->local_transform.translation.x,
                                 fbx_node->local_transform.translation.y,
                                 fbx_node->local_transform.translation.z));
    scale_mat =
        glm::scale(scale_mat, glm::vec3(fbx_node->local_transform.scale.x,
                                        fbx_node->local_transform.scale.y,
                                        fbx_node->local_transform.scale.z));
    rotation_mat =
        glm::mat4_cast(glm::quat(fbx_node->local_transform.rotation.x,
                                 fbx_node->local_transform.rotation.y,
                                 fbx_node->local_transform.rotation.z,
                                 fbx_node->local_transform.rotation.w));
    render_node.m_transform = scale_mat * rotation_mat * translate_mat;
    // One node, One mesh
    if (fbx_node->mesh) {
      render_node.m_mesh = world.mesh_map.at(
          std::string(fbx_node->name.data, fbx_node->name.length));
    }
    world.node_tree.emplace_back(std::make_shared<Node>(render_node));
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
  parse_mesh(scene, world);
  parse_material(scene, world);
  parse_node(scene, world);
  ufbx_free_scene(scene);
  return true;
}
} // namespace Yoda
