#include "render/mesh.h"
#include "core/logger.h"
#include "nvrhi/nvrhi.h"
#include "render/world.h"
#include <memory>
#include <ufbx.h>
#include <vector>

namespace Yoda {

static Mesh process_mesh(aiMesh *mesh, const aiScene *scene) {
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
  return mesh_node;
}

static void process_node(aiNode *node, const aiScene *scene, World &world) {
  for (UINT i = 0; i < node->mNumMeshes; i++) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    std::vector<std::shared_ptr<Mesh>> &meshes = world.mesh_group;
    //meshes.push_back(process_mesh(mesh, scene));
  }

  for (UINT i = 0; i < node->mNumChildren; i++) {
    process_node(node->mChildren[i], scene, world);
  }
}

// bool Mesh::load_scene(std::string &path, World &world) {
//   Assimp::Importer importer;

//   const aiScene *pScene = importer.ReadFile(
//       path.c_str(), aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);
//   if (pScene == nullptr) {
//     LOG_ERROR("Failed to load mesh file scene by Assimp !!");
//     return false;
//   }

//   process_node(pScene->mRootNode, pScene, world);
//   return true;
// }

bool Mesh::load_scene(std::string &path, World &world) {
  // ufbx_load_opts opts = {}; // Optional, pass NULL for defaults
  // ufbx_error error; // Optional, pass NULL if you don't care about errors
  // ufbx_scene *scene = ufbx_load_file(path.c_str(), &opts, &error);
  // if (!scene) {
  //   fprintf(stderr, "Failed to load: %s\n", error.description.data);
  //   exit(1);
  // }
  // std::vector<Mesh> &meshes = world.mesh_group;
  // // Let's just list all objects within the scene for example:
  // for (ufbx_node *node : scene->nodes) {
  //   if (node->is_root)
  //     continue;

  //   printf("Object: %s\n", node->name.data);
  //   if (node->mesh) {
  //     ufbx_mesh *mesh = node->mesh;
  //     Mesh mesh_node;
  //     for (const ufbx_mesh_part &fbx_mesh_part : mesh->material_parts) {
  //       uint32_t num_indices = fbx_mesh_part.num_triangles * 3;
  //       std::vector<uint32_t> tri_indices(mesh->max_face_triangles * 3);
  //       for (uint32_t face_index : fbx_mesh_part.face_indices) {
  //         ufbx_face face = mesh->faces[face_index];

  //         uint32_t num_triangles = ufbx_triangulate_face(
  //             tri_indices.data(), tri_indices.size(), mesh, face);
  //         for (size_t i = 0; i < num_triangles * 3; i++) {
  //           const uint32_t index = tri_indices[i];
  //           const uint32_t vertex = mesh->vertex_indices[index];

  //           ufbx_vec3 u_position = mesh->vertex_position[index];
  //           ufbx_vec3 u_normal = mesh->vertex_normal[index];
  //           ufbx_vec2 u_uv = mesh->vertex_uv[index];
  //           glm::vec4 color = glm::vec4(1.0f);
  //           glm::vec4 tangent = glm::vec4(1.0f);
  //           if (mesh->vertex_tangent.exists) {
  //             tangent = glm::vec4(mesh->vertex_tangent[index].x,
  //                                 mesh->vertex_tangent[index].y,
  //                                 mesh->vertex_tangent[index].z, 1.0f);
  //           }

  //           if (mesh->vertex_color.exists) {
  //             color = glm::vec4(
  //                 mesh->vertex_color[index].x, mesh->vertex_color[index].y,
  //                 mesh->vertex_color[index].z, mesh->vertex_color[index].w);
  //           }

  //           glm::vec3 position =
  //               glm::vec3(u_position.x, u_position.y, u_position.z);
  //           glm::vec3 normal = glm::vec3(u_normal.x, u_normal.y, u_normal.z);
  //           glm::vec2 uv = glm::vec2(u_uv.x, u_uv.y);

  //           mesh_node.positions_stream.emplace_back(position);
  //           mesh_node.normals_stream.emplace_back(normal);
  //           mesh_node.tangents_stream.emplace_back(tangent);
  //           mesh_node.UVs_stream.emplace_back(uv);
  //           mesh_node.colors_stream.emplace_back(color);
  //         }
  //       }
  //       mesh_node.indices.resize(num_indices);
  //       std::vector<ufbx_vertex_stream> streams;
  //       if (!mesh_node.positions_stream.empty()) {
  //         streams.push_back({mesh_node.positions_stream.data(),
  //                            mesh_node.positions_stream.size(),
  //                            sizeof(mesh_node.positions_stream[0])});
  //       }
  //       if (!mesh_node.normals_stream.empty()) {
  //         streams.push_back({mesh_node.normals_stream.data(),
  //                            mesh_node.normals_stream.size(),
  //                            sizeof(mesh_node.normals_stream[0])});
  //       }
  //       if (!mesh_node.UVs_stream.empty()) {
  //         streams.push_back({mesh_node.UVs_stream.data(),
  //                            mesh_node.UVs_stream.size(),
  //                            sizeof(mesh_node.UVs_stream[0])});
  //       }
  //       if (!mesh_node.colors_stream.empty()) {
  //         streams.push_back({mesh_node.colors_stream.data(),
  //                            mesh_node.colors_stream.size(),
  //                            sizeof(mesh_node.colors_stream[0])});
  //       }
  //       if (!mesh_node.tangents_stream.empty()) {
  //         streams.push_back({mesh_node.tangents_stream.data(),
  //                            mesh_node.tangents_stream.size(),
  //                            sizeof(mesh_node.tangents_stream[0])});
  //       }
  //       const size_t num_vertices = ufbx_generate_indices(
  //           streams.data(), streams.size(), mesh_node.indices.data(),
  //           mesh_node.indices.size(), nullptr, nullptr);
  //       meshes.emplace_back(mesh_node);
  //     }
  //   }
  // }
  // std::vector<Material> &mats = world.mat_group;
  // std::unordered_map<std::string, nvrhi::TextureHandle> tex_cache;
  // for (const ufbx_material *material : scene->materials) {
  //   Material mat_node;
  //   mat_node.base_color = glm::vec4(material->pbr.base_color.value_vec4.x,
  //                                   material->pbr.base_color.value_vec4.y,
  //                                   material->pbr.base_color.value_vec4.z,
  //                                   material->pbr.base_color.value_vec4.w);

    
  // }

  // ufbx_free_scene(scene);
  return true;
}
} // namespace Yoda