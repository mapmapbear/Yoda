#pragma once
#include "rhi/d3d12/rhi_d3d12_descriptor_heap.h"
#include "rhi/rhi_context_commons.h"
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <memory>
#include <wrl/client.h>

using namespace Microsoft::WRL;
#define FRAMES_IN_FLIGHT 3
namespace Yoda {
class D3D12Device;
class D3D12Allocator;
class D3D12CommandQueue;
class D3D12DescroptorHeap;
class D3D12Fence;
class D3D12CommandList;
class D3D12Swapchain;
class D3D12Pipeline;
class Texture;
class Buffer;
class Copyer;

struct FencePair {
  std::shared_ptr<D3D12Fence> fence = nullptr;
  uint64_t value = 0;
};
class RHIContextD3D12 {
public:
  RHIContextD3D12() = delete;
  RHIContextD3D12(HWND hwnd);
  ~RHIContextD3D12();
  void begin_frame();
  void end_frame();
  std::shared_ptr<Texture> get_back_buffer();
  std::shared_ptr<D3D12CommandList> get_current_command_list();
  std::shared_ptr<D3D12Pipeline> create_graphics_pipeline(GraphicsPipelineSpecs& specs);
  std::shared_ptr<Buffer> create_buffer(uint64_t size, uint64_t stride, BufferType type, bool readback);
  std::shared_ptr<Copyer> create_copyer();
  void FlushCopyerList(std::shared_ptr<Copyer> copyer);
  void excute_command_list(const std::vector<std::shared_ptr<D3D12CommandList>>& buffers, CommandQueueType type);
  void WaitForPreviousDeviceSubmit(CommandQueueType type);
public:
  std::shared_ptr<D3D12Device> m_device;
  std::shared_ptr<D3D12Allocator> m_alloctor;

  std::shared_ptr<D3D12CommandQueue> graphic_queue;
  std::shared_ptr<D3D12CommandQueue> compute_queue;
  std::shared_ptr<D3D12CommandQueue> copy_queue;

  std::shared_ptr<D3D12Swapchain> m_swapchain;

  Heaps heaps;
  FencePair *graphics_fence;
  FencePair *compute_fence;
  FencePair *copy_fence;

  std::shared_ptr<D3D12CommandList> command_lists[FRAMES_IN_FLIGHT];

  uint64_t frame_values[FRAMES_IN_FLIGHT];
  uint32_t frame_index;

  using Descriptor = D3D12DescroptorHeap::Descriptor;
  std::shared_ptr<Descriptor> font_descriptor;
};

} // namespace Yoda
