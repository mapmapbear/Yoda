#include "application/sample_app.h"
#include "core/window.h"
#include "render/copyer.h"
#include "render/shader_bytecode.h"
#include "render/texture.h"
#include "rhi/d3d12/rhi_context_d3d12.h"
#include "rhi/d3d12/rhi_d3d12_command_list.h"
#include "rhi/rhi_context_commons.h"
#include <memory>
#include <windef.h>

namespace Yoda {
SampleApp::SampleApp(const SampleAppConfig &config) {
  mShowUI = config.showUI;
  mVsyncOn = config.windowDesc.enableVSync;

  if (!config.headless) {
    auto windowDesc = config.windowDesc;
    // Create the window
    m_window = Window::create(windowDesc, this);
    // m_window->setWindowIcon(getRuntimeDirectory() /
    // "data/framework/nvidia.ico");c HWND hw = Window::GetHwnd();
    m_render_context =
        std::make_shared<RHIContextD3D12>(m_window->getApiHandle());

    GraphicsPipelineSpecs specs;
    specs.FormatCount = 1;
    specs.Formats[0] = TextureFormat::RGBA8;
    specs.DepthFormat = TextureFormat::R32Depth;
    specs.Depth = DepthOperation::Less;
    specs.DepthEnabled = true;
    specs.Cull = CullMode::None;
    specs.Fill = FillMode::Solid;
    ShaderCompiler::CompileShader("shaders/HelloTriangle/TriVertex.hlsl",
                                  "Main", ShaderType::Vertex,
                                  specs.Bytecodes[ShaderType::Vertex]);
    ShaderCompiler::CompileShader("shaders/HelloTriangle/TriFrag.hlsl", "Main",
                                  ShaderType::Fragment,
                                  specs.Bytecodes[ShaderType::Fragment]);

    m_triPipeline = m_render_context->create_graphics_pipeline(specs);
  }
}

SampleApp::~SampleApp() { m_window.reset(); }
int SampleApp::run() {
  if (m_window) {
    m_window->msgLoop();
    m_render_context->begin_frame();
    std::shared_ptr<D3D12CommandList> current_commandlist =
        m_render_context->get_current_command_list();
    std::shared_ptr<Texture> color_texture =
        m_render_context->get_back_buffer();

    current_commandlist->begin();
    current_commandlist->resource_barrier(color_texture,
                                          TextureLayout::RenderTarget);
    current_commandlist->set_topology(Topology::TriangleList);
    current_commandlist->bind_rendertarget({color_texture}, nullptr);
    current_commandlist->clear_rendertarget(color_texture, 0.0, 0.0, 0.0, 0.0);
    current_commandlist->bind_graphics_pipeline(m_triPipeline);
    float vertices[] = {-0.5f, -0.5f, 0.0f, 0.5f, -0.5f,
                        0.0f,  0.0f,  0.5f, 0.0f};

    m_vertex_buffer = m_render_context->create_buffer(
        sizeof(vertices), sizeof(float) * 3, BufferType::Vertex, false);
    std::shared_ptr<Copyer> copyer = m_render_context->create_copyer();
    copyer->CopyHostToDeviceLocal(vertices, sizeof(vertices), m_vertex_buffer);
    m_render_context->FlushCopyerList(copyer);
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
    m_window->resize(width, height);
  } else {
    // TODO
  }
}

void SampleApp::renderFrame() {
  // Render something
}

void SampleApp::handleWindowSizeChange() {}

void SampleApp::handleRenderFrame() { renderFrame(); }
} // namespace Yoda
