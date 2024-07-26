#include "gui_pass.h"
#include "core/logger.h"
#include "core/shadercode.h"
#include "d3dx12_core.h"
#include "nvrhi/nvrhi.h"
#include "render/render_pass/gui_pass.h"
#include "rhi/d3d12/rhi_context_d3d12.h"
#include <cstdio>
#include <fstream>
#include <imgui.h>
#include <memory>

#define GLFW_INCLUDE_NONE // Do not include any OpenGL headers
#include <GLFW/glfw3.h>

namespace Yoda {
GUIPass::GUIPass(std::shared_ptr<RHIContextD3D12> context) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  m_context = context;
}

bool GUIPass::init() {
  ImGuiIO &io = ImGui::GetIO();
  io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
  io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
  io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
  io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
  io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
  io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
  io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
  io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
  io.KeyMap[ImGuiKey_A] = 'A';
  io.KeyMap[ImGuiKey_C] = 'C';
  io.KeyMap[ImGuiKey_V] = 'V';
  io.KeyMap[ImGuiKey_X] = 'X';
  io.KeyMap[ImGuiKey_Y] = 'Y';
  io.KeyMap[ImGuiKey_Z] = 'Z';

  io.DisplaySize = ImVec2((float)(900), (float)(600));
  io.DisplaySize = ImVec2((float)(m_context->get_swapchain_info().width),
                          (float)(m_context->get_swapchain_info().height));

  io.IniFilename = nullptr;
  imgui_data = std::make_unique<GUIData>();
  //   m_font = load_font("fonts/DroidSans-Mono.ttf", 14.f);

  return imgui_data->init(m_context);
}

ImFont *GUIPass::load_font(const std::string &fontFile, float fontSize) {
  std::ifstream file(fontFile.c_str(), std::ios::binary);
  if (!file.is_open()) {
    LOG_ERROR("can't not open this file: {}", fontFile);
    return nullptr;
  }
  file.seekg(0, std::ios::end);
  std::streampos fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<char> fontData(fileSize);
  file.read(fontData.data(), fileSize);
  if (!file) {
    LOG_ERROR("can't not read this file: {}", fontFile);
    return nullptr;
  }
  file.close();

  if (fontData.empty())
    return nullptr;

  ImFontConfig fontConfig;

  // XXXX mk: this appears to be a bug: the atlas copies (& owns) the data when
  // the flag is set to false !
  fontConfig.FontDataOwnedByAtlas = false;
  ImFont *imFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
      (void *)(fontData.data()), (int)(fontData.size()), fontSize, &fontConfig);

  return imFont;
}

void GUIPass::update_renderdata(float delta_time) {
  int w, h;
  w = m_context->get_swapchain_info().width;
  h = m_context->get_swapchain_info().height;
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(float(w), float(h));

  io.KeyCtrl =
      io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
  io.KeyShift =
      io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
  io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
  io.KeySuper =
      io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];

  io.DeltaTime = delta_time;
  io.MouseDrawCursor = false;

  ImGui::NewFrame();
}

void GUIPass::Render(nvrhi::TextureHandle color_tex,
                     nvrhi::TextureHandle depth_tex) {
  auto framebufferDesc =
      nvrhi::FramebufferDesc().addColorAttachment(color_tex).setDepthAttachment(
          depth_tex);
  auto framebuffer =
      m_context->nvrhi_device->createFramebuffer(framebufferDesc);
  nvrhi::CommandListHandle current_commandList =
      m_context->get_current_command_list(CommandQueueFamily::TYPE_GRAPHICS);

  ImGui::ShowDemoWindow();
  ImGui::Render();
  ImDrawData *drawData = ImGui::GetDrawData();
  const auto &io = ImGui::GetIO();

  current_commandList->beginMarker("ImGUI");
  if (!imgui_data->updateGeometry(current_commandList)) {
    return;
  }

  drawData->ScaleClipRects(io.DisplayFramebufferScale);

  float invDisplaySize[2] = {1.f / io.DisplaySize.x, 1.f / io.DisplaySize.y};

  // set up graphics state
  nvrhi::GraphicsState drawState;

  drawState.framebuffer = framebuffer;
  assert(drawState.framebuffer);

  drawState.pipeline = imgui_data->getPSO(drawState.framebuffer);

  drawState.viewport.viewports.push_back(
      nvrhi::Viewport(io.DisplaySize.x * io.DisplayFramebufferScale.x,
                      io.DisplaySize.y * io.DisplayFramebufferScale.y));
  drawState.viewport.scissorRects.resize(1); // updated below

  nvrhi::VertexBufferBinding vbufBinding;
  vbufBinding.buffer = imgui_data->vertexBuffer;
  vbufBinding.slot = 0;
  vbufBinding.offset = 0;
  drawState.vertexBuffers.push_back(vbufBinding);

  drawState.indexBuffer.buffer = imgui_data->indexBuffer;
  drawState.indexBuffer.format =
      (sizeof(ImDrawIdx) == 2 ? nvrhi::Format::R16_UINT
                              : nvrhi::Format::R32_UINT);
  drawState.indexBuffer.offset = 0;

  // render command lists
  int vtxOffset = 0;
  int idxOffset = 0;
  for (int n = 0; n < drawData->CmdListsCount; n++) {
    const ImDrawList *cmdList = drawData->CmdLists[n];
    for (int i = 0; i < cmdList->CmdBuffer.Size; i++) {
      const ImDrawCmd *pCmd = &cmdList->CmdBuffer[i];

      if (pCmd->UserCallback) {
        pCmd->UserCallback(cmdList, pCmd);
      } else {
        drawState.bindings = {
            imgui_data->getBindingSet((nvrhi::ITexture *)pCmd->TextureId)};
        assert(drawState.bindings[0]);

        drawState.viewport.scissorRects[0] =
            nvrhi::Rect(int(pCmd->ClipRect.x), int(pCmd->ClipRect.z),
                        int(pCmd->ClipRect.y), int(pCmd->ClipRect.w));

        nvrhi::DrawArguments drawArguments;
        drawArguments.vertexCount = pCmd->ElemCount;
        drawArguments.startIndexLocation = idxOffset;
        drawArguments.startVertexLocation = vtxOffset;

        current_commandList->setGraphicsState(drawState);
        current_commandList->setPushConstants(invDisplaySize,
                                              sizeof(invDisplaySize));
        current_commandList->drawIndexed(drawArguments);
      }

      idxOffset += pCmd->ElemCount;
    }

    vtxOffset += cmdList->VtxBuffer.Size;
  }

  current_commandList->endMarker();
}

GUIPass::~GUIPass() {
  imgui_data->vertexShader = nullptr;
  imgui_data->pixelShader = nullptr;
  imgui_data->shaderAttribLayout = nullptr;

  imgui_data->fontTexture = nullptr;
  imgui_data->fontSampler = nullptr;

  imgui_data->vertexBuffer = nullptr;
  imgui_data->indexBuffer = nullptr;

  imgui_data->bindingLayout = nullptr;

  imgui_data->pso = nullptr;
  imgui_data->bindingsCache.clear();
  imgui_data->vtxBuffer.clear();
  imgui_data->idxBuffer.clear();
  imgui_data = nullptr;
}

bool GUIData::init(std::shared_ptr<RHIContextD3D12> context) {
  m_context = context;
  nvrhi::CommandListHandle current_commandList =
      context->get_current_command_list(CommandQueueFamily::TYPE_GRAPHICS);
  ShaderByteCode vs_byte_code;
  std::string path = "shaders/imgui.hlsl";
  std::string entry_point = "main_vs";
  vs_byte_code.compile_shader(nvrhi::ShaderType::Vertex, path, entry_point);

  ShaderByteCode ps_byte_code;
  entry_point = "main_ps";
  ps_byte_code.compile_shader(nvrhi::ShaderType::Pixel, path, entry_point);

  vertexShader = context->shader_create_from_bytecode(vs_byte_code.m_type,
                                                      vs_byte_code.m_byte_code);
  pixelShader = context->shader_create_from_bytecode(ps_byte_code.m_type,
                                                     ps_byte_code.m_byte_code);

  // create attribute layout object
  nvrhi::VertexAttributeDesc vertexAttribLayout[] = {
      {"POSITION", nvrhi::Format::RG32_FLOAT, 1, 0, offsetof(ImDrawVert, pos),
       sizeof(ImDrawVert), false},
      {"TEXCOORD", nvrhi::Format::RG32_FLOAT, 1, 0, offsetof(ImDrawVert, uv),
       sizeof(ImDrawVert), false},
      {"COLOR", nvrhi::Format::RGBA8_UNORM, 1, 0, offsetof(ImDrawVert, col),
       sizeof(ImDrawVert), false},
  };

  shaderAttribLayout = context->nvrhi_device->createInputLayout(
      vertexAttribLayout,
      sizeof(vertexAttribLayout) / sizeof(vertexAttribLayout[0]), vertexShader);

  // add the default font - before creating the font texture
  auto &io = ImGui::GetIO();
  io.Fonts->AddFontDefault();

  // create font texture
  if (!create_font_texture(current_commandList)) {
    return false;
  }

  // create PSO
  {
    nvrhi::BlendState blendState;
    blendState.targets[0]
        .setBlendEnable(true)
        .setSrcBlend(nvrhi::BlendFactor::SrcAlpha)
        .setDestBlend(nvrhi::BlendFactor::InvSrcAlpha)
        .setSrcBlendAlpha(nvrhi::BlendFactor::InvSrcAlpha)
        .setDestBlendAlpha(nvrhi::BlendFactor::Zero);

    auto rasterState = nvrhi::RasterState()
                           .setFillSolid()
                           .setCullNone()
                           .setScissorEnable(true)
                           .setDepthClipEnable(true);

    auto depthStencilState = nvrhi::DepthStencilState()
                                 .disableDepthTest()
                                 .enableDepthWrite()
                                 .disableStencil()
                                 .setDepthFunc(nvrhi::ComparisonFunc::Always);

    nvrhi::RenderState renderState;
    renderState.blendState = blendState;
    renderState.depthStencilState = depthStencilState;
    renderState.rasterState = rasterState;

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::PushConstants(0, sizeof(float) * 2),
        nvrhi::BindingLayoutItem::Texture_SRV(0),
        nvrhi::BindingLayoutItem::Sampler(0)};
    bindingLayout = context->nvrhi_device->createBindingLayout(layoutDesc);

    basePSODesc.primType = nvrhi::PrimitiveType::TriangleList;
    basePSODesc.inputLayout = shaderAttribLayout;
    basePSODesc.VS = vertexShader;
    basePSODesc.PS = pixelShader;
    basePSODesc.renderState = renderState;
    basePSODesc.bindingLayouts = {bindingLayout};
  }

  return true;
}

bool GUIData::create_font_texture(nvrhi::CommandListHandle commandList) {
  ImGuiIO &io = ImGui::GetIO();
  int width, height;

  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

  {
    nvrhi::TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = nvrhi::Format::RGBA8_UNORM;
    desc.debugName = "ImGui font texture";
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    
    fontTexture = m_context->nvrhi_device->createTexture(desc);

    io.Fonts->TexID = fontTexture;
  }

  {
    const auto desc = nvrhi::SamplerDesc()
                          .setAllAddressModes(nvrhi::SamplerAddressMode::Wrap)
                          .setAllFilters(true);

    fontSampler = m_context->nvrhi_device->createSampler(desc);

    if (fontSampler == nullptr)
      return false;
  }

  return true;
}

bool GUIData::updateGeometry(nvrhi::ICommandList *commandList) {
  if (fontTexture == nullptr)
    return false;
  commandList->writeTexture(fontTexture, 0, 0, pixels,
                            fontTexture->getDesc().width * 4);
  ImDrawData *drawData = ImGui::GetDrawData();

  // create/resize vertex and index buffers if needed
  if (!reallocateBuffer(
          vertexBuffer, drawData->TotalVtxCount * sizeof(ImDrawVert),
          (drawData->TotalVtxCount + 5000) * sizeof(ImDrawVert), false)) {
    return false;
  }

  if (!reallocateBuffer(
          indexBuffer, drawData->TotalIdxCount * sizeof(ImDrawIdx),
          (drawData->TotalIdxCount + 5000) * sizeof(ImDrawIdx), true)) {
    return false;
  }

  vtxBuffer.resize(vertexBuffer->getDesc().byteSize / sizeof(ImDrawVert));
  idxBuffer.resize(indexBuffer->getDesc().byteSize / sizeof(ImDrawIdx));

  // copy and convert all vertices into a single contiguous buffer
  ImDrawVert *vtxDst = &vtxBuffer[0];
  ImDrawIdx *idxDst = &idxBuffer[0];

  for (int n = 0; n < drawData->CmdListsCount; n++) {
    const ImDrawList *cmdList = drawData->CmdLists[n];

    memcpy(vtxDst, cmdList->VtxBuffer.Data,
           cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
    memcpy(idxDst, cmdList->IdxBuffer.Data,
           cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

    vtxDst += cmdList->VtxBuffer.Size;
    idxDst += cmdList->IdxBuffer.Size;
  }

  commandList->writeBuffer(vertexBuffer, &vtxBuffer[0],
                           vertexBuffer->getDesc().byteSize);
  commandList->writeBuffer(indexBuffer, &idxBuffer[0],
                           indexBuffer->getDesc().byteSize);

  return true;
}

bool GUIData::reallocateBuffer(nvrhi::BufferHandle &buffer, size_t requiredSize,
                               size_t reallocateSize, const bool indexBuffer) {
  if (buffer == nullptr || size_t(buffer->getDesc().byteSize) < requiredSize) {
    nvrhi::BufferDesc desc;
    desc.byteSize = uint32_t(reallocateSize);
    desc.structStride = 0;
    desc.debugName = indexBuffer ? "ImGui index buffer" : "ImGui vertex buffer";
    desc.canHaveUAVs = false;
    desc.isVertexBuffer = !indexBuffer;
    desc.isIndexBuffer = indexBuffer;
    desc.isDrawIndirectArgs = false;
    desc.isVolatile = false;
    desc.initialState = indexBuffer ? nvrhi::ResourceStates::IndexBuffer
                                    : nvrhi::ResourceStates::VertexBuffer;
    desc.keepInitialState = true;

    buffer = m_context->nvrhi_device->createBuffer(desc);

    if (!buffer) {
      return false;
    }
  }

  return true;
}

nvrhi::GraphicsPipelineHandle GUIData::getPSO(nvrhi::IFramebuffer *fb) {
  if (pso)
    return pso;

  pso = m_context->nvrhi_device->createGraphicsPipeline(basePSODesc, fb);
  assert(pso);

  return pso;
}

nvrhi::BindingSetHandle GUIData::getBindingSet(nvrhi::ITexture *texture) {
  auto iter = bindingsCache.find(texture);
  if (iter != bindingsCache.end()) {
    return iter->second;
  }

  nvrhi::BindingSetDesc desc;

  desc.bindings = {nvrhi::BindingSetItem::PushConstants(0, sizeof(float) * 2),
                   nvrhi::BindingSetItem::Texture_SRV(0, texture),
                   nvrhi::BindingSetItem::Sampler(0, fontSampler)};

  nvrhi::BindingSetHandle binding;
  binding = m_context->nvrhi_device->createBindingSet(desc, bindingLayout);
  assert(binding);

  bindingsCache[texture] = binding;
  return binding;
}

} // namespace Yoda