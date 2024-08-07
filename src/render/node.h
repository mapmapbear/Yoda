#pragma once
#include "entt/entity/fwd.hpp"
#include "render/material.h"
#include "render/mesh.h"
#include <memory>
#include <string>
#include <vector>

// entt
#include <entt/entt.hpp>

namespace Yoda {
class Mesh;
class Material;
class World;
struct local_transform {
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

class NewNode {
public:
  NewNode() = default;
  NewNode(World &world, entt::entity entity)
      : m_entity_handle(entity), m_world(world) {}
  ~NewNode() = default;
  bool operator==(NewNode &Other) {
    return m_entity_handle == Other.m_entity_handle;
  }

  // template <class ComponentType, class... Args>
  // ComponentType &add_component(Args &&...args) {
  //   assert(!has_component<ComponentType>());
  //   auto &comp = m_world->m_Registry.emplace<ComponentType>(
  //       m_entity_handle, std::forward<Args>(args)...);
  //   m_world->OnComponentAdd<ComponentType>(*this, comp);
  //   return comp;
  // }
  
public:
  std::shared_ptr<NewNode> parent;
  std::vector<std::shared_ptr<NewNode>> childs;
  World& m_world;
  entt::entity m_entity_handle = entt::null;
};
} // namespace Yoda