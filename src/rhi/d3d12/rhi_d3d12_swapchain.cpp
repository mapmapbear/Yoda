
#include "rhi/d3d12/rhi_d3d12_swapchain.h"
#include "core/logger.h"
#include "rhi/d3d12/rhi_d3d12_command_queue.h"
#include "rhi/d3d12/rhi_d3d12_descriptor_heap.h"
#include "rhi/d3d12/rhi_d3d12_device.h"

namespace Yoda {
D3D12Swapchain::D3D12Swapchain(
    std::shared_ptr<D3D12Device> device,
    std::shared_ptr<D3D12CommandQueue> graphics_quene,
    std::shared_ptr<D3D12DescroptorHeap> rtvHeap, HWND window)
    : heap(rtvHeap), window_handle(window) {
  RECT ClientRect;
  GetClientRect(window_handle, &ClientRect);
  _width = ClientRect.right - ClientRect.left;
  _height = ClientRect.bottom - ClientRect.top;

  DXGI_SWAP_CHAIN_DESC1 desc = {};
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount = MAX_FRAMES_COUNT;
  desc.Scaling = DXGI_SCALING_NONE;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  desc.Width = _width;
  desc.Height = _height;
  IDXGISwapChain1 *temp;
  HRESULT result = device->get_factory()->CreateSwapChainForHwnd(
      graphics_quene->GetQueue(), window_handle, &desc, nullptr, nullptr,
      &temp);
  if (FAILED(result)) {
    LOG_ERROR("D3D12: Failed to create swap chain!");
  }
}

D3D12Swapchain::~D3D12Swapchain() {
  for (int i = 0; i < MAX_FRAMES_COUNT; i++) {
    buffers[i]->Release();
    heap->Release(descriptors[i]);
  }
  swapchain->Release();
}
} // namespace Yoda