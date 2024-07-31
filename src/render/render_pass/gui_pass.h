#pragma once
#include "nvrhi/nvrhi.h"
#include "render/world.h"
#include "ufbx.h"
#include <imgui.h>
#include <memory>
#include <unordered_map>

namespace Yoda {
class RHIContextD3D12;
class World;
class GUIData {
public:
  bool init(std::shared_ptr<RHIContextD3D12> context);
  bool create_font_texture(nvrhi::CommandListHandle commandList);
  bool updateGeometry(nvrhi::ICommandList *commandList);
  bool reallocateBuffer(nvrhi::BufferHandle &buffer, size_t requiredSize,
                        size_t reallocateSize, const bool indexBuffer);
  nvrhi::BindingSetHandle getBindingSet(nvrhi::ITexture *texture);
  nvrhi::GraphicsPipelineHandle getPSO(nvrhi::IFramebuffer *fb);

public:
  std::shared_ptr<RHIContextD3D12> m_context;
  // nvrhi::CommandListHandle m_commandList;

  nvrhi::ShaderHandle vertexShader;
  nvrhi::ShaderHandle pixelShader;
  nvrhi::InputLayoutHandle shaderAttribLayout;

  nvrhi::TextureHandle fontTexture;
  nvrhi::SamplerHandle fontSampler;

  nvrhi::BufferHandle vertexBuffer;
  nvrhi::BufferHandle indexBuffer;

  nvrhi::BindingLayoutHandle bindingLayout;
  nvrhi::GraphicsPipelineDesc basePSODesc;

  nvrhi::GraphicsPipelineHandle pso;
  std::unordered_map<nvrhi::TextureHandle, nvrhi::BindingSetHandle>
      bindingsCache;

  std::vector<ImDrawVert> vtxBuffer;
  std::vector<ImDrawIdx> idxBuffer;

protected:
  unsigned char *pixels;
};

class GUIPass {
public:
  GUIPass(std::shared_ptr<RHIContextD3D12> context, World& world);
  ~GUIPass();
  bool init();
  ImFont *load_font(const std::string &fontFile, float fontSize);
  void update_renderdata(float delta_time);
  void Render(nvrhi::TextureHandle color_tex, nvrhi::TextureHandle depth_tex);
  bool mouse_pos_update(float xpos, float ypos);
  bool mouse_button_update(int button, int action);
  bool keybord_update(int key, int action);
  bool keybord_char_input(int code);
  void build_UI();
  void set_world_scene(World &world);

protected:
  std::unique_ptr<GUIData> imgui_data;
  std::shared_ptr<RHIContextD3D12> m_context;
  ImFont *m_font = nullptr;
  World scene_world;
};
} // namespace Yoda