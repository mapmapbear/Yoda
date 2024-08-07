#pragma once
#include <glm/glm.hpp>

namespace Yoda {
class TransformComp {
public:
  TransformComp() = default;
  ~TransformComp() = default;

public:
  glm::mat4 transform_mat;
  glm::vec3 position;
  glm::vec4 rotation;
  glm::vec3 scale;
};
} // namespace Yoda