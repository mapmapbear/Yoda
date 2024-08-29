#pragma once
#include "core/window.h"
#include "render/render_pass/gui_pass.h"
#include "render/world.h"
#include "rhi/d3d12/rhi_context_d3d12.h"
#include "rhi/rhi_context.h"
#include <core/fly_camera.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>


namespace Yoda {

/**
 * Sample application configuration.
 */
struct SampleAppConfig {
  // Device::Desc deviceDesc; ///< GPU device settings.
  Window::Desc windowDesc; ///< Window settings.

  //     ResourceFormat colorFormat = ResourceFormat::BGRA8UnormSrgb; ///< Color
  //     format of the frame buffer. ResourceFormat depthFormat =
  //     ResourceFormat::D32Float;       ///< Depth buffer format of the frame
  //     buffer.

  bool headless = false; ///< Do not create a window and handle user input.
  float timeScale =
      1.0f; ///< A scaling factor for the time elapsed between frames.
  bool pauseTime = false; ///< Control whether or not to start the clock when
                          ///< the sample start running.
  bool showUI = true;     ///< Show the UI.

  bool generateShaderDebugInfo = false;
  bool shaderPreciseFloat = false;
};
class SimplePass;
class SkyPass;
class GUIPass;
class PresentPass;
class SampleApp : public Window::ICallbacks {
public:
  SampleApp(const SampleAppConfig &config);
  virtual ~SampleApp();
  int run();
  virtual void onShutdown() {}
  Window *getWindow() { return m_window.get(); }
  void resizeFrameBuffer(uint32_t width, uint32_t height);
  void renderFrame();
  void UpdateRenderData();
  void toggleUI(bool showUI) { mShowUI = showUI; }
  bool isUiEnabled() { return mShowUI; }
  void pauseRenderer(bool pause) { mRendererPaused = pause; }
  bool isRendererPaused() { return mRendererPaused; }
  void toggleVsync(bool on) { mVsyncOn = on; }
  bool isVsyncEnabled() { return mVsyncOn; }

public:
  std::shared_ptr<SimplePass> passA;
  std::shared_ptr<SkyPass> passB;
  std::shared_ptr<GUIPass> passC;
  std::shared_ptr<PresentPass> presentPass;

protected:
  // base pass
  nvrhi::ShaderHandle vs_shader;
  nvrhi::ShaderHandle ps_shader;
  nvrhi::GraphicsPipelineHandle pipeline;
  nvrhi::InputLayoutHandle input_layout;
  nvrhi::BufferHandle vertex_buffer;
  nvrhi::BufferHandle uv_buffer;
  nvrhi::BufferHandle normal_buffer;
  nvrhi::BufferHandle tangent_buffer;
  nvrhi::BufferHandle binormal_buffer;
  nvrhi::BufferHandle index_buffer;
  nvrhi::BufferHandle constant_buffer;
  nvrhi::BindingLayoutHandle binding_layout;
  nvrhi::BindingSetHandle binding_set;
  World scene_world;
  FlyCamera fcamera;

  nvrhi::TextureHandle depth_buffer;
  nvrhi::TextureHandle color_buffer;
  nvrhi::FramebufferHandle test_framebuffer;

private:
  // Implementation of IWindow::Callbacks

  void handleWindowSizeChange() override;
  void handleRenderFrame() override;
  void handleKeyboardEvent(int key, int action) override;
  void handleMouseEvent(int action, double x, double y) override;
  // void handleGamepadEvent(const GamepadEvent& gamepadEvent) override;
  // void handleGamepadState(const GamepadState& gamepadState) override;
  // void handleDroppedFile(const std::filesystem::path& path) override;
  std::shared_ptr<Window> m_window; ///< Main window (nullptr if headless).
  std::shared_ptr<RHIContextD3D12> m_render_context;
  double current_time =
      0.0;                 // Current wall clock time used for timing (seconds)
  double frame_time = 0.0; // Elapsed frame time for most recent frame (seconds)

  glm::vec3 camera_translation = glm::vec3(0.0f);
  glm::vec2 camera_rotation = glm::vec2(0.0f);
  glm::vec2 mouse_pos = glm::vec2(0.0f);
  float camera_speed = 1.2f;
  bool camera_move_dirty = false;
  bool mShouldTerminate = false; ///< True if application should terminate.
  bool mRendererPaused = false;  ///< True if rendering is paused.
  bool mVsyncOn = false;
  bool mShowUI = true;
  bool mCaptureScreen = false;

  int mReturnCode = 0;

  SampleApp(const SampleApp &) = delete;
  SampleApp &operator=(const SampleApp &) = delete;
};
}; // namespace Yoda
