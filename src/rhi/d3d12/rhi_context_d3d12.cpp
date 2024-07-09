#pragma once
#include "rhi/d3d12/rhi_context_d3d12.h"
#include "core/logger.h"
#include "nvrhi/nvrhi.h"
#include "rhi/rhi_context_commons.h"
#include <dxgi1_5.h>
#include <synchapi.h>
#include <vcruntime.h>
#include <windef.h>
#include <winerror.h>
#include <winnt.h>
#include <wrl/client.h>

namespace Yoda {

RHIContextD3D12::RHIContextD3D12() {
  if (lib_d3d12 != nullptr || lib_dxgi != nullptr)
    return;
  lib_d3d12 = LoadLibraryW(L"D3D12.dll");
  lib_dxgi = LoadLibraryW(L"DXGI.dll");
}
RHIContextD3D12::~RHIContextD3D12() {

  // Commandlist
  graphics_commandlist_pool.clear();
  compute_commandlist_pool.clear();
  copy_commandlist_pool.clear();
  // Queue
  graphics_queue = nullptr;
  compute_queue = nullptr;
  copy_queue = nullptr;

  // Swapchain
  swapchain_frame_buffers.clear();
  release_render_targets();
  rhi_swapchain_buffers.clear();
  swapchain = nullptr;
  frame_fence = nullptr;
  frame_fence_events.clear();

  // Device
  nvrhi_device = nullptr;
  device = nullptr;

  // Dynamic Library(DLL)
  if (lib_d3d12) {
    FreeLibrary(lib_d3d12);
  }
  if (lib_dxgi) {
    FreeLibrary(lib_dxgi);
  }
}

size_t RHIContextD3D12::initialize(HWND handle) {
  size_t status = 0;
  status = init_device_factory();
  if (status != 0) {
    LOG_ERROR("device factory create error !!!");
    return -1;
  }
#ifdef DEBUG
  status = init_debug_layers();
  if (status != 0) {
    LOG_ERROR("debug layer create error !!!");
    return -1;
  }
#endif
  status = init_devices();
  if (status != 0) {
    LOG_ERROR("d3d device create error !!!");
    return -1;
  }

  graphics_queue = command_queue_create(CommandQueueFamily::TYPE_GRAPHICS,
                                        L"Graphics Queue");
  compute_queue =
      command_queue_create(CommandQueueFamily::TYPE_COMPUTE, L"Compute Queue");
  copy_queue =
      command_queue_create(CommandQueueFamily::TYPE_COPY, L"Copy Queue");

  nvrhi::d3d12::DeviceDesc deviceDesc;
  deviceDesc.errorCB = &DefaultMessageCallback::GetInstance();
  deviceDesc.pDevice = device.Get();
  deviceDesc.pGraphicsCommandQueue = graphics_queue.Get();
  deviceDesc.pComputeCommandQueue = compute_queue.Get();
  deviceDesc.pCopyCommandQueue = copy_queue.Get();

  nvrhi_device = nvrhi::d3d12::createDevice(deviceDesc);
#ifdef DEBUG
  nvrhi_device = nvrhi::validation::createValidationLayer(nvrhi_device);
#endif
  swapchain = swapchain_create(handle);
  create_render_targets();
  frame_fence = fence_create();
  for (size_t i = 0; i < MAX_FRAMES_COUNT; ++i) {
    frame_fence_events.emplace_back(CreateEvent(nullptr, false, true, nullptr));
    nvrhi::CommandListHandle graphics_cmd_list =
        commandlist_create(CommandQueueFamily::TYPE_GRAPHICS);
    nvrhi::CommandListHandle compute_cmd_list =
        commandlist_create(CommandQueueFamily::TYPE_COMPUTE);
    nvrhi::CommandListHandle copy_cmd_list =
        commandlist_create(CommandQueueFamily::TYPE_COPY);

    graphics_commandlist_pool.emplace_back(graphics_cmd_list);
    compute_commandlist_pool.emplace_back(compute_cmd_list);
    copy_commandlist_pool.emplace_back(copy_cmd_list);
  }
  // timer = nvrhi_device->createTimerQuery();
  resize_swapchain(swapchain_info.width, swapchain_info.height);
  return 0;
}

float RHIContextD3D12::get_time_query_data() {
  // return nvrhi_device->getTimerQueryTime(timer);
  return 0;
}

size_t RHIContextD3D12::init_device_factory() {
  uint32_t agility_sdk_version = 614;
  std::string agility_sdk_path = "D3D12";
  PFN_D3D12_GET_INTERFACE d3d_D3D12GetInterface =
      (PFN_D3D12_GET_INTERFACE)(void *)GetProcAddress(lib_d3d12,
                                                      "D3D12GetInterface");
  if (!d3d_D3D12GetInterface) {
    return 0; // Fallback to the system loader.
  }
  ID3D12SDKConfiguration *sdk_config = nullptr;
  if (SUCCEEDED(d3d_D3D12GetInterface(CLSID_D3D12SDKConfiguration,
                                      IID_PPV_ARGS(&sdk_config)))) {
    ID3D12SDKConfiguration1 *sdk_config1 = nullptr;
    if (SUCCEEDED(sdk_config->QueryInterface(&sdk_config1))) {
      if (SUCCEEDED(sdk_config1->CreateDeviceFactory(
              agility_sdk_version, agility_sdk_path.data(),
              IID_PPV_ARGS(device_factory.GetAddressOf())))) {
        d3d_D3D12GetInterface(CLSID_D3D12DeviceFactory,
                              IID_PPV_ARGS(device_factory.GetAddressOf()));
      } else if (SUCCEEDED(sdk_config1->CreateDeviceFactory(
                     agility_sdk_version, ".\\",
                     IID_PPV_ARGS(device_factory.GetAddressOf())))) {
        d3d_D3D12GetInterface(CLSID_D3D12DeviceFactory,
                              IID_PPV_ARGS(device_factory.GetAddressOf()));
      }
      sdk_config1->Release();
    }
    sdk_config->Release();
  }
  return 0;
}
size_t RHIContextD3D12::init_debug_layers() {
  ComPtr<ID3D12Debug> debug_controller;
  HRESULT res;

  if (device_factory) {
    res = device_factory->GetConfigurationInterface(
        CLSID_D3D12Debug, IID_PPV_ARGS(&debug_controller));
  } else {
    PFN_D3D12_GET_DEBUG_INTERFACE d3d_D3D12GetDebugInterface =
        (PFN_D3D12_GET_DEBUG_INTERFACE)(void *)GetProcAddress(
            lib_d3d12, "D3D12GetDebugInterface");
    if (d3d_D3D12GetDebugInterface == nullptr) {
      LOG_ERROR("can't not get d3d12 debug layer interface !!!");
      return -1;
    }
    res = d3d_D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller));
  }
  debug_controller->EnableDebugLayer();
  return 0;
}
size_t RHIContextD3D12::init_devices() {
  uint32_t dxgi_factory_flags = 0;
#ifdef DEBUG
  dxgi_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif
  typedef HRESULT(WINAPI * PFN_DXGI_CREATE_DXGI_FACTORY2)(UINT, REFIID,
                                                          void **);
  PFN_DXGI_CREATE_DXGI_FACTORY2 dxgi_CreateDXGIFactory2 =
      (PFN_DXGI_CREATE_DXGI_FACTORY2)(void *)GetProcAddress(
          lib_dxgi, "CreateDXGIFactory2");
  if (dxgi_CreateDXGIFactory2 == nullptr) {
    LOG_ERROR("can't not load CreateDXGIFactory Func Address !!!");
    return -1;
  }

  HRESULT res =
      dxgi_CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory));
  if (!SUCCEEDED(res)) {
    LOG_ERROR("can't not Create DXGIFactory !!!");
    return -1;
  }
  std::vector<IDXGIAdapter1 *> adapters;
  IDXGIAdapter1 *adapter = nullptr;
  adapter = create_adapter(adapters.size());
  res = adapter->GetDesc(&adapter_desc);
  if (!SUCCEEDED(res)) {
    LOG_ERROR("Adapters desc get error !!!");
    return -1;
  }
  // Logger::get_singleton().singletonLogger->info("{}-driver-{}",
  // adapter_desc.AdapterLuid, adapter_desc.Revision);
  if (device_factory != nullptr) {
    res = device_factory->CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0,
                                       IID_PPV_ARGS(device.GetAddressOf()));
  } else {
    PFN_D3D12_CREATE_DEVICE d3d_D3D12CreateDevice =
        (PFN_D3D12_CREATE_DEVICE)(void *)GetProcAddress(lib_d3d12,
                                                        "D3D12CreateDevice");
    res = d3d_D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0,
                                IID_PPV_ARGS(device.GetAddressOf()));
  }

  res = device->QueryInterface(debug_device.GetAddressOf());
  if (FAILED(res))
    LOG_ERROR("D3D12: Failed to query debug device!");

  ID3D12InfoQueue *info_queue = 0;
  device->QueryInterface(IID_PPV_ARGS(&info_queue));

  res = info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
  res = info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
  res = info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);

  D3D12_MESSAGE_SEVERITY SupressSeverities[] = {D3D12_MESSAGE_SEVERITY_INFO};

  D3D12_MESSAGE_ID SupressIDs[] = {
      D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
      D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
      D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
      D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
  };

  D3D12_INFO_QUEUE_FILTER filter = {0};
  filter.DenyList.NumSeverities = ARRAYSIZE(SupressSeverities);
  filter.DenyList.pSeverityList = SupressSeverities;
  filter.DenyList.NumIDs = ARRAYSIZE(SupressIDs);
  filter.DenyList.pIDList = SupressIDs;

  info_queue->PushStorageFilter(&filter);
  info_queue->Release();

  // if don't need adapter devce infomation, please release "adapter device
  // array" !!!!
  for (uint32_t i = 0; i < adapters.size(); ++i) {
    adapters[i]->Release();
  }
  return 0;
}

IDXGIAdapter1 *RHIContextD3D12::create_adapter(uint32_t p_adapter_index) const {
  ComPtr<IDXGIFactory6> factory_6;
  dxgi_factory.As(&factory_6);

  // TODO: Use IDXCoreAdapterList, which gives more comprehensive information.
  IDXGIAdapter1 *adapter = nullptr;
  if (factory_6) {
    if (factory_6->EnumAdapterByGpuPreference(
            p_adapter_index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&adapter)) == DXGI_ERROR_NOT_FOUND) {
      return nullptr;
    }
  } else {
    if (dxgi_factory->EnumAdapters1(p_adapter_index, &adapter) ==
        DXGI_ERROR_NOT_FOUND) {
      return nullptr;
    }
  }

  return adapter;
}

ComPtr<ID3D12CommandQueue>
RHIContextD3D12::command_queue_create(CommandQueueFamily type,
                                      std::wstring queue_name) {
  ComPtr<ID3D12CommandQueue> d3d_queue;
  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  queue_desc.Type = (D3D12_COMMAND_LIST_TYPE)(type);
  HRESULT res = device->CreateCommandQueue(
      &queue_desc, IID_PPV_ARGS(d3d_queue.GetAddressOf()));
  if (FAILED(res)) {
    LOG_ERROR("Failed to create Command Queue !!");
    return nullptr;
  }
  d3d_queue->SetName(queue_name.c_str());
  return d3d_queue;
}

void RHIContextD3D12::DefaultMessageCallback::message(
    nvrhi::MessageSeverity severity, const char *messageText) {
  switch (severity) {
  case nvrhi::MessageSeverity::Info:
    LOG_INFO(messageText);
    break;
  case nvrhi::MessageSeverity::Warning:
    LOG_ERROR(messageText);
    break;
  case nvrhi::MessageSeverity::Error:
    LOG_ERROR(messageText);
    break;
  case nvrhi::MessageSeverity::Fatal:
    LOG_ERROR(messageText);
    break;
  }
}

ComPtr<IDXGISwapChain4> RHIContextD3D12::swapchain_create(HWND window_handle) {
  RECT client_rect;
  GetClientRect(window_handle, &client_rect);
  UINT width = client_rect.right - client_rect.left;
  UINT height = client_rect.bottom - client_rect.top;
  DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
  ZeroMemory(&swapchain_desc, sizeof(swapchain_desc));
  swapchain_desc.Width = width;
  swapchain_desc.Height = height;
  swapchain_desc.SampleDesc.Count = 1;
  swapchain_desc.SampleDesc.Quality = 0;
  swapchain_desc.BufferUsage =
      DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
  ;
  swapchain_desc.BufferCount = 3;
  swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

  bool tearing_supported;
  ComPtr<IDXGIFactory5> pDxgiFactory5;
  if (SUCCEEDED(dxgi_factory->QueryInterface(IID_PPV_ARGS(&pDxgiFactory5)))) {
    BOOL supported = 0;
    if (SUCCEEDED(pDxgiFactory5->CheckFeatureSupport(
            DXGI_FEATURE_PRESENT_ALLOW_TEARING, &supported, sizeof(supported))))
      tearing_supported = (supported != 0);
  }

  if (tearing_supported) {
    // swapchain_desc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
  }

  ComPtr<IDXGISwapChain1> swap_chain_1;
  HRESULT hr = pDxgiFactory5->CreateSwapChainForHwnd(
      graphics_queue.Get(), window_handle, &swapchain_desc, nullptr, nullptr,
      swap_chain_1.GetAddressOf());

  ComPtr<IDXGISwapChain4> swapchain = nullptr;
  hr = swap_chain_1->QueryInterface(IID_PPV_ARGS(&swapchain));

  hr = pDxgiFactory5->MakeWindowAssociation(
      window_handle, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);

  swapchain_info.width = width;
  swapchain_info.height = height;

  hr =
      device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frame_fence));
  if (FAILED(hr))
    LOG_ERROR("Failed to create Fence !!");
  for (UINT bufferIndex = 0; bufferIndex < MAX_FRAMES_COUNT; bufferIndex++) {
    frame_fence_events.push_back(CreateEvent(nullptr, false, true, nullptr));
  }
  return swapchain;
}

bool RHIContextD3D12::create_render_targets() {
  swapchain_buffers.resize(MAX_FRAMES_COUNT);
  rhi_swapchain_buffers.resize(MAX_FRAMES_COUNT);

  for (UINT n = 0; n < MAX_FRAMES_COUNT; n++) {
    const HRESULT hr =
        swapchain->GetBuffer(n, IID_PPV_ARGS(&swapchain_buffers[n]));
    if (FAILED(hr))
      return false;

    nvrhi::TextureDesc textureDesc;
    textureDesc.width = swapchain_info.width;
    textureDesc.height = swapchain_info.height;
    textureDesc.sampleCount = 1;
    textureDesc.sampleQuality = 0;
    textureDesc.format = nvrhi::Format::RGBA8_UNORM;
    textureDesc.debugName = "SwapChainBuffer";
    textureDesc.isRenderTarget = true;
    textureDesc.isUAV = false;
    textureDesc.initialState = nvrhi::ResourceStates::Present;
    textureDesc.keepInitialState = true;

    rhi_swapchain_buffers[n] = nvrhi_device->createHandleForNativeTexture(
        nvrhi::ObjectTypes::D3D12_Resource,
        nvrhi::Object(swapchain_buffers[n].Get()), textureDesc);
  }

  return true;
}

nvrhi::TextureHandle RHIContextD3D12::texture_create(nvrhi::TextureDesc tex_desc) {
  return nvrhi_device->createTexture(tex_desc);
}

void RHIContextD3D12::release_render_targets() {
  // Make sure that all frames have finished rendering
  nvrhi_device->waitForIdle();

  // Release all in-flight references to the render targets
  nvrhi_device->runGarbageCollection();

  // Set the events so that WaitForSingleObject in OneFrame will not hang later
  for (auto e : frame_fence_events)
    SetEvent(e);

  // Release the old buffers because ResizeBuffers requires that
  rhi_swapchain_buffers.clear();
  swapchain_buffers.clear();
}

ComPtr<ID3D12Fence> RHIContextD3D12::fence_create() {
  ComPtr<ID3D12Fence> d3d_fence;
  HRESULT res = device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                    IID_PPV_ARGS(d3d_fence.GetAddressOf()));
  if (FAILED(res))
    LOG_ERROR("Failed to create fence");
  return d3d_fence;
}

void RHIContextD3D12::resize_swapchain(uint32_t width, uint32_t height) {
  rhi_swapchain_buffers.clear();
  swapchain_buffers.clear();
  swapchain_frame_buffers.clear();
  nvrhi_device->waitForIdle();
  nvrhi_device->runGarbageCollection();
  for (auto e : frame_fence_events)
    SetEvent(e);

  if (!nvrhi_device)
    return;
  if (!swapchain)
    return;
  const HRESULT hr = swapchain->ResizeBuffers(
      MAX_FRAMES_COUNT, width, height, DXGI_FORMAT_R8G8B8A8_UNORM,
      DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
  if (FAILED(hr))
    LOG_ERROR("Resize Buffer Failed");
  create_render_targets();

  uint32_t backBufferCount = MAX_FRAMES_COUNT;
  swapchain_frame_buffers.resize(backBufferCount);

  swapchain_info.width = width;
  swapchain_info.height = height;

  for (uint32_t index = 0; index < backBufferCount; index++) {
    nvrhi::ITexture *back_buffer = (index < rhi_swapchain_buffers.size())
                                       ? rhi_swapchain_buffers[index]
                                       : nullptr;
    nvrhi::FramebufferHandle freamPtr = nvrhi_device->createFramebuffer(
        nvrhi::FramebufferDesc().addColorAttachment(back_buffer));
    swapchain_frame_buffers.at(index) = freamPtr;
  }
}

nvrhi::CommandListHandle
RHIContextD3D12::commandlist_create(CommandQueueFamily type) {
  nvrhi::CommandQueue nvrhiType;
  switch (type) {
  case CommandQueueFamily::TYPE_GRAPHICS:
    nvrhiType = nvrhi::CommandQueue::Graphics;
    break;
  case CommandQueueFamily::TYPE_COMPUTE:
    nvrhiType = nvrhi::CommandQueue::Compute;
    break;
  case CommandQueueFamily::TYPE_COPY:
    nvrhiType = nvrhi::CommandQueue::Copy;
    break;
  }
  nvrhi::CommandListParameters params;
  params.queueType = nvrhiType;
  // deafault create graphics commandlist
  return nvrhi_device->createCommandList(params);
}

void RHIContextD3D12::begin_frame() {
  auto bufferIndex = swapchain->GetCurrentBackBufferIndex();
  WaitForSingleObject(frame_fence_events[bufferIndex], INFINITE);
}

nvrhi::FramebufferHandle RHIContextD3D12::get_current_frame_buffer() {
  return swapchain_frame_buffers.at(swapchain->GetCurrentBackBufferIndex());
}

nvrhi::TextureHandle RHIContextD3D12::get_swapchain_back_buffer() {
  return rhi_swapchain_buffers.at(swapchain->GetCurrentBackBufferIndex());
}

nvrhi::CommandListHandle
RHIContextD3D12::get_current_command_list(CommandQueueFamily type) {
  switch (type) {
  case CommandQueueFamily::TYPE_GRAPHICS:
    return graphics_commandlist_pool.at(swapchain->GetCurrentBackBufferIndex());
    break;
  case CommandQueueFamily::TYPE_COMPUTE:
    return compute_commandlist_pool.at(swapchain->GetCurrentBackBufferIndex());
    break;
  case CommandQueueFamily::TYPE_COPY:
    return copy_commandlist_pool.at(swapchain->GetCurrentBackBufferIndex());
    break;
  default:
    return graphics_commandlist_pool.at(swapchain->GetCurrentBackBufferIndex());
    break;
  }
}

void RHIContextD3D12::excute_command_list(nvrhi::CommandListHandle cmdlist) {
  nvrhi_device->executeCommandList(cmdlist);
}

void RHIContextD3D12::present(bool vsyncState) {
  auto bufferIndex = swapchain->GetCurrentBackBufferIndex();
  swapchain->Present(0, 0);
  frame_fence->SetEventOnCompletion(frame_count,
                                    frame_fence_events[bufferIndex]);
  graphics_queue->Signal(frame_fence.Get(), frame_count);
  frame_count++;
}

nvrhi::ShaderHandle
RHIContextD3D12::shader_create_from_bytecode(nvrhi::ShaderType type,
                                             std::vector<uint8_t> byte_code) {
  nvrhi::ShaderDesc desc;
  desc.shaderType = type;
  return nvrhi_device->createShader(desc, (void *)byte_code.data(),
                                    byte_code.size() * sizeof(uint8_t));
}

nvrhi::GraphicsPipelineHandle
RHIContextD3D12::render_pipeline_create(nvrhi::GraphicsPipelineDesc desc,
                                        nvrhi::FramebufferHandle fram_buffer) {

  return nvrhi_device->createGraphicsPipeline(desc, fram_buffer);
}

} // namespace Yoda
