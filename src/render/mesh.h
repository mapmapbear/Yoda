#pragma once
#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp\scene.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Yoda {
class Mesh {
public:
  friend class World;

public:
  std::vector<glm::vec3> positions_stream;
  std::vector<glm::vec3> normals_stream;
  std::vector<glm::vec3> tangents_stream;
  std::vector<glm::vec2> UVs_stream;
  std::vector<glm::vec4> colors_stream;
  std::vector<uint32_t> indices;

  std::string mesh_name;

public:
  static bool load_scene(std::string &path, World &world);

protected:
  // void process_node(aiNode *node, const aiScene *scene,
  //                   std::vector<Mesh> meshes);
  // Mesh process_mesh(aiMesh *mesh, const aiScene *scene);
};
} // namespace Yoda