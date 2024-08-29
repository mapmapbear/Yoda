#pragma once
#include <memory>
#include <nvrhi/nvrhi.h>
#include "rhi/rhi_context_commons.h"

namespace Yoda {
class RHIContextD3D12;
// class FlyCamera;
class PresentPass {
public:
  PresentPass(std::shared_ptr<RHIContextD3D12> context);

  ~PresentPass();
//   void UpdateRenderdata(FlyCamera camera);
  void Render(nvrhi::FramebufferHandle frame_buffer);
  void Resize(nvrhi::FramebufferHandle fram_buffer);
  void setMainColorTex(nvrhi::TextureHandle tex);
protected:
  nvrhi::ShaderHandle vs_shader;
  nvrhi::ShaderHandle ps_shader;
  nvrhi::GraphicsPipelineHandle pipeline;
  nvrhi::BindingLayoutHandle binding_layout;
  nvrhi::BindingSetHandle binding_set;
  nvrhi::FramebufferHandle frame_buffer;  
  // nvrhi::SamplerHandle clamp_sampler;

protected:
  nvrhi::TextureHandle main_texture;
//   ImgData<float> hdr_tex_data;

protected:
  std::shared_ptr<RHIContextD3D12> render_contex;
  bool is_pre_frame = true;
  bool is_start_frame = true;
};
} // namespace Yoda