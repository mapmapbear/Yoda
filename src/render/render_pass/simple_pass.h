#pragma once
#include "render/world.h"
#include <memory>
#include <nvrhi/nvrhi.h>

namespace Yoda {
class RHIContextD3D12;
class FlyCamera;

class SimplePass {
public:
  SimplePass(std::shared_ptr<RHIContextD3D12> context, World *wolrd);
  ~SimplePass();
  void Render(nvrhi::TextureHandle col_tex, nvrhi::TextureHandle depth_tex);
  void PreZ_Render(nvrhi::TextureHandle col_tex,
                   nvrhi::TextureHandle depth_tex);
  void Base_Render(nvrhi::TextureHandle col_tex,
                   nvrhi::TextureHandle depth_tex);
  void Submit();
  void UpdateRenderdata(FlyCamera camera);
  void Resize(nvrhi::TextureHandle col_tex, nvrhi::TextureHandle depth_tex);

protected:
  nvrhi::ShaderHandle vs_shader;
  nvrhi::ShaderHandle ps_shader;
  nvrhi::GraphicsPipelineHandle pipeline;
  nvrhi::GraphicsPipelineHandle preZ_pipeline;
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
  nvrhi::FramebufferHandle framebuffer;
  nvrhi::FramebufferHandle preZ_framebuffer;

protected:
  nvrhi::TextureHandle albedo_texute;
  nvrhi::TextureDesc aldebo_desc;
  unsigned char *albedo_raw_data = nullptr;

  nvrhi::TextureDesc irradiance_desc;
  nvrhi::TextureHandle irradiance_texture;
  std::vector<unsigned char *> irradiance_raw_data;

  nvrhi::TextureDesc specular_desc;
  nvrhi::TextureHandle specular_texture;
  std::vector<unsigned char *> specular_raw_data;

protected:
  World *scene_world;

  std::shared_ptr<RHIContextD3D12> render_contex;
  bool is_pre_frame = true;
  bool is_start_frame = true;
};
} // namespace Yoda