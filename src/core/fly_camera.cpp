#include "core/fly_camera.h"
#include "glm/gtc/matrix_transform.hpp"

namespace Yoda {
float CalculateHaltonNumber(uint32_t index, uint32_t base) {
  float f = 1.0f, result = 0.0f;
  for (uint32_t i = index; i > 0;) {
    f /= base;
    result = result + f * (i % base);
    i = (uint32_t)(i / (float)base);
  }
  return result;
}

FlyCamera::FlyCamera()
    : eye(glm::vec3(0.0f)), center(glm::vec3(0.0f)), up(glm::vec3(0.0f)) {
  glm::mat4 one = glm::mat4(1.0f);
  view = one;
  proj = one;
  view_proj = one;
  prev_view = one;
  prev_proj = one;
  prev_view_proj = one;
}

FlyCamera::FlyCamera(glm::vec3 const _eye, glm::vec3 const _center,
                     SwapChainInfo info) {
  float const aspect_ratio = (float)info.width / info.height;

  eye = _eye;
  center = _center;
  up = glm::vec3(0.0f, 1.0f, 0.0f);

  view = glm::lookAt(eye, center, up);

  proj = glm::perspective(0.6f, aspect_ratio, 1e-1f, 1e4f);

  view_proj = proj * view;

  prev_view = view;
  prev_proj = proj;
  prev_view_proj = view_proj;
}

void FlyCamera::UpdateCamera(SwapChainInfo &info) {
  // Update camera history
  view = glm::lookAt(eye, center, up);
  prev_view = view;
  prev_proj = proj;

  // TODO: animate the camera... (gboisse)

  // Update projection aspect ratio
  float const aspect_ratio = (float)info.width / info.height;

  proj = glm::perspective(0.6f, aspect_ratio, 1e-1f, 1e4f);

  // Update projection jitter for anti-aliasing
  static uint32_t jitter_index;

  jitter_index = (jitter_index + 1) & 15; // 16 samples TAA

  float const jitter_x =
      (2.0f * CalculateHaltonNumber(jitter_index + 1, 2) - 1.0f) / info.width;
  float const jitter_y =
      (2.0f * CalculateHaltonNumber(jitter_index + 1, 3) - 1.0f) / info.height;

  proj[2][0] = jitter_x;
  proj[2][1] = jitter_y;
  prev_proj[2][0] = jitter_x; // patch previous projection matrix so subpixel
                              // jitter doesn't generate velocity values
  prev_proj[2][1] = jitter_y;

  view_proj = proj * view; 
  prev_view_proj = prev_proj * prev_view;
}

} // namespace Yoda