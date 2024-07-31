#pragma once
#include "render/material.h"
#include "render/mesh.h"
#include <memory>
#include <string>
#include <vector>

namespace Yoda {
class Mesh;
class Material;
struct local_transform
{
  glm::vec3 translation;
  glm::vec3 scale;
  glm::vec4 rotation;
};
class Node {
public:
  Node(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat)
      : m_mesh(mesh), m_material(mat), child_nodes({}) {
    m_transform = glm::mat4(1.0);
  };
  Node() {}

public:
  std::string node_name;
  std::shared_ptr<Mesh> m_mesh;
  std::shared_ptr<Material> m_material;
  glm::mat4 m_transform;
  local_transform m_local_transform;
  std::vector<std::shared_ptr<Node>> child_nodes;
};
} // namespace Yoda