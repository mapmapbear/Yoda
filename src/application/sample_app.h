#pragma once
#include "core/window.h"
#include "render/buffer.h"
#include "rhi/d3d12/rhi_d3d12_pipeline.h"
#include "rhi/rhi_context.h"
#include "rhi/d3d12/rhi_context_d3d12.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>



namespace Yoda
{

/**
 * Sample application configuration.
 */
struct SampleAppConfig
{
    // Device::Desc deviceDesc; ///< GPU device settings.
    Window::Desc windowDesc; ///< Window settings.

	//     ResourceFormat colorFormat = ResourceFormat::BGRA8UnormSrgb; ///< Color format of the frame buffer.
	//     ResourceFormat depthFormat = ResourceFormat::D32Float;       ///< Depth buffer format of the frame buffer.

    bool headless = false;  ///< Do not create a window and handle user input.
    float timeScale = 1.0f; ///< A scaling factor for the time elapsed between frames.
    bool pauseTime = false; ///< Control whether or not to start the clock when the sample start running.
    bool showUI = true;     ///< Show the UI.

    bool generateShaderDebugInfo = false;
    bool shaderPreciseFloat = false;
};

/**
 * Sample application base class.
 */
class SampleApp : public Window::ICallbacks
{
public:
    SampleApp(const SampleAppConfig& config);
    virtual ~SampleApp();
    int run();
    virtual void onShutdown() {}
    Window* getWindow() { return m_window.get(); }
    void resizeFrameBuffer(uint32_t width, uint32_t height);
    void renderFrame();
    void toggleUI(bool showUI) { mShowUI = showUI; }
    bool isUiEnabled() { return mShowUI; }
    void pauseRenderer(bool pause) { mRendererPaused = pause; }
    bool isRendererPaused() { return mRendererPaused; }
    void toggleVsync(bool on) { mVsyncOn = on; }
    bool isVsyncEnabled() { return mVsyncOn; }

private:
    // Implementation of IWindow::Callbacks

    void handleWindowSizeChange() override;
    void handleRenderFrame() override;
	// void handleKeyboardEvent(const KeyboardEvent& keyEvent) override;
	// void handleMouseEvent(const MouseEvent& mouseEvent) override;
	// void handleGamepadEvent(const GamepadEvent& gamepadEvent) override;
	// void handleGamepadState(const GamepadState& gamepadState) override;
	// void handleDroppedFile(const std::filesystem::path& path) override;
    std::shared_ptr<Window> m_window;              ///< Main window (nullptr if headless).   
    std::shared_ptr<RHIContextD3D12> m_render_context;
    // TODO:------------------
    // std::shared_ptr<Device> m_device;              ///< GPU device.
    // std::shared_ptr<Swapchain> m_swapchain;        ///< Main swapchain (nullptr if headless).
    // std::shared_ptr<CommandQueue> m_graphics_queue;
    // std::shared_ptr<CommandQueue> m_compute_queue;
    // std::shared_ptr<CommandQueue> m_copy_queue;
    // DescriptorHeap::Heaps m_heaps;
    // std::shared_ptr<Allocator> m_allocator;
    // --------------------------------

    bool mShouldTerminate = false; ///< True if application should terminate.
    bool mRendererPaused = false;  ///< True if rendering is paused.
    bool mVsyncOn = false;
    bool mShowUI = true;
    bool mCaptureScreen = false;

    int mReturnCode = 0;

    SampleApp(const SampleApp&) = delete;
    SampleApp& operator=(const SampleApp&) = delete;

    std::shared_ptr<D3D12Pipeline> m_triPipeline;
    std::shared_ptr<Buffer> m_vertex_buffer;
};
}; // namespace Yoda
