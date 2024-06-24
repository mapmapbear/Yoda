#pragma once
#include "rhi/d3d12/rhi_d3d12_descriptor_heap.h"
#include "rhi/rhi_context_commons.h"
#include <d3d12.h>
#include <memory>

namespace Yoda {
class D3D12Device;
class D3D12Allocator;
class D3D12DescroptorHeap;

class Texture {
public:
  Texture(std::shared_ptr<D3D12Device> device);
  Texture(std::shared_ptr<D3D12Device> device,
          std::shared_ptr<D3D12Allocator> allocator, uint32_t width,
          uint32_t height, TextureFormat format, TextureUsage usage);
  struct GPUResource *get_resource();
  D3D12_RESOURCE_STATES get_state();
  void set_state(D3D12_RESOURCE_STATES state);

public:
  std::shared_ptr<D3D12Device> m_device;
  std::shared_ptr<D3D12DescroptorHeap> rtvHeap;
  std::shared_ptr<D3D12DescroptorHeap> dsvHeap;
  std::shared_ptr<D3D12DescroptorHeap> shaderHeap;

  D3D12_RESOURCE_STATES m_state;
  struct GPUResource *m_resource;
  bool m_release = true;
  D3D12DescroptorHeap::Descriptor *m_rtv;
  D3D12DescroptorHeap::Descriptor *m_dsv;
  D3D12DescroptorHeap::Descriptor *m_srvUav;

  TextureFormat m_format;
};
}; // namespace Yoda