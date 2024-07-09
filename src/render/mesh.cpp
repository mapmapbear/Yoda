#include "render/mesh.h"
#include "core/logger.h"
#include "render/world.h"
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
    std::vector<Mesh> &meshes = world.mesh_group;
    meshes.push_back(process_mesh(mesh, scene));
  }

  for (UINT i = 0; i < node->mNumChildren; i++) {
    process_node(node->mChildren[i], scene, world);
  }
}

bool Mesh::load_scene(std::string &path, World &world) {
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
} // namespace Yoda