#include "simple_pass.h"
#include "core/fly_camera.h"
#include "core/shadercode.h"
#include "glm/trigonometric.hpp"
#include "nvrhi/nvrhi.h"
#include "render/world.h"
#include "rhi/d3d12/rhi_context_d3d12.h"
#include "rhi/rhi_context_commons.h"
#include <cmath>
#include <glm/gtc/quaternion.hpp>
#include <nvrhi/utils.h>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Yoda {
SimplePass::SimplePass(std::shared_ptr<RHIContextD3D12> context, World *wolrd) {
  render_contex = context;
  scene_world = wolrd;
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
  int x, y, n;
  albedo_raw_data = stbi_load("textures/gray.png", &x, &y, &n, 4);
  // albedo_raw_data = stbi_load("textures/lemon/lemon_diff_4k.tga", &x, &y, &n,
  // 4);
  int nx, ny, nz;
  normal_raw_data =
      stbi_load("textures/lemon/lemon_nor_dx_4k.tga", &nx, &ny, &nz, 4);
  int rx, ry, rz;
  RMO_raw_data = stbi_load("textures/lemon/lemon_RMO_4k.tga", &rx, &ry, &rz, 4);

  int ix, iy, in;
  irradiance_raw_data.resize(6);
  irradiance_raw_data[0] =
      stbi_load("textures/IBL/irradiance/posX.tga", &ix, &iy, &in, 4);
  irradiance_raw_data[1] =
      stbi_load("textures/IBL/irradiance/negX.tga", &ix, &iy, &in, 4);
  irradiance_raw_data[2] =
      stbi_load("textures/IBL/irradiance/posY.tga", &ix, &iy, &in, 4);
  irradiance_raw_data[3] =
      stbi_load("textures/IBL/irradiance/negY.tga", &ix, &iy, &in, 4);
  irradiance_raw_data[4] =
      stbi_load("textures/IBL/irradiance/posZ.tga", &ix, &iy, &in, 4);
  irradiance_raw_data[5] =
      stbi_load("textures/IBL/irradiance/negZ.tga", &ix, &iy, &in, 4);

  specular_raw_data.resize(6);
  specular_raw_data[0] =
      stbi_load("textures/IBL/specular/posX.tga", &ix, &iy, &in, 4);
  specular_raw_data[1] =
      stbi_load("textures/IBL/specular/negX.tga", &ix, &iy, &in, 4);
  specular_raw_data[2] =
      stbi_load("textures/IBL/specular/posY.tga", &ix, &iy, &in, 4);
  specular_raw_data[3] =
      stbi_load("textures/IBL/specular/negY.tga", &ix, &iy, &in, 4);
  specular_raw_data[4] =
      stbi_load("textures/IBL/specular/posZ.tga", &ix, &iy, &in, 4);
  specular_raw_data[5] =
      stbi_load("textures/IBL/specular/negZ.tga", &ix, &iy, &in, 4);

  int bx, by, bz;
  brdfIntegration_raw_data =
      stbi_load("textures/IBL/brdf.png", &bx, &by, &bz, 4);

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
                    .setFormat(nvrhi::Format::SRGBA8_UNORM)
                    .setInitialState(nvrhi::ResourceStates::ShaderResource)
                    .setKeepInitialState(true)
                    .setDebugName("Albedo Texture");
  albedo_texture = device->createTexture(aldebo_desc);

  normal_desc = nvrhi::TextureDesc()
                    .setDimension(nvrhi::TextureDimension::Texture2D)
                    .setWidth(nx)
                    .setHeight(ny)
                    .setFormat(nvrhi::Format::RGBA8_UNORM)
                    .setInitialState(nvrhi::ResourceStates::ShaderResource)
                    .setKeepInitialState(true)
                    .setDebugName("Normal Texture");
  normal_texture = device->createTexture(normal_desc);

  RMO_desc = nvrhi::TextureDesc()
                 .setDimension(nvrhi::TextureDimension::Texture2D)
                 .setWidth(rx)
                 .setHeight(ry)
                 .setFormat(nvrhi::Format::RGBA8_UNORM)
                 .setInitialState(nvrhi::ResourceStates::ShaderResource)
                 .setKeepInitialState(true)
                 .setDebugName("RMO Texture");
  RMO_texture = device->createTexture(RMO_desc);

//   irradiance_desc = nvrhi::TextureDesc()
//                         .setArraySize(6)
//                         .setDimension(nvrhi::TextureDimension::Texture2DArray)
//                         .setWidth(32)
//                         .setHeight(32)
//                         .setFormat(nvrhi::Format::RGBA8_UNORM)
//                         .setInitialState(nvrhi::ResourceStates::ShaderResource)
//                         .setKeepInitialState(true)
//                         .setDebugName("irradianceMap");
//   irradiance_texture = device->createTexture(irradiance_desc);

  irradiance_desc = nvrhi::TextureDesc()
                        .setArraySize(6)
                        .setDimension(nvrhi::TextureDimension::TextureCube)
                        .setWidth(32)
                        .setHeight(32)
                        .setFormat(nvrhi::Format::RGBA8_UNORM)
                        .setInitialState(nvrhi::ResourceStates::ShaderResource)
                        .setKeepInitialState(true)
                        .setDebugName("irradianceEnvMap");
  irradiance_texture = device->createTexture(irradiance_desc);

  specular_desc = nvrhi::TextureDesc()
                      .setArraySize(6)
                      .setDimension(nvrhi::TextureDimension::TextureCube)
                      .setWidth(128)
                      .setHeight(128)
                      .setFormat(nvrhi::Format::RGBA8_UNORM)
                      .setInitialState(nvrhi::ResourceStates::ShaderResource)
                      .setKeepInitialState(true)
                      .setDebugName("specularEnvMap");
  specular_texture = device->createTexture(specular_desc);

  brdfIntegration_desc =
      nvrhi::TextureDesc()
          .setDimension(nvrhi::TextureDimension::Texture2D)
          .setWidth(bx)
          .setHeight(by)
          .setFormat(nvrhi::Format::RGBA8_UNORM)
          .setInitialState(nvrhi::ResourceStates::ShaderResource)
          .setKeepInitialState(true)
          .setDebugName("BRDF Texture");
  brdfIntegration_texture = device->createTexture(brdfIntegration_desc);

  // Create Vertex Buffer
  nvrhi::BufferDesc vertexBufferDesc;
  vertexBufferDesc.byteSize =
      sizeof(scene_world->mesh_group[0]->positions_stream[0]) *
      scene_world->mesh_group[0]->positions_stream.size();
  vertexBufferDesc.isVertexBuffer = true;
  vertexBufferDesc.debugName = "VertexBuffer";
  vertexBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
  vertex_buffer = device->createBuffer(vertexBufferDesc);

  nvrhi::BufferDesc vertexUVBufferDesc;
  vertexUVBufferDesc.byteSize =
      sizeof(scene_world->mesh_group[0]->UVs_stream[0]) *
      scene_world->mesh_group[0]->UVs_stream.size();
  vertexUVBufferDesc.isVertexBuffer = true;
  vertexUVBufferDesc.debugName = "UV Buffer";
  vertexUVBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
  uv_buffer = device->createBuffer(vertexUVBufferDesc);

  nvrhi::BufferDesc vertexNormalBufferDesc;
  vertexNormalBufferDesc.byteSize =
      sizeof(scene_world->mesh_group[0]->normals_stream[0]) *
      scene_world->mesh_group[0]->normals_stream.size();
  vertexNormalBufferDesc.isVertexBuffer = true;
  vertexNormalBufferDesc.debugName = "Normal Buffer";
  vertexNormalBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
  normal_buffer = device->createBuffer(vertexNormalBufferDesc);

  nvrhi::BufferDesc vertexBiNormalBufferDesc;
  vertexBiNormalBufferDesc.byteSize =
      sizeof(scene_world->mesh_group[0]->normals_stream[0]) *
      scene_world->mesh_group[0]->normals_stream.size();
  vertexBiNormalBufferDesc.isVertexBuffer = true;
  vertexBiNormalBufferDesc.debugName = "BiNormal Buffer";
  vertexBiNormalBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
  binormal_buffer = device->createBuffer(vertexBiNormalBufferDesc);

  nvrhi::BufferDesc vertexTangentBufferDesc;
  vertexTangentBufferDesc.byteSize =
      sizeof(scene_world->mesh_group[0]->tangents_stream[0]) *
      scene_world->mesh_group[0]->tangents_stream.size();
  vertexTangentBufferDesc.isVertexBuffer = true;
  vertexTangentBufferDesc.debugName = "Tangent Buffer";
  vertexTangentBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
  tangent_buffer = device->createBuffer(vertexTangentBufferDesc);

  nvrhi::BufferDesc indexBufferDesc;
  indexBufferDesc.byteSize = sizeof(scene_world->mesh_group[0]->indices[0]) *
                             scene_world->mesh_group[0]->indices.size();
  indexBufferDesc.isIndexBuffer = true;
  indexBufferDesc.debugName = "IndexBuffer";
  indexBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
  index_buffer = device->createBuffer(indexBufferDesc);

  auto layoutDesc = nvrhi::BindingLayoutDesc()
                        .setVisibility(nvrhi::ShaderType::All)
                        .addItem(nvrhi::BindingLayoutItem::ConstantBuffer(0))
                        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
                        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(1))
                        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(2))
                        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(3))
                        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(4))
                        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(5))
                        .addItem(nvrhi::BindingLayoutItem::Sampler(0))
                        .addItem(nvrhi::BindingLayoutItem::Sampler(1));

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
  auto &envTextureSampler =
      nvrhi::SamplerDesc().setAllFilters(true).setAllAddressModes(
          nvrhi::SamplerAddressMode::Wrap);
  nvrhi::SamplerHandle repeat_sampler = device->createSampler(textureSampler);
  nvrhi::SamplerHandle warp_sampler = device->createSampler(envTextureSampler);
 
  auto bindingSetDesc =
      nvrhi::BindingSetDesc()
          .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, constant_buffer))
          .addItem(nvrhi::BindingSetItem::Texture_SRV(
              0, albedo_texture)) // texture at t0
          .addItem(nvrhi::BindingSetItem::Texture_SRV(1, normal_texture))
          .addItem(nvrhi::BindingSetItem::Texture_SRV(2, RMO_texture))
          .addItem(nvrhi::BindingSetItem::Texture_SRV(3, irradiance_texture))
          .addItem(nvrhi::BindingSetItem::Texture_SRV(4, specular_texture))
          .addItem(nvrhi::BindingSetItem::Texture_SRV(5, brdfIntegration_texture))
          .addItem(nvrhi::BindingSetItem::Sampler(0, repeat_sampler))
          .addItem(nvrhi::BindingSetItem::Sampler(1, warp_sampler));

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
        vertex_buffer, scene_world->mesh_group[0]->positions_stream.data(),
        sizeof(glm::vec3) *
            scene_world->mesh_group[0]->positions_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        vertex_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        uv_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        uv_buffer, scene_world->mesh_group[0]->UVs_stream.data(),
        sizeof(glm::vec2) * scene_world->mesh_group[0]->UVs_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        uv_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        normal_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        normal_buffer, scene_world->mesh_group[0]->normals_stream.data(),
        sizeof(glm::vec3) * scene_world->mesh_group[0]->normals_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        normal_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        binormal_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        binormal_buffer, scene_world->mesh_group[0]->tangents_stream.data(),
        sizeof(glm::vec3) * scene_world->mesh_group[0]->tangents_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        binormal_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        tangent_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        tangent_buffer, scene_world->mesh_group[0]->tangents_stream.data(),
        sizeof(glm::vec3) * scene_world->mesh_group[0]->tangents_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        tangent_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        index_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        index_buffer, scene_world->mesh_group[0]->indices.data(),
        sizeof(scene_world->mesh_group[0]->indices[0]) *
            scene_world->mesh_group[0]->indices.size());
    current_copy_commandlist->setPermanentBufferState(
        index_buffer, nvrhi::ResourceStates::IndexBuffer);

    size_t textureRowPitch = aldebo_desc.width * 4;
    current_copy_commandlist->writeTexture(albedo_texture,
                                           /* arraySlice = */ 0,
                                           /* mipLevel = */ 0, albedo_raw_data,
                                           textureRowPitch);

    textureRowPitch = normal_desc.width * 4;
    current_copy_commandlist->writeTexture(normal_texture,
                                           /* arraySlice = */ 0,
                                           /* mipLevel = */ 0, normal_raw_data,
                                           textureRowPitch);

    textureRowPitch = RMO_desc.width * 4;
    current_copy_commandlist->writeTexture(RMO_texture,
                                           /* arraySlice = */ 0,
                                           /* mipLevel = */ 0, RMO_raw_data,
                                           textureRowPitch);

    textureRowPitch = irradiance_desc.width * 4;
    for (size_t i = 0; i < 6; ++i) {
      current_copy_commandlist->writeTexture(
          irradiance_texture,
          /* arraySlice = */ i,
          /* mipLevel = */ 0, irradiance_raw_data[i], textureRowPitch);
    }

    textureRowPitch = specular_desc.width * 4;
    for (size_t i = 0; i < 6; ++i) {
      current_copy_commandlist->writeTexture(
          specular_texture,
          /* arraySlice = */ i,
          /* mipLevel = */ 0, specular_raw_data[i], textureRowPitch);
    }

    textureRowPitch = brdfIntegration_desc.width * 4;
    current_copy_commandlist->writeTexture(
        brdfIntegration_texture,
        /* arraySlice = */ 0,
        /* mipLevel = */ 0, brdfIntegration_raw_data, textureRowPitch);

    is_start_frame = false;
  }
  if (is_pre_frame) {
    // TODO:
    glm::quat rotationX =
        glm::angleAxis(glm::radians(35.0f), glm::vec3(0.0, 1.0, 0.0));
    // glm::mat4 invViewProj = camera.view_proj * glm::mat4_cast(rotationX);
    glm::mat4 invViewProj = camera.view_proj;
    glm::mat4 world = glm::mat4(1.0);
    glm::vec4 camera_pos =
        glm::vec4(glm::vec3(camera.eye - camera.center), 1.0);
    glm::vec4 lightDir = glm::vec4(-0.6, 0.25, 0.25, 0.0);
    struct CameraConstBufferBlock const_buffer_block {
      world, invViewProj, camera_pos, lightDir
    };
    current_copy_commandlist->writeBuffer(constant_buffer, &const_buffer_block,
                                          sizeof(const_buffer_block));
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
    // render_state.rasterState.frontCounterClockwise = true;
    render_state.rasterState.cullMode = nvrhi::RasterCullMode::Back;
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
  nvrhi::utils::ClearColorAttachment(current_command_list_graphics,
                                     preZ_framebuffer, 0, nvrhi::Color(0.0f));
  nvrhi::utils::ClearDepthStencilAttachment(current_command_list_graphics,
                                            preZ_framebuffer, 0.0, 0.0);

  auto drawArguments = nvrhi::DrawArguments().setVertexCount(
      scene_world->mesh_group[0]->indices.size());
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
    render_state.depthStencilState.depthWriteEnable = true;
    render_state.rasterState.depthClipEnable = true;
    // render_state.rasterState.frontCounterClockwise = true;
    render_state.rasterState.cullMode = nvrhi::RasterCullMode::Back;
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
      scene_world->mesh_group[0]->indices.size());
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
  preZ_framebuffer = nullptr;
  nvrhi::DeviceHandle devicePtr = render_contex->nvrhi_device;
  auto framebufferDesc =
      nvrhi::FramebufferDesc().addColorAttachment(col_tex).setDepthAttachment(
          depth_tex);
  auto desc = col_tex->getDesc();
  framebuffer = devicePtr->createFramebuffer(framebufferDesc);
  preZ_framebuffer = devicePtr->createFramebuffer(framebufferDesc);
}

} // namespace Yoda