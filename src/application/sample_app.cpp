#include "application/sample_app.h"
#include "core/fly_camera.h"
#include "core/logger.h"
#include "core/shadercode.h"
#include "render/mesh.h"
#include "render/world.h"
#include "rhi/d3d12/rhi_context_d3d12.h"
#include <fstream>
#include <imgui.h>
#include <memory>
#include <nvrhi/d3d12.h>
#include <nvrhi/utils.h>
#include <string>

#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Yoda {
SampleApp::SampleApp(const SampleAppConfig &config) {
  mShowUI = config.showUI;
  mVsyncOn = config.windowDesc.enableVSync;
  if (!config.headless) {
    auto windowDesc = config.windowDesc;
    // Create the window
    m_window = Window::create(windowDesc, this);
    // m_window->setWindowIcon(getRuntimeDirectory() /
    // "data/framework/nvidia.ico");
    m_render_context = std::make_shared<RHIContextD3D12>();
    m_render_context->initialize(m_window->getApiHandle());

    ShaderByteCode vs_byte_code;
    std::string path = "shaders/basic_triangle.hlsl";
    std::string entry_point = "main_vs";
    vs_byte_code.compile_shader(nvrhi::ShaderType::Vertex, path, entry_point);

    ShaderByteCode ps_byte_code;
    entry_point = "main_ps";
    ps_byte_code.compile_shader(nvrhi::ShaderType::Pixel, path, entry_point);
    vs_shader = m_render_context->shader_create_from_bytecode(
        vs_byte_code.m_type, vs_byte_code.m_byte_code);
    ps_shader = m_render_context->shader_create_from_bytecode(
        ps_byte_code.m_type, ps_byte_code.m_byte_code);

    std::string test_scene_path = "module/sphere_units.fbx";
    bool state = Mesh::load_scene(test_scene_path, scene_world);

    int x, y, n;
    unsigned char *data = stbi_load("textures/test.png", &x, &y, &n, 4);

    fcamera =
        FlyCamera{glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                  m_render_context->get_swapchain_info()};
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
        m_render_context
            ->get_current_command_list(CommandQueueFamily::TYPE_GRAPHICS)
            ->getDevice();
    input_layout = device->createInputLayout(
        attributes, uint32_t(std::size(attributes)), vs_shader);

    auto textureDesc =
        nvrhi::TextureDesc()
            .setDimension(nvrhi::TextureDimension::Texture2D)
            .setWidth(x)
            .setHeight(y)
            .setFormat(nvrhi::Format::RGBA8_UNORM)
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setDebugName("Albedo Texture");
    nvrhi::TextureHandle albedo_texute = device->createTexture(textureDesc);

    // Create Vertex Buffer
    nvrhi::BufferDesc vertexBufferDesc;
    vertexBufferDesc.byteSize =
        sizeof(scene_world.mesh_group[0].positions_stream[0]) *
        scene_world.mesh_group[0].positions_stream.size();
    vertexBufferDesc.isVertexBuffer = true;
    vertexBufferDesc.debugName = "VertexBuffer";
    vertexBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
    vertex_buffer = device->createBuffer(vertexBufferDesc);

    nvrhi::BufferDesc vertexUVBufferDesc;
    vertexUVBufferDesc.byteSize =
        sizeof(scene_world.mesh_group[0].UVs_stream[0]) *
        scene_world.mesh_group[0].UVs_stream.size();
    vertexUVBufferDesc.isVertexBuffer = true;
    vertexUVBufferDesc.debugName = "UV Buffer";
    vertexUVBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
    uv_buffer = device->createBuffer(vertexUVBufferDesc);

    nvrhi::BufferDesc vertexNormalBufferDesc;
    vertexNormalBufferDesc.byteSize =
        sizeof(scene_world.mesh_group[0].normals_stream[0]) *
        scene_world.mesh_group[0].normals_stream.size();
    vertexNormalBufferDesc.isVertexBuffer = true;
    vertexNormalBufferDesc.debugName = "Normal Buffer";
    vertexNormalBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
    normal_buffer = device->createBuffer(vertexNormalBufferDesc);

    nvrhi::BufferDesc vertexBiNormalBufferDesc;
    vertexBiNormalBufferDesc.byteSize =
        sizeof(scene_world.mesh_group[0].normals_stream[0]) *
        scene_world.mesh_group[0].normals_stream.size();
    vertexBiNormalBufferDesc.isVertexBuffer = true;
    vertexBiNormalBufferDesc.debugName = "BiNormal Buffer";
    vertexBiNormalBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
    binormal_buffer = device->createBuffer(vertexBiNormalBufferDesc);

    nvrhi::BufferDesc vertexTangentBufferDesc;
    vertexTangentBufferDesc.byteSize =
        sizeof(scene_world.mesh_group[0].tangents_stream[0]) *
        scene_world.mesh_group[0].tangents_stream.size();
    vertexTangentBufferDesc.isVertexBuffer = true;
    vertexTangentBufferDesc.debugName = "Tangent Buffer";
    vertexTangentBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
    tangent_buffer = device->createBuffer(vertexTangentBufferDesc);

    nvrhi::BufferDesc indexBufferDesc;
    indexBufferDesc.byteSize = sizeof(scene_world.mesh_group[0].indices[0]) *
                               scene_world.mesh_group[0].indices.size();
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
    // Upload Buffer
    nvrhi::CommandListHandle current_copy_commandlist =
        m_render_context->get_current_command_list(
            CommandQueueFamily::TYPE_GRAPHICS);
    current_copy_commandlist->open();
    current_copy_commandlist->beginTrackingBufferState(
        vertex_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        vertex_buffer, scene_world.mesh_group[0].positions_stream.data(),
        sizeof(glm::vec3) * scene_world.mesh_group[0].positions_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        vertex_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        uv_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        uv_buffer, scene_world.mesh_group[0].UVs_stream.data(),
        sizeof(glm::vec2) * scene_world.mesh_group[0].UVs_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        uv_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        normal_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        normal_buffer, scene_world.mesh_group[0].normals_stream.data(),
        sizeof(glm::vec2) * scene_world.mesh_group[0].normals_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        normal_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        binormal_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        binormal_buffer, scene_world.mesh_group[0].tangents_stream.data(),
        sizeof(glm::vec2) * scene_world.mesh_group[0].tangents_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        binormal_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        tangent_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        tangent_buffer, scene_world.mesh_group[0].tangents_stream.data(),
        sizeof(glm::vec2) * scene_world.mesh_group[0].tangents_stream.size());
    current_copy_commandlist->setPermanentBufferState(
        tangent_buffer, nvrhi::ResourceStates::VertexBuffer);

    current_copy_commandlist->beginTrackingBufferState(
        index_buffer, nvrhi::ResourceStates::CopyDest);
    current_copy_commandlist->writeBuffer(
        index_buffer, scene_world.mesh_group[0].indices.data(),
        sizeof(scene_world.mesh_group[0].indices[0]) *
            scene_world.mesh_group[0].indices.size());
    current_copy_commandlist->setPermanentBufferState(
        index_buffer, nvrhi::ResourceStates::IndexBuffer);

    // glm::mat4 invViewProj = glm::transpose(fcamera.view_proj);
    //// invViewProj = (fcamera.view_proj);
    // current_copy_commandlist->beginTrackingBufferState(
    //     constant_buffer, nvrhi::ResourceStates::CopyDest);
    // current_copy_commandlist->writeBuffer(constant_buffer, &invViewProj,
    //                                       sizeof(float) * 16);
    // current_copy_commandlist->setPermanentBufferState(
    //     constant_buffer, nvrhi::ResourceStates::ConstantBuffer);

    const size_t textureRowPitch = x * 4;
    current_copy_commandlist->writeTexture(
        albedo_texute,
        /* arraySlice = */ 0, /* mipLevel = */ 0, data, textureRowPitch);

    current_copy_commandlist->close();
    m_render_context->excute_command_list(current_copy_commandlist);

    // pass A
    nvrhi::TextureDesc texture_desc;
    texture_desc.width = m_render_context->get_swapchain_info().width;
    texture_desc.height = m_render_context->get_swapchain_info().height;
    texture_desc.format = nvrhi::Format::RGBA8_UNORM;
    texture_desc.debugName = "Pass A Color Buffer";
    texture_desc.isRenderTarget = true;
    texture_desc.isUAV = false;
    texture_desc.initialState = nvrhi::ResourceStates::RenderTarget;
    texture_desc.keepInitialState = true;
    color_buffer = m_render_context->texture_create(texture_desc);

    nvrhi::TextureDesc depth_texture_desc = texture_desc;
    depth_texture_desc.format = nvrhi::Format::D32;
    depth_texture_desc.initialState = nvrhi::ResourceStates::DepthWrite;
    depth_texture_desc.clearValue = nvrhi::Color(0.0f);
    depth_texture_desc.debugName = "Pass A Depth Buffer";
    depth_buffer = m_render_context->texture_create(depth_texture_desc);

    nvrhi::DeviceHandle devicePtr = m_render_context->nvrhi_device;
    auto framebufferDesc = nvrhi::FramebufferDesc()
                               .addColorAttachment(color_buffer)
                               .setDepthAttachment(depth_buffer);
    test_framebuffer = devicePtr->createFramebuffer(framebufferDesc);
  }
}

SampleApp::~SampleApp() {
  m_window.reset();
  pipeline = nullptr;
  vs_shader = nullptr;
  ps_shader = nullptr;

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

  m_render_context.reset();
}
int SampleApp::run() {
  if (m_window) {
    m_window->msgLoop();
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

  auto const previousTime = current_time;
  auto wallTime = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch());
  current_time = static_cast<double>(wallTime.count()) / 1000000.0;
  frame_time = current_time - previousTime;

  // Render something
  m_render_context->begin_frame();
  nvrhi::IFramebuffer *framebuffer =
      m_render_context->get_current_frame_buffer();

  framebuffer = test_framebuffer;
  // -----------------------------------------------------------
  nvrhi::CommandListHandle current_command_list_graphics =
      m_render_context->get_current_command_list(
          CommandQueueFamily::TYPE_GRAPHICS);
  if (!pipeline) {
    nvrhi::RenderState render_state;
    render_state.depthStencilState.enableDepthTest();
    render_state.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Always;
    render_state.depthStencilState.enableDepthTest();

    nvrhi::GraphicsPipelineDesc desc;
    desc.VS = vs_shader;
    desc.PS = ps_shader;
    desc.primType = nvrhi::PrimitiveType::TriangleList;
    desc.renderState = render_state;
    desc.inputLayout = input_layout;
    desc.addBindingLayout(binding_layout);

    pipeline = m_render_context->render_pipeline_create(desc, framebuffer);
  }
  current_command_list_graphics->open();
  nvrhi::utils::ClearColorAttachment(current_command_list_graphics, framebuffer,
                                     0, nvrhi::Color(0.0f));
  nvrhi::utils::ClearDepthStencilAttachment(current_command_list_graphics,
                                            framebuffer, 0.0, 0.0);
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

  glm::mat4 invViewProj = (fcamera.view_proj);
  // invViewProj = (fcamera.proj * fcamera.view);
  current_command_list_graphics->writeBuffer(constant_buffer, &invViewProj,
                                             sizeof(float) * 16);
  auto drawArguments = nvrhi::DrawArguments().setVertexCount(
      scene_world.mesh_group[0].indices.size());
  current_command_list_graphics->drawIndexed(drawArguments);

  nvrhi::TextureSlice dst_slice;
  nvrhi::TextureSlice src_slice;

  current_command_list_graphics->copyTexture(
      m_render_context->get_swapchain_back_buffer(), nvrhi::TextureSlice(),
      color_buffer, nvrhi::TextureSlice());
  current_command_list_graphics->close();
  m_render_context->excute_command_list(current_command_list_graphics);
  m_render_context->present(0);
}

void SampleApp::handleWindowSizeChange() {
  HWND window_handle = m_window->getApiHandle();
  RECT client_rect;
  GetClientRect(window_handle, &client_rect);
  UINT width = client_rect.right - client_rect.left;
  UINT height = client_rect.bottom - client_rect.top;
  m_render_context->resize_swapchain(width, height);
}

void SampleApp::handleRenderFrame() { renderFrame(); }

void SampleApp::handleKeyboardEvent(int key, int action) {

  if (action != GLFW_RELEASE) {
    auto frame_time_min = glm::min(static_cast<float>(frame_time), 0.05f);
    glm::vec3 const forward = normalize(fcamera.center - fcamera.eye);
    glm::vec3 const right = cross(forward, fcamera.up);
    glm::vec3 const up = cross(right, forward);
    glm::vec3 acceleration = camera_translation * -30.0f;
    float const force = camera_speed * 10000.0f;
    switch (key) {
    case GLFW_KEY_W:
      camera_move_dirty = true;
      acceleration.z += force;
      break;
    case GLFW_KEY_S:
      camera_move_dirty = true;
      acceleration.z -= force;
      break;
    case GLFW_KEY_A:
      camera_move_dirty = true;
      acceleration.x -= force;
      break;
    case GLFW_KEY_D:
      camera_move_dirty = true;
      acceleration.x += force;
      break;
    case GLFW_KEY_Q:
      camera_move_dirty = true;
      acceleration.y += force;
      break;
    case GLFW_KEY_E:
      camera_move_dirty = true;
      acceleration.y -= force;
      break;
    }
    if (camera_move_dirty) {
      camera_translation += acceleration * 0.5f * (float)frame_time_min;
      camera_translation =
          glm::clamp(camera_translation, -camera_speed, camera_speed);
      // Clamp tiny values to zero to improve convergence to resting state
      auto const clampMin =
          glm::lessThan(glm::abs(camera_translation), glm::vec3(0.0000001f));
      if (glm::any(clampMin)) {
        if (clampMin.x) {
          camera_translation.x = 0.0f;
        }
        if (clampMin.y) {
          camera_translation.y = 0.0f;
        }
        if (clampMin.z) {
          camera_translation.z = 0.0f;
        }
      }
      glm::vec3 translation = camera_translation;
      fcamera.eye +=
          translation.x * right + translation.y * up + translation.z * forward;
      fcamera.center +=
          translation.x * right + translation.y * up + translation.z * forward;
      SwapChainInfo info = m_render_context->get_swapchain_info();
      fcamera.UpdateCamera(info);
      camera_move_dirty = false;
    }
  }
}

void SampleApp::handleMouseEvent(int action, double x, double y) {
  if (action != -2) {
    auto frame_time_min = glm::min(static_cast<float>(frame_time), 0.05f);
    glm::vec3 const forward = normalize(fcamera.center - fcamera.eye);
    glm::vec3 const right = cross(forward, fcamera.up);
    glm::vec3 const up = cross(right, forward);
    float const force2 = 0.15f;
    glm::vec2 newXY = glm::vec2(x, y);
    newXY = newXY - mouse_pos;

    glm::vec2 acceleration = camera_rotation * -45.0f;
    acceleration.x -= force2 * newXY.x;
    acceleration.y += force2 * newXY.y;
    camera_rotation += acceleration * 0.5f * frame_time_min;
    camera_rotation = glm::clamp(camera_rotation, -4e-2f, 4e-2f);
    // Clamp tiny values to zero to improve convergence to resting state
    auto const clampRotationMin =
        glm::lessThan(glm::abs(camera_rotation), glm::vec2(0.00000001f));
    if (glm::any(clampRotationMin)) {
      if (clampRotationMin.x) {
        camera_rotation.x = 0.0f;
      }
      if (clampRotationMin.y) {
        camera_rotation.y = 0.0f;
      }
    }
    glm::quat rotationX = glm::angleAxis(-camera_rotation.x, up);
    glm::quat rotationY = glm::angleAxis(camera_rotation.y, right);

    const glm::vec3 newForward = normalize(forward * rotationX * rotationY);
    if (abs(dot(newForward, glm::vec3(0.0f, 1.0f, 0.0f))) < 0.9f) {
      // Prevent view and up direction becoming parallel (this uses a FPS style
      // camera)
      fcamera.center = fcamera.eye + newForward;
      const glm::vec3 newRight =
          normalize(cross(newForward, glm::vec3(0.0f, 1.0f, 0.0f)));
      fcamera.up = normalize(cross(newRight, newForward));
      SwapChainInfo info = m_render_context->get_swapchain_info();
      fcamera.UpdateCamera(info);
      mouse_pos = glm::vec2(x, y);
    }
  } else {
    mouse_pos = glm::vec2(x, y);
  }
}
} // namespace Yoda
