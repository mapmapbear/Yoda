#pragma once
#include <memory>
#include <nvrhi/nvrhi.h>
#include "rhi/rhi_context_commons.h"

namespace Yoda {
class RHIContextD3D12;
class FlyCamera;
class SkyPass {
public:
  SkyPass(std::shared_ptr<RHIContextD3D12> context);

  ~SkyPass();
  void UpdateRenderdata(FlyCamera camera);
  void Render(nvrhi::TextureHandle color_tex, nvrhi::TextureHandle depth_tex);
  void Resize(nvrhi::TextureHandle col_tex, nvrhi::TextureHandle depth_tex);
protected:
  nvrhi::ShaderHandle vs_shader;
  nvrhi::ShaderHandle ps_shader;
  nvrhi::GraphicsPipelineHandle pipeline;
  nvrhi::BindingLayoutHandle binding_layout;
  nvrhi::BindingSetHandle binding_set;
  nvrhi::SamplerHandle clamp_sampler;
  nvrhi::BufferHandle constant_buffer;
  nvrhi::FramebufferHandle framebuffer;

protected:
  nvrhi::TextureHandle hdr_texture;
  ImgData<float> hdr_tex_data;

protected:
  std::shared_ptr<RHIContextD3D12> render_contex;
  bool is_pre_frame = true;
  bool is_start_frame = true;
};
} // namespace Yoda