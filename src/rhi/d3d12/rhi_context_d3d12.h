#pragma once
#include "d3d12.h"
#include "rhi/rhi_context.h"
#include "rhi/rhi_context_commons.h"
#include <d3dx12.h>
#include <dxgi1_5.h>
#include <dxgi1_6.h>
#include <stdint.h>
#include <vector>
#include <wrl/client.h>

#include <nvrhi/d3d12.h>
#include <nvrhi/validation.h>

using namespace Microsoft::WRL;

namespace Yoda {
struct SwapChainInfo {
  int width = 0;
  int height = 0;
};
class RHIContextD3D12 : public RHIContext {
public:
  RHIContextD3D12();
  ~RHIContextD3D12();
  virtual size_t initialize(HWND handle) override;

  ComPtr<ID3D12CommandQueue> command_queue_create(CommandQueueFamily type,
                                                  std::wstring queue_name);
  ComPtr<IDXGISwapChain4> swapchain_create(HWND window_handle);
  ComPtr<ID3D12Fence> fence_create();
  void resize_swapchain(uint32_t width, uint32_t height);
  void begin_frame();
  nvrhi::CommandListHandle commandlist_create(CommandQueueFamily type);
  nvrhi::TextureHandle texture_create(nvrhi::TextureDesc tex_desc);
  nvrhi::DeviceHandle get_rhi_device() { return nvrhi_device; }
  nvrhi::FramebufferHandle get_current_frame_buffer();
  nvrhi::TextureHandle get_swapchain_back_buffer();
  nvrhi::CommandListHandle get_current_command_list(CommandQueueFamily type);
  void excute_command_list(nvrhi::CommandListHandle cmdlist);
  void present(bool vsyncState);
  nvrhi::ShaderHandle
  shader_create_from_bytecode(nvrhi::ShaderType type,
                              std::vector<uint8_t> byte_code);
  nvrhi::GraphicsPipelineHandle
  render_pipeline_create(nvrhi::GraphicsPipelineDesc desc,
                         nvrhi::FramebufferHandle fram_buffer);
  float get_time_query_data();
  SwapChainInfo get_swapchain_info() { return swapchain_info; }

protected:
  size_t init_device_factory();
  size_t init_debug_layers();
  size_t init_devices();
  IDXGIAdapter1 *create_adapter(uint32_t p_adapter_index) const;

  struct DefaultMessageCallback : public nvrhi::IMessageCallback {
    static DefaultMessageCallback &GetInstance() {
      static DefaultMessageCallback Instance;
      return Instance;
    }

    void message(nvrhi::MessageSeverity severity,
                 const char *messageText) override;
  };

protected:
  // Device
  ComPtr<ID3D12DeviceFactory> device_factory;
  ComPtr<IDXGIFactory2> dxgi_factory;
  ComPtr<ID3D12Device> device;
  ComPtr<ID3D12DebugDevice> debug_device;
  DXGI_ADAPTER_DESC adapter_desc;
  HMODULE lib_d3d12 = nullptr;
  HMODULE lib_dxgi = nullptr;
  // Queue
  ComPtr<ID3D12CommandQueue> graphics_queue;
  ComPtr<ID3D12CommandQueue> compute_queue;
  ComPtr<ID3D12CommandQueue> copy_queue;
  // CommandList
  std::vector<nvrhi::CommandListHandle> graphics_commandlist_pool;
  std::vector<nvrhi::CommandListHandle> compute_commandlist_pool;
  std::vector<nvrhi::CommandListHandle> copy_commandlist_pool;
  ComPtr<IDXGISwapChain4> swapchain;

  SwapChainInfo swapchain_info;
  // Swapchain
  std::vector<ComPtr<ID3D12Resource>> swapchain_buffers;
  std::vector<nvrhi::TextureHandle> rhi_swapchain_buffers;
  std::vector<nvrhi::FramebufferHandle> swapchain_frame_buffers;
  ComPtr<ID3D12Fence> frame_fence;
  std::vector<HANDLE> frame_fence_events;
  uint64_t frame_count = 1;
  // Time Query
  // nvrhi::TimerQueryHandle timer;

private:
  bool create_render_targets();
  void release_render_targets();

public:
  nvrhi::DeviceHandle nvrhi_device;
};

} // namespace Yoda
