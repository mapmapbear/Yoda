#pragma once
#include "nvrhi/nvrhi.h"
#include "render/node.h"
#include <memory>
#include <ufbx.h>
#include <unordered_map>
#include <vector>

#include "entt/entt.hpp"

namespace Yoda {
class Mesh;
class Material;
class Node;

class World {
public:
  World() { mesh_group = {}; }

public:
  static bool load_scene1(std::string &path, World &world);
  static bool load_scene2(std::string &path, World &world);
  // static bool parse_mesh(ufbx_scene *scene, World &world);
  // static bool parse_material(ufbx_scene *scene, World &world);

public:
  std::vector<std::shared_ptr<Mesh>> mesh_group;
  std::vector<std::shared_ptr<Material>> mat_group;
  std::vector<std::shared_ptr<Node>> node_tree;
  std::unordered_map<std::string, std::shared_ptr<Mesh>> mesh_map;
  std::shared_ptr<Node> active_node;
  entt::registry m_registry;
};
} // namespace Yoda