#include "sky_pass.h"
#include "core/fly_camera.h"
#include "core/shadercode.h"
#include "rhi/d3d12/rhi_context_d3d12.h"
#include <glm/glm.hpp>
#include <memory>
#include <nvrhi/utils.h>
#include <stb_image.h>

struct ConstantBufferEntry {
  glm::vec4 viewProjMatrix;
  glm::mat4 projectVec;
  glm::mat4 transformMatrix;
  float padding[28];
};

namespace Yoda {
SkyPass::SkyPass(std::shared_ptr<RHIContextD3D12> context) {
  render_contex = context;
  ShaderByteCode vs_byte_code;
  std::string path = "shaders/sky.hlsl";
  std::string entry_point = "main_vs";
  vs_byte_code.compile_shader(nvrhi::ShaderType::Vertex, path, entry_point);

  ShaderByteCode ps_byte_code;
  entry_point = "main_ps";
  ps_byte_code.compile_shader(nvrhi::ShaderType::Pixel, path, entry_point);
  vs_shader = context->shader_create_from_bytecode(vs_byte_code.m_type,
                                                   vs_byte_code.m_byte_code);
  ps_shader = context->shader_create_from_bytecode(ps_byte_code.m_type,
                                                   ps_byte_code.m_byte_code);

  int imgX, imgY, comp;
  float *hdr_raw_texture =
      stbi_loadf("textures/barcelona.hdr", &imgX, &imgY, &comp, 0);
  ImgData<float> iData;
  iData.comp = comp;
  iData.width = imgX;
  iData.height = imgY;
  iData.data = hdr_raw_texture;
  hdr_tex_data = iData;

  nvrhi::TextureDesc HDRtextureDesc =
      nvrhi::TextureDesc()
          .setDimension(nvrhi::TextureDimension::Texture2D)
          .setWidth(imgX)
          .setHeight(imgY)
          .setMipLevels(1)
          .setArraySize(1)
          .setDepth(1)
          .setFormat(nvrhi::Format::RGB32_FLOAT)
          .setInitialState(nvrhi::ResourceStates::ShaderResource)
          .setKeepInitialState(true)
          .setDebugName("HDR Texture");
  hdr_texture = context->nvrhi_device->createTexture(HDRtextureDesc);

  auto layoutDesc =
      nvrhi::BindingLayoutDesc()
          .setVisibility(nvrhi::ShaderType::All)
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0)) // texture at t0
          .addItem(nvrhi::BindingLayoutItem::Sampler(0))
          .addItem(nvrhi::BindingLayoutItem::ConstantBuffer(0));

  binding_layout = context->nvrhi_device->createBindingLayout(layoutDesc);

  auto &textureSampler =
      nvrhi::SamplerDesc()
          .setAllFilters(true)
          .setMaxAnisotropy(16.0f)
          .setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);

  clamp_sampler = context->nvrhi_device->createSampler(textureSampler);
  constant_buffer = context->nvrhi_device->createBuffer(
      nvrhi::utils::CreateStaticConstantBufferDesc(sizeof(ConstantBufferEntry),
                                                   "ConstantBuffer")
          .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
          .setKeepInitialState(true));
  auto bindingSetDesc =
      nvrhi::BindingSetDesc()
          .addItem(nvrhi::BindingSetItem::Texture_SRV(0, hdr_texture))
          .addItem(nvrhi::BindingSetItem::Sampler(0, clamp_sampler))
          .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, constant_buffer));
  binding_set =
      context->nvrhi_device->createBindingSet(bindingSetDesc, binding_layout);
}

void SkyPass::Render(nvrhi::TextureHandle color_tex,
                     nvrhi::TextureHandle depth_tex) {
  nvrhi::CommandListHandle current_command_list_graphics =
      render_contex->get_current_command_list(
          CommandQueueFamily::TYPE_GRAPHICS);
  if (!pipeline) {
    auto framebufferDesc = nvrhi::FramebufferDesc()
                               .addColorAttachment(color_tex)
                               .setDepthAttachment(depth_tex);
    framebuffer =
        render_contex->nvrhi_device->createFramebuffer(framebufferDesc);

    nvrhi::GraphicsPipelineDesc psoDesc;
    psoDesc.VS = vs_shader;
    psoDesc.PS = ps_shader;
    psoDesc.primType = nvrhi::PrimitiveType::TriangleList;
    nvrhi::DepthStencilState defaultSt = {};
    defaultSt.setDepthFunc(nvrhi::ComparisonFunc::GreaterOrEqual);
    defaultSt.setDepthTestEnable(false);
    defaultSt.setDepthWriteEnable(true);
    psoDesc.renderState.depthStencilState = defaultSt;
    psoDesc.renderState.rasterState.depthClipEnable = true;
    psoDesc.addBindingLayout(binding_layout);
    pipeline = render_contex->nvrhi_device->createGraphicsPipeline(psoDesc,
                                                                   framebuffer);
  }
  nvrhi::GraphicsState state;
  state.pipeline = pipeline;
  state.framebuffer = framebuffer;
  state.addBindingSet(binding_set);
  state.viewport.addViewportAndScissorRect(
      framebuffer->getFramebufferInfo().getViewport());
  current_command_list_graphics->setGraphicsState(state);
  nvrhi::utils::ClearColorAttachment(current_command_list_graphics, framebuffer,
                                     0, nvrhi::Color(0.0f));
  nvrhi::utils::ClearDepthStencilAttachment(current_command_list_graphics,
                                            framebuffer, 0.0, 0.0);
  nvrhi::DrawArguments args;
  args.vertexCount = 3;
  current_command_list_graphics->draw(args);
}

SkyPass::~SkyPass() {
  vs_shader = nullptr;
  ps_shader = nullptr;
  binding_layout = nullptr;
  binding_set = nullptr;
  clamp_sampler = nullptr;
  constant_buffer = nullptr;
  pipeline = nullptr;
  hdr_texture = nullptr;
  framebuffer = nullptr;
}

struct sky_constanct_buffer {
  glm::mat4 transformMat;
  glm::vec4 projectVec;
};

void SkyPass::UpdateRenderdata(FlyCamera camera) {
  nvrhi::CommandListHandle current_command_list_graphics =
      render_contex->get_current_command_list(
          CommandQueueFamily::TYPE_GRAPHICS);
  if (is_start_frame) {
    current_command_list_graphics->writeTexture(
        hdr_texture, 0, 0, hdr_tex_data.data,
        hdr_tex_data.width * hdr_tex_data.comp * sizeof(float));
    is_start_frame = false;
  }
  if (is_pre_frame) {
    struct sky_constanct_buffer buffer;
    buffer.transformMat = camera.view;
    buffer.transformMat = camera.proj * buffer.transformMat;
    buffer.projectVec = glm::vec4(1.0);
    buffer.projectVec.x = camera.proj[2][0];
    buffer.projectVec.y = camera.proj[0][0];
    buffer.projectVec.z = camera.proj[2][1];
    buffer.projectVec.w = camera.proj[1][1];


    buffer.projectVec.x = 0.0;
    buffer.projectVec.y = camera.proj[0][0];
    buffer.projectVec.z = 0.0;
    buffer.projectVec.w = camera.proj[1][1];

    current_command_list_graphics->writeBuffer(constant_buffer, &buffer,
                                               sizeof(buffer));
  }
}

void SkyPass::Resize(nvrhi::TextureHandle col_tex,
                     nvrhi::TextureHandle depth_tex) {
  framebuffer = nullptr;
  nvrhi::DeviceHandle devicePtr = render_contex->nvrhi_device;
  auto framebufferDesc =
      nvrhi::FramebufferDesc().addColorAttachment(col_tex).setDepthAttachment(
          depth_tex);
  framebuffer = devicePtr->createFramebuffer(framebufferDesc);
}

} // namespace Yoda