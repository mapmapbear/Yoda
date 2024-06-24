#include "texture.h"
#include "render/texture.h"
#include "rhi/d3d12/rhi_d3d12_allocator.h"

namespace Yoda {

D3D12_RESOURCE_FLAGS GetResourceFlag(TextureUsage usage) {
  switch (usage) {
  case TextureUsage::RenderTarget:
    return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
           D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  case TextureUsage::DepthTarget:
    return D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  case TextureUsage::Copy:
    return D3D12_RESOURCE_FLAG_NONE;
  case TextureUsage::ShaderResource:
  case TextureUsage::Storage:
    return D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  }
  return D3D12_RESOURCE_FLAG_NONE;
}
Texture::Texture(std::shared_ptr<D3D12Device> device)
    : m_release(false), m_device(device) {}
Texture::Texture(std::shared_ptr<D3D12Device> device,
                 std::shared_ptr<D3D12Allocator> allocator, uint32_t width,
                 uint32_t height, TextureFormat format, TextureUsage usage) {
  switch (usage) {
  case TextureUsage::RenderTarget:
    m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
    break;
  case TextureUsage::DepthTarget:
    m_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    break;
  case TextureUsage::Storage:
    m_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    break;
  case TextureUsage::ShaderResource:
    m_state = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
    break;
  case TextureUsage::Copy:
    m_state = D3D12_RESOURCE_STATE_COPY_DEST;
    break;
  }
  D3D12MA::ALLOCATION_DESC AllocationDesc = {};
  AllocationDesc.HeapType = usage == TextureUsage::Copy
                                ? D3D12_HEAP_TYPE_UPLOAD
                                : D3D12_HEAP_TYPE_DEFAULT;

  D3D12_RESOURCE_DESC ResourceDesc = {};
  ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  ResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  ResourceDesc.Width = width;
  ResourceDesc.Height = height;
  ResourceDesc.DepthOrArraySize = 1;
  ResourceDesc.MipLevels = 1;
  ResourceDesc.Format = DXGI_FORMAT(format);
  ResourceDesc.SampleDesc.Count = 1;
  ResourceDesc.SampleDesc.Quality = 0;
  ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  ResourceDesc.Flags = GetResourceFlag(usage);
  GPUResource res =
      allocator->Allocate(&AllocationDesc, &ResourceDesc, m_state);
  m_resource = &res;
}

GPUResource *Texture::get_resource() { return m_resource; }

D3D12_RESOURCE_STATES Texture::get_state() { return m_state; }

void Texture::set_state(D3D12_RESOURCE_STATES state) { m_state = state; }
} // namespace Yoda