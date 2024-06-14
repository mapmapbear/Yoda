#pragma once
#include "rhi/d3d12/rhi_context_d3d12.h"
#include "core/logger.h"
#include "rhi/d3d12/rhi_d3d12_allocator.h"
#include "rhi/d3d12/rhi_d3d12_command_queue.h"
#include "rhi/d3d12/rhi_d3d12_descriptor_heap.h"
#include "rhi/d3d12/rhi_d3d12_device.h"
#include "rhi/d3d12/rhi_d3d12_fence.h"
#include <memory>
#include <vcruntime.h>
#include <windef.h>

namespace Yoda {

RHIContextD3D12::RHIContextD3D12() {
  m_device = std::make_shared<D3D12Device>();
  alloctor = std::make_shared<D3D12Allocator>(m_device);

  graphics_queue =
      std::make_shared<D3D12CommandQueue>(m_device, CommandQueueType::Graphics);
  compute_queue =
      std::make_shared<D3D12CommandQueue>(m_device, CommandQueueType::Compute);
  copy_queue =
      std::make_shared<D3D12CommandQueue>(m_device, CommandQueueType::Copy);
  heaps.RTVHeap = std::make_shared<D3D12DescroptorHeap>(
      m_device, DescriptorHeapType::RenderTarget, 1024);
  heaps.DSVHeap = std::make_shared<D3D12DescroptorHeap>(
      m_device, DescriptorHeapType::DepthTarget, 1024);
  heaps.ShaderHeap = std::make_shared<D3D12DescroptorHeap>(
      m_device, DescriptorHeapType::ShaderResource, 1'000'000);
  heaps.SamplerHeap = std::make_shared<D3D12DescroptorHeap>(
      m_device, DescriptorHeapType::Sampler, 512);
  graphics_fence->fence = std::make_shared<D3D12Fence>(m_device);
  compute_fence->fence = std::make_shared<D3D12Fence>(m_device);
  copy_fence->fence = std::make_shared<D3D12Fence>(m_device);
  graphics_fence->value = 0;
  compute_fence->value = 0;
  copy_fence->value = 0;
}
RHIContextD3D12::~RHIContextD3D12() {}

size_t RHIContextD3D12::initialize(HWND handle) { return 0; }

} // namespace Yoda
