
#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>
#include "rhi/d3d12/rhi_d3d12_descriptor_heap.h"

#define MAX_FRAMES_COUNT 3

namespace Yoda {
class D3D12Device;
class D3D12CommandQueue;
class D3D12DescroptorHeap;
class Texture;
class D3D12Swapchain {
public:
  D3D12Swapchain(std::shared_ptr<D3D12Device> device,
                 std::shared_ptr<D3D12CommandQueue> graphics_quene,
                 std::shared_ptr<D3D12DescroptorHeap> rtvHeap, HWND window);
  ~D3D12Swapchain();
  void present(bool vsync);
  void resize(uint32_t width, uint32_t height);
 int AcquireImage();
 std::shared_ptr<Texture> GetTexture(uint32_t index) { return m_textures[index]; }

protected:
  std::shared_ptr<D3D12Device> device;
  std::shared_ptr<D3D12DescroptorHeap> heap;
  HWND window_handle;
  IDXGISwapChain3 *swapchain;
  ID3D12Resource *buffers[MAX_FRAMES_COUNT];
  using Descriptor = D3D12DescroptorHeap::Descriptor;
  Descriptor* descriptors[MAX_FRAMES_COUNT];
  std::shared_ptr<Texture> m_textures[MAX_FRAMES_COUNT];
  int _width;
  int _height;
};
} // namespace Yoda