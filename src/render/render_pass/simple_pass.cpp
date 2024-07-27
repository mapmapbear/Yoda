#include "simple_pass.h"
#include "core/fly_camera.h"
#include "core/shadercode.h"
#include "glm/trigonometric.hpp"
#include "nvrhi/nvrhi.h"
#include "rhi/d3d12/rhi_context_d3d12.h"
#include "rhi/rhi_context_commons.h"
#include <glm/gtc/quaternion.hpp>
#include <nvrhi/utils.h>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Yoda {
SimplePass::SimplePass(std::shared_ptr<RHIContextD3D12> context) {
  render_contex = context;
  ShaderByteCode vs_byte_code;
  std::string path = "shaders/basic_triangle.hlsl";
  std::string entry_point = "main_vs";
  vs_byte_code.compile_shader(nvrhi::ShaderType::Vertex, path, entry_point);

  ShaderByteCode ps_byte_code;
  entry_point = "main_ps";
  ps_byte_code.compile_shader(nvrhi::ShaderType::Pixel, path, entry_point);
  vs_shader = context->shader_create_from_bytecode(vs_byte_code.m_type,
                                                   vs_byte_code.m_byte_code);
  ps_shader = context->shader_create_from_bytecode(ps_byte_code.m_type,
                                                   ps_byte_code.m_byte_code);

  std::string test_scene_path = "module/sphere.fbx";
  //   std::string test_scene_path = "module/bistro/BistroExterior.fbx";
  bool state = World::load_scene2(test_scene_path, scene_world);

  int x, y, n;
  albedo_raw_data = stbi_load("textures/test.png", &x, &y, &n, 4);

  nvrhi::VertexAttributeDesc attributes[] = {
      nvrhi::VertexAttributeDesc()
          .setName("POSITION")
          .setFormat(nvrhi::Format::RGB32_FLOAT)
          .setBufferIndex(0)
          .setElementStride(sizeof(float) * 3),
      nvrhi::VertexAttributeDesc()
          .setName("TEXCOORD")
          .setFormat(nvrhi::Format::RG32_FLOAT)
          .setBufferIndex(1)
          .setElementStride(sizeof(float) * 2),

      nvrhi::VertexAttributeDesc()
          .setName("NORMAL")
          .setFormat(nvrhi::Format::RGB32_FLOAT)
          .setBufferIndex(2)
          .setElementStride(sizeof(float) * 3),

      nvrhi::VertexAttributeDesc()
          .setName("TANGENT")
          .setFormat(nvrhi::Format::RGB32_FLOAT)
          .setBufferIndex(3)
          .setElementStride(sizeof(float) * 3),

      nvrhi::VertexAttributeDesc()
          .setName("BINORMAL")
          .setFormat(nvrhi::Format::RGB32_FLOAT)
          .setBufferIndex(4)
          .setElementStride(sizeof(float) * 3),

  };
  nvrhi::DeviceHandle device =
      context->get_current_command_list(CommandQueueFamily::TYPE_GRAPHICS)
          ->getDevice();
  input_layout = device->createInputLayout(
      attributes, uint32_t(std::size(attributes)), vs_shader);

  aldebo_desc = nvrhi::TextureDesc()
                    .setDimension(nvrhi::TextureDimension::Texture2D)
                    .setWidth(x)
                    .setHeight(y)
                    .setFormat(nvrhi::Format::RGBA8_UNORM)
                    .setInitialState(nvrhi::ResourceStates::ShaderResource)
                    .setKeepInitialState(true)
                    .setDebugName("Albedo Texture");
  albedo_texute = device->createTexture(aldebo_desc);

  // Create Vertex Buffer
  nvrhi::BufferDesc vertexBufferDesc;
  vertexBufferDesc.byteSize =
      sizeof(scene_world.mesh_group[0]->positions_stream[0]) *
      scene_world.mesh_group[0]->positions_stream.size();
  vertexBufferDesc.isVertexBuffer = true;
  vertexBufferDesc.debugName = "VertexBuffer";
  vertexBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
  vertex_buffer = device->createBuffer(vertexBufferDesc);

  nvrhi::BufferDesc vertexUVBufferDesc;
  vertexUVBufferDesc.byteSize =
      sizeof(scene_world.mesh_group[0]->UVs_stream[0]) *
      scene_world.mesh_group[0]->UVs_stream.size();
  vertexUVBufferDesc.isVertexBuffer = true;
  vertexUVBufferDesc.debugName = "UV Buffer";
  vertexUVBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
  uv_buffer = device->createBuffer(vertexUVBufferDesc);

  nvrhi::BufferDesc vertexNormalBufferDesc;
  vertexNormalBufferDesc.byteSize =
      sizeof(scene_world.mesh_group[0]->normals_stream[0]) *
      scene_world.mesh_group[0]->normals_stream.size();
  vertexNormalBufferDesc.isVertexBuffer = true;
  vertexNormalBufferDesc.debugName = "Normal Buffer";
  vertexNormalBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
  normal_buffer = device->createBuffer(vertexNormalBufferDesc);

  nvrhi::BufferDesc vertexBiNormalBufferDesc;
  vertexBiNormalBufferDesc.byteSize =
      sizeof(scene_world.mesh_group[0]->normals_stream[0]) *
      scene_world.mesh_group[0]->normals_stream.size();
  vertexBiNormalBufferDesc.isVertexBuffer = true;
  vertexBiNormalBufferDesc.debugName = "BiNormal Buffer";
  vertexBiNormalBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
  binormal_buffer = device->createBuffer(vertexBiNormalBufferDesc);

  nvrhi::BufferDesc vertexTangentBufferDesc;
  vertexTangentBufferDesc.byteSize =
      sizeof(scene_world.mesh_group[0]->tangents_stream[0]) *
      scene_world.mesh_group[0]->tangents_stream.size();
  vertexTangentBufferDesc.isVertexBuffer = true;
  vertexTangentBufferDesc.debugName = "Tangent Buffer";
  vertexTangentBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
  tangent_buffer = device->createBuffer(vertexTangentBufferDesc);

  nvrhi::BufferDesc indexBufferDesc;
  indexBufferDesc.byteSize = sizeof(scene_world.mesh_group[0]->indices[0]) *
                             scene_world.mesh_group[0]->indices.size();
  indexBufferDesc.isIndexBuffer = true;
  indexBufferDesc.debugName = "IndexBuffer";
  indexBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
  index_buffer = device->createBuffer(indexBufferDesc);

  auto layoutDesc = nvrhi::BindingLayoutDesc()
                        .setVisibility(nvrhi::ShaderType::All)
                        .addItem(nvrhi::BindingLayoutItem::ConstantBuffer(0))
                        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
                        .addItem(nvrhi::BindingLayoutItem::Sampler(0));

  binding_layout = device->createBindingLayout(layoutDesc);

  nvrhi::BufferDesc constantBufferDesc =
      nvrhi::BufferDesc()
          .setByteSize(sizeof(float) * 16) // stores one matrix
          .setIsConstantBuffer(true)
          .setIsVolatile(false)
          .setInitialState(nvrhi::ResourceStates::CopyDest)
          .setKeepInitialState(true);
  constant_buffer =
      device->createBuffer(constantBufferDesc); // only one float4x4 size

  auto &textureSampler =
      nvrhi::SamplerDesc().setAllFilters(true).setAllAddressModes(
          nvrhi::SamplerAddressMode::Repeat);

  nvrhi::SamplerHandle repeat_sampler = device->createSampler(textureSampler);
  auto bindingSetDesc =
      nvrhi::BindingSetDesc()
          .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, constant_buffer))
          .addItem(nvrhi::BindingSetItem::Texture_SRV(
              0, albedo_texute)) // texture at t0
          .addItem(nvrhi::BindingSetItem::Sampler(0, repeat_sampler));

  binding_set = device->createBindingSet(bindingSetDesc, binding_layout);
}

SimplePass::~SimplePass() {
  vs_shader = nullptr;
  ps_shader = nullptr;
  pipeline = nullptr;
  preZ_pipeline = nullptr;
  input_layout = nullptr;
  vertex_buffer = nullptr;
  uv_buffer = nullptr;
  normal_buffer = nullptr;
  tangent_buffer = nullptr;
  binormal_buffer = nullptr;
  index_buffer = nullptr;
  constant_buffer = nullptr;
  binding_layout = nullptr;
  binding_set = nullptr;
  preZ_framebuffer = nullptr;
  framebuffer = nullptr;
}

void SimplePass::UpdateRenderdata(FlyCamera camera) {
  nvrhi::CommandListHandle current_copy_commandlist =
      render_contex->get_current_command_list(
          CommandQueueFamily::TYPE_GRAPHICS);
  if (is_start_frame) {
    // Upload Buffer

    current_copy_commandlist->beginTrackingBufferState(
        vertex_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        vertex_buffer, scene_world.mesh_group[0]->positions_stream.data(),
        sizeof(glm::vec3) * scene_world.mesh_group[0]->positions_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        vertex_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        uv_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        uv_buffer, scene_world.mesh_group[0]->UVs_stream.data(),
        sizeof(glm::vec2) * scene_world.mesh_group[0]->UVs_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        uv_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        normal_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        normal_buffer, scene_world.mesh_group[0]->normals_stream.data(),
        sizeof(glm::vec2) * scene_world.mesh_group[0]->normals_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        normal_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        binormal_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        binormal_buffer, scene_world.mesh_group[0]->tangents_stream.data(),
        sizeof(glm::vec2) * scene_world.mesh_group[0]->tangents_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        binormal_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        tangent_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        tangent_buffer, scene_world.mesh_group[0]->tangents_stream.data(),
        sizeof(glm::vec2) * scene_world.mesh_group[0]->tangents_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        tangent_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        index_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        index_buffer, scene_world.mesh_group[0]->indices.data(),
        sizeof(scene_world.mesh_group[0]->indices[0]) *
            scene_world.mesh_group[0]->indices.size());
    current_copy_commandlist->setPermanentBufferState(
        index_buffer, nvrhi::ResourceStates::IndexBuffer);

    const size_t textureRowPitch = aldebo_desc.width * 4;
    current_copy_commandlist->writeTexture(albedo_texute,
                                           /* arraySlice = */ 0,
                                           /* mipLevel = */ 0, albedo_raw_data,
                                           textureRowPitch);
    is_start_frame = false;
  }
  if (is_pre_frame) {
    // TODO:
    glm::quat rotationX =
        glm::angleAxis(glm::radians(35.0f), glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 invViewProj = camera.view_proj * glm::mat4_cast(rotationX);
    current_copy_commandlist->writeBuffer(constant_buffer, &invViewProj,
                                          sizeof(float) * 16);
  }
}

void SimplePass::PreZ_Render(nvrhi::TextureHandle col_tex,
                             nvrhi::TextureHandle depth_tex) {
  nvrhi::CommandListHandle current_command_list_graphics =
      render_contex->get_current_command_list(
          CommandQueueFamily::TYPE_GRAPHICS);
  if (!preZ_pipeline) {
    nvrhi::RenderState render_state;
    render_state.depthStencilState.depthTestEnable = true;
    render_state.depthStencilState.depthFunc =
        nvrhi::ComparisonFunc::GreaterOrEqual;
    render_state.depthStencilState.depthWriteEnable = true;
    render_state.rasterState.depthClipEnable = true;
    render_state.rasterState.frontCounterClockwise = true;
    nvrhi::GraphicsPipelineDesc desc;
    desc.VS = vs_shader;
    // desc.PS = nullptr; // do not need PS Shader
    desc.primType = nvrhi::PrimitiveType::TriangleList;
    desc.renderState = render_state;
    desc.inputLayout = input_layout;
    desc.addBindingLayout(binding_layout);

    auto framebufferDesc =
        nvrhi::FramebufferDesc().addColorAttachment(col_tex).setDepthAttachment(
            depth_tex);
    preZ_framebuffer =
        render_contex->nvrhi_device->createFramebuffer(framebufferDesc);

    preZ_pipeline =
        render_contex->render_pipeline_create(desc, preZ_framebuffer);
  }
  nvrhi::GraphicsState state;
  state.pipeline = preZ_pipeline;
  state.framebuffer = preZ_framebuffer;
  state.vertexBuffers = {{vertex_buffer, 0, 0},
                         {uv_buffer, 1, 0},
                         {normal_buffer, 2, 0},
                         {tangent_buffer, 3, 0},
                         {binormal_buffer, 4, 0}};
  state.indexBuffer = {index_buffer, nvrhi::Format::R32_UINT, 0};
  state.addBindingSet(binding_set);
  state.viewport.addViewportAndScissorRect(
      preZ_framebuffer->getFramebufferInfo().getViewport());
  current_command_list_graphics->setGraphicsState(state);

  auto drawArguments = nvrhi::DrawArguments().setVertexCount(
      scene_world.mesh_group[0]->indices.size());
  current_command_list_graphics->drawIndexed(drawArguments);
}

void SimplePass::Base_Render(nvrhi::TextureHandle col_tex,
                             nvrhi::TextureHandle depth_tex) {
  nvrhi::CommandListHandle current_command_list_graphics =
      render_contex->get_current_command_list(
          CommandQueueFamily::TYPE_GRAPHICS);
  if (!pipeline) {
    nvrhi::RenderState render_state;
    render_state.depthStencilState.depthTestEnable = true;
    render_state.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Equal;
    render_state.depthStencilState.depthWriteEnable = false;
    render_state.rasterState.depthClipEnable = true;
    render_state.rasterState.frontCounterClockwise = true;
    nvrhi::GraphicsPipelineDesc desc;
    desc.VS = vs_shader;
    desc.PS = ps_shader;
    desc.primType = nvrhi::PrimitiveType::TriangleList;
    desc.renderState = render_state;
    desc.inputLayout = input_layout;
    desc.addBindingLayout(binding_layout);

    auto framebufferDesc =
        nvrhi::FramebufferDesc().addColorAttachment(col_tex).setDepthAttachment(
            depth_tex);
    framebuffer =
        render_contex->nvrhi_device->createFramebuffer(framebufferDesc);

    pipeline = render_contex->render_pipeline_create(desc, framebuffer);
  }
  nvrhi::GraphicsState state;
  state.pipeline = pipeline;
  state.framebuffer = framebuffer;
  state.vertexBuffers = {{vertex_buffer, 0, 0},
                         {uv_buffer, 1, 0},
                         {normal_buffer, 2, 0},
                         {tangent_buffer, 3, 0},
                         {binormal_buffer, 4, 0}};
  state.indexBuffer = {index_buffer, nvrhi::Format::R32_UINT, 0};
  state.addBindingSet(binding_set);
  state.viewport.addViewportAndScissorRect(
      framebuffer->getFramebufferInfo().getViewport());
  current_command_list_graphics->setGraphicsState(state);

  auto drawArguments = nvrhi::DrawArguments().setVertexCount(
      scene_world.mesh_group[0]->indices.size());
  current_command_list_graphics->drawIndexed(drawArguments);
}

void SimplePass::Render(nvrhi::TextureHandle col_tex,
                        nvrhi::TextureHandle depth_tex) {
  PreZ_Render(col_tex, depth_tex);
  Base_Render(col_tex, depth_tex);
}

void SimplePass::Resize(nvrhi::TextureHandle col_tex,
                        nvrhi::TextureHandle depth_tex) {
  framebuffer = nullptr;
  nvrhi::DeviceHandle devicePtr = render_contex->nvrhi_device;
  auto framebufferDesc =
      nvrhi::FramebufferDesc().addColorAttachment(col_tex).setDepthAttachment(
          depth_tex);
  framebuffer = devicePtr->createFramebuffer(framebufferDesc);
}

} // namespace Yoda