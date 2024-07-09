#pragma once
#include "render/mesh.h"
#include <vector>

namespace Yoda {
class World {
public:
  World() { mesh_group = {}; }

public:
  std::vector<Mesh> mesh_group;
};
} // namespace Yoda