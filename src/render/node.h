#pragma once
#include "render/material.h"
#include "render/mesh.h"
#include <memory>
#include <string>

namespace Yoda {
class Mesh;
class Material;
class Node {
public:
  Node(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat)
      : m_mesh(mesh), m_material(mat) {
    m_transform = glm::mat4(1.0);
  };
  Node() {}

public:
  std::string node_name;
  std::shared_ptr<Mesh> m_mesh;
  std::shared_ptr<Material> m_material;
  glm::mat4 m_transform;
};
} // namespace Yoda