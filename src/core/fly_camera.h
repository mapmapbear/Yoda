#pragma once
#include "rhi/d3d12/rhi_context_d3d12.h"
#include <glm/glm.hpp>

namespace Yoda {

class FlyCamera {
public:
  FlyCamera();

  FlyCamera(glm::vec3 const _eye, glm::vec3 const _center,
            SwapChainInfo info);
  void UpdateCamera(SwapChainInfo& info);

public:
  glm::vec3 eye;
  glm::vec3 center;
  glm::vec3 up;

  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 view_proj;

  glm::mat4 prev_view;
  glm::mat4 prev_proj;
  glm::mat4 prev_view_proj;


  // custom pass 
  glm::mat4 sky_transform;
};
} // namespace Yoda