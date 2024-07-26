#include "application/sample_app.h"
#include "core/fly_camera.h"
#include "core/logger.h"
#include "core/shadercode.h"
#include "render/mesh.h"
#include "render/render_pass/gui_pass.h"
#include "render/render_pass/simple_pass.h"
#include "render/render_pass/sky_pass.h"
#include "render/world.h"
#include "rhi/d3d12/rhi_context_d3d12.h"

#include <fstream>
#include <imgui.h>
#include <memory>
#include <nvrhi/d3d12.h>
#include <nvrhi/utils.h>
#include <string>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Yoda {
SampleApp::SampleApp(const SampleAppConfig &config) {
  mShowUI = config.showUI;
  mVsyncOn = config.windowDesc.enableVSync;
  if (!config.headless) {
    auto windowDesc = config.windowDesc;
    // Create the window
    m_window = Window::create(windowDesc, this);
    // m_window->setWindowIcon(getRuntimeDirectory() /
    // "data/framework/nvidia.ico");
    m_render_context = std::make_shared<RHIContextD3D12>();
    m_render_context->initialize(m_window->getApiHandle());

    // std::string test_scene_path = "module/sphere_units.fbx";
    //  std::string test_scene_path = "module/crag.fbx";
    //   bool state = Mesh::load_scene(test_scene_path, scene_world);
    //   state = World::load_scene1(test_scene_path, scene_world);

    fcamera =
        FlyCamera{glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                  m_render_context->get_swapchain_info()};

    nvrhi::TextureDesc texture_desc;
    texture_desc.width = m_render_context->get_swapchain_info().width;
    texture_desc.height = m_render_context->get_swapchain_info().height;
    texture_desc.format = nvrhi::Format::RGBA8_UNORM;
    texture_desc.debugName = "Pass A Color Buffer";
    texture_desc.isRenderTarget = true;
    texture_desc.isUAV = false;
    texture_desc.initialState = nvrhi::ResourceStates::RenderTarget;
    texture_desc.keepInitialState = true;
    color_buffer = m_render_context->texture_create(texture_desc);

    nvrhi::TextureDesc depth_texture_desc = texture_desc;
    depth_texture_desc.format = nvrhi::Format::D32;
    depth_texture_desc.initialState = nvrhi::ResourceStates::DepthWrite;
    depth_texture_desc.clearValue = nvrhi::Color(0.0f);
    depth_texture_desc.debugName = "Pass A Depth Buffer";
    depth_buffer = m_render_context->texture_create(depth_texture_desc);

    passA = std::make_shared<SimplePass>(m_render_context);
    passB = std::make_shared<SkyPass>(m_render_context);
    passC = std::make_shared<GUIPass>(m_render_context);
	  passC->init();
  }
}

SampleApp::~SampleApp() {
  m_window.reset();
  pipeline = nullptr;
  vs_shader = nullptr;
  ps_shader = nullptr;

  input_layout = nullptr;
  vertex_buffer = nullptr;
  uv_buffer = nullptr;
  normal_buffer = nullptr;
  tangent_buffer = nullptr;
  binormal_buffer = nullptr;
  index_buffer = nullptr;
  constant_buffer = nullptr;
  binding_layout = nullptr;
  binding_set = nullptr;
  color_buffer = nullptr;
  depth_buffer = nullptr;
  test_framebuffer = nullptr;

  m_render_context.reset();
}
int SampleApp::run() {
  if (m_window) {
    m_window->msgLoop();
  } else {
    // TODO
  }

  return 0;
}

void SampleApp::resizeFrameBuffer(uint32_t width, uint32_t height) {
  if (m_window) {
    // If we have a window, resize it. This will result in a call
    // back to handleWindowSizeChange() which in turn will resize the frame
    // buffer.
    color_buffer = nullptr;
    depth_buffer = nullptr;
    test_framebuffer = nullptr;
    nvrhi::TextureDesc texture_desc;
    texture_desc.width = m_render_context->get_swapchain_info().width;
    texture_desc.height = m_render_context->get_swapchain_info().height;
    texture_desc.format = nvrhi::Format::RGBA8_UNORM;
    texture_desc.debugName = "Pass A Color Buffer";
    texture_desc.isRenderTarget = true;
    texture_desc.isUAV = false;
    texture_desc.initialState = nvrhi::ResourceStates::RenderTarget;
    texture_desc.keepInitialState = true;
    color_buffer = m_render_context->texture_create(texture_desc);

    nvrhi::TextureDesc depth_texture_desc = texture_desc;
    depth_texture_desc.format = nvrhi::Format::D32;
    depth_texture_desc.initialState = nvrhi::ResourceStates::DepthWrite;
    depth_texture_desc.clearValue = nvrhi::Color(1.0f);
    depth_texture_desc.debugName = "Pass A Depth Buffer";
    depth_buffer = m_render_context->texture_create(depth_texture_desc);

    nvrhi::DeviceHandle devicePtr = m_render_context->nvrhi_device;
    auto framebufferDesc = nvrhi::FramebufferDesc()
                               .addColorAttachment(color_buffer)
                               .setDepthAttachment(depth_buffer);
    test_framebuffer = devicePtr->createFramebuffer(framebufferDesc);
    passA->Resize(color_buffer, depth_buffer);
    passB->Resize(color_buffer, depth_buffer);
  } else {
    // TODO
  }
}

void SampleApp::renderFrame() {

  auto const previousTime = current_time;
  auto wallTime = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch());
  current_time = static_cast<double>(wallTime.count()) / 1000000.0;
  frame_time = current_time - previousTime;

  // Render something
  m_render_context->begin_frame();
  nvrhi::CommandListHandle current_command_list_graphics =
      m_render_context->get_current_command_list(
          CommandQueueFamily::TYPE_GRAPHICS);
  current_command_list_graphics->open();
  passA->UpdateRenderdata(fcamera);
  passB->UpdateRenderdata(fcamera);
  passC->update_renderdata(frame_time);

  // simple pass render
  passB->Render(color_buffer, depth_buffer);
  passA->Render(color_buffer, depth_buffer);
  passC->Render(color_buffer, depth_buffer);
  
  nvrhi::TextureSlice dst_slice = nvrhi::TextureSlice();
  dst_slice.width = m_render_context->get_swapchain_info().width;
  dst_slice.height = m_render_context->get_swapchain_info().height;
  nvrhi::TextureSlice src_slice = nvrhi::TextureSlice();
  src_slice.width = m_render_context->get_swapchain_info().width;
  src_slice.height = m_render_context->get_swapchain_info().height;

  current_command_list_graphics->copyTexture(
      m_render_context->get_swapchain_back_buffer(), dst_slice, color_buffer,
      src_slice);
  current_command_list_graphics->close();
  m_render_context->excute_command_list(current_command_list_graphics);
  m_render_context->present(0);
}

void SampleApp::UpdateRenderData() {}

void SampleApp::handleWindowSizeChange() {
  HWND window_handle = m_window->getApiHandle();
  RECT client_rect;
  GetClientRect(window_handle, &client_rect);
  UINT width = client_rect.right - client_rect.left;
  UINT height = client_rect.bottom - client_rect.top;
  m_render_context->resize_swapchain(width, height);
  resizeFrameBuffer(width, height);
  SwapChainInfo info = m_render_context->get_swapchain_info();
  fcamera.UpdateCamera(info);
}

void SampleApp::handleRenderFrame() {

  UpdateRenderData();
  renderFrame();
}

void SampleApp::handleKeyboardEvent(int key, int action) {

  if (action != GLFW_RELEASE) {
    auto frame_time_min = glm::min(static_cast<float>(frame_time), 0.05f);
    glm::vec3 const forward = normalize(fcamera.center - fcamera.eye);
    glm::vec3 const right = cross(forward, fcamera.up);
    glm::vec3 const up = cross(right, forward);
    glm::vec3 acceleration = camera_translation * -30.0f;
    float const force = camera_speed * 10000.0f;
    switch (key) {
    case GLFW_KEY_W:
      camera_move_dirty = true;
      acceleration.z += force;
      break;
    case GLFW_KEY_S:
      camera_move_dirty = true;
      acceleration.z -= force;
      break;
    case GLFW_KEY_A:
      camera_move_dirty = true;
      acceleration.x += force;
      break;
    case GLFW_KEY_D:
      camera_move_dirty = true;
      acceleration.x -= force;
      break;
    case GLFW_KEY_Q:
      camera_move_dirty = true;
      acceleration.y += force;
      break;
    case GLFW_KEY_E:
      camera_move_dirty = true;
      acceleration.y -= force;
      break;
    }
    if (camera_move_dirty) {
      camera_translation += acceleration * 0.5f * (float)frame_time_min;
      camera_translation =
          glm::clamp(camera_translation, -camera_speed, camera_speed);
      // Clamp tiny values to zero to improve convergence to resting state
      auto const clampMin =
          glm::lessThan(glm::abs(camera_translation), glm::vec3(0.0000001f));
      if (glm::any(clampMin)) {
        if (clampMin.x) {
          camera_translation.x = 0.0f;
        }
        if (clampMin.y) {
          camera_translation.y = 0.0f;
        }
        if (clampMin.z) {
          camera_translation.z = 0.0f;
        }
      }
      glm::vec3 translation = camera_translation;
      fcamera.eye +=
          translation.x * right + translation.y * up + translation.z * forward;
      fcamera.center +=
          translation.x * right + translation.y * up + translation.z * forward;
      SwapChainInfo info = m_render_context->get_swapchain_info();
      fcamera.UpdateCamera(info);
      camera_move_dirty = false;
    }
  }
}

void SampleApp::handleMouseEvent(int action, double x, double y) {
  if (action != -2) {
    auto frame_time_min = glm::min(static_cast<float>(frame_time), 0.05f);
    glm::vec3 const forward = normalize(fcamera.center - fcamera.eye);
    glm::vec3 const right = cross(forward, fcamera.up);
    glm::vec3 const up = cross(right, forward);
    float const force2 = 0.15f;
    glm::vec2 newXY = glm::vec2(x, y);
    newXY = newXY - mouse_pos;

    glm::vec2 acceleration = camera_rotation * -45.0f;
    acceleration.x += force2 * newXY.x;
    acceleration.y -= force2 * newXY.y;
    camera_rotation += acceleration * 0.5f * frame_time_min;
    camera_rotation = glm::clamp(camera_rotation, -4e-2f, 4e-2f);
    // Clamp tiny values to zero to improve convergence to resting state
    auto const clampRotationMin =
        glm::lessThan(glm::abs(camera_rotation), glm::vec2(0.00000001f));
    if (glm::any(clampRotationMin)) {
      if (clampRotationMin.x) {
        camera_rotation.x = 0.0f;
      }
      if (clampRotationMin.y) {
        camera_rotation.y = 0.0f;
      }
    }
    glm::quat rotationX = glm::angleAxis(camera_rotation.x, up);
    glm::quat rotationY = glm::angleAxis(camera_rotation.y, right);

    const glm::vec3 newForward = normalize(forward * rotationX * rotationY);
    if (abs(dot(newForward, glm::vec3(0.0f, 1.0f, 0.0f))) < 0.9f) {
      // Prevent view and up direction becoming parallel (this uses a FPS style
      // camera)
      fcamera.center = fcamera.eye + newForward;
      const glm::vec3 newRight =
          normalize(cross(newForward, glm::vec3(0.0f, 1.0f, 0.0f)));
      fcamera.up = normalize(cross(newRight, newForward));
      SwapChainInfo info = m_render_context->get_swapchain_info();
      fcamera.UpdateCamera(info);
      mouse_pos = glm::vec2(x, y);
    }
  } else {
    mouse_pos = glm::vec2(x, y);
  }
}
} // namespace Yoda
