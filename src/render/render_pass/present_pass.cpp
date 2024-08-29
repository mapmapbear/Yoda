#include "present_pass.h"
#include "core/shadercode.h"
#include "rhi/d3d12/rhi_context_d3d12.h"

namespace Yoda {
PresentPass::PresentPass(std::shared_ptr<RHIContextD3D12> context) {
  render_contex = context;
  // main_texture = mainTexture;
  ShaderByteCode vs_byte_code;
  std::string path = "shaders/blitSDR.hlsl";
  std::string entry_point = "main_vs";
  vs_byte_code.compile_shader(nvrhi::ShaderType::Vertex, path, entry_point);

  ShaderByteCode ps_byte_code;
  entry_point = "main_ps";
  ps_byte_code.compile_shader(nvrhi::ShaderType::Pixel, path, entry_point);
  vs_shader = context->shader_create_from_bytecode(vs_byte_code.m_type,
                                                   vs_byte_code.m_byte_code);
  ps_shader = context->shader_create_from_bytecode(ps_byte_code.m_type,
                                                   ps_byte_code.m_byte_code);
  // auto &textureSampler =
  //     nvrhi::SamplerDesc()
  //         .setAllFilters(true)
  //         .setMaxAnisotropy(16.0f)
  //         .setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);

  // clamp_sampler = context->nvrhi_device->createSampler(textureSampler);
}

void PresentPass::Render(nvrhi::FramebufferHandle frameBuffer) {
  frame_buffer = frameBuffer;
  nvrhi::CommandListHandle current_command_list_graphics =
      render_contex->get_current_command_list(
          CommandQueueFamily::TYPE_GRAPHICS);
  if (!pipeline) {
    auto layoutDesc = nvrhi::BindingLayoutDesc()
                          .setVisibility(nvrhi::ShaderType::All)
                          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0));
                          // .addItem(nvrhi::BindingLayoutItem::Sampler(0));

    binding_layout =
        render_contex->nvrhi_device->createBindingLayout(layoutDesc);

    auto bindingSetDesc =
        nvrhi::BindingSetDesc()
            .addItem(nvrhi::BindingSetItem::Texture_SRV(0, main_texture));
            // .addItem(nvrhi::BindingSetItem::Sampler(0, clamp_sampler));
    binding_set = render_contex->nvrhi_device->createBindingSet(bindingSetDesc,
                                                                binding_layout);

    nvrhi::GraphicsPipelineDesc psoDesc;
    psoDesc.VS = vs_shader;
    psoDesc.PS = ps_shader;
    psoDesc.primType = nvrhi::PrimitiveType::TriangleList;
    nvrhi::DepthStencilState defaultSt = {};
    defaultSt.setDepthTestEnable(false);
    defaultSt.setDepthWriteEnable(false);
    psoDesc.renderState.depthStencilState = defaultSt;
    psoDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    psoDesc.renderState.rasterState.depthClipEnable = true;
    psoDesc.addBindingLayout(binding_layout);
    pipeline = render_contex->nvrhi_device->createGraphicsPipeline(
        psoDesc, frame_buffer);
  }
  nvrhi::GraphicsState state;
  state.pipeline = pipeline;
  state.framebuffer = frameBuffer;
  state.addBindingSet(binding_set);
  state.viewport.addViewportAndScissorRect(
      frameBuffer->getFramebufferInfo().getViewport());
  current_command_list_graphics->setGraphicsState(state);
  nvrhi::DrawArguments args;
  args.vertexCount = 3;
  current_command_list_graphics->draw(args);
}

PresentPass::~PresentPass() {
  vs_shader = nullptr;
  ps_shader = nullptr;
  binding_layout = nullptr;
  binding_set = nullptr;
  pipeline = nullptr;
}

void PresentPass::setMainColorTex(nvrhi::TextureHandle tex) {
  main_texture = tex;
}

void PresentPass::Resize(nvrhi::FramebufferHandle framBuffer) {
  frame_buffer = framBuffer;
  // nvrhi::DeviceHandle devicePtr = render_contex->nvrhi_device;
  // auto framebufferDesc =
  //     nvrhi::FramebufferDesc().addColorAttachment(col_tex).setDepthAttachment(
  //         depth_tex);
  // framebuffer = devicePtr->createFramebuffer(framebufferDesc);
}

} // namespace Yoda
