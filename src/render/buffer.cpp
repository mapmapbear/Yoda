#include "rhi/d3d12/rhi_d3d12_allocator.h"
#include "render/buffer.h"
#include "core/logger.h"
#include <D3D12MemAlloc.h>

namespace Yoda {
Buffer::Buffer(std::shared_ptr<D3D12Allocator> allocator, uint64_t size,
               uint64_t stride, BufferType type, bool readback) {
  D3D12MA::ALLOCATION_DESC AllocationDesc = {};
  AllocationDesc.HeapType =
      readback == true
          ? D3D12_HEAP_TYPE_READBACK
          : ((type == BufferType::Constant || type == BufferType::Copy)
                 ? D3D12_HEAP_TYPE_UPLOAD
                 : D3D12_HEAP_TYPE_DEFAULT);

  D3D12_RESOURCE_DESC ResourceDesc = {};
  ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  ResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  ResourceDesc.Width = size;
  ResourceDesc.Height = 1;
  ResourceDesc.DepthOrArraySize = 1;
  ResourceDesc.MipLevels = 1;
  ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  ResourceDesc.SampleDesc.Count = 1;
  ResourceDesc.SampleDesc.Quality = 0;
  ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  m_state = type == BufferType::Constant ? D3D12_RESOURCE_STATE_GENERIC_READ
                                         : D3D12_RESOURCE_STATE_COMMON;
  GPUResource res =
      allocator->Allocate(&AllocationDesc, &ResourceDesc, m_state);
  m_resource = &res;

  switch (type) {
  case BufferType::Vertex: {
    m_VB_view.BufferLocation = m_resource->Resource->GetGPUVirtualAddress();
    m_VB_view.SizeInBytes = size;
    m_VB_view.StrideInBytes = stride;
    break;
  }
  case BufferType::Index: {
    m_IB_view.BufferLocation = m_resource->Resource->GetGPUVirtualAddress();
    m_IB_view.SizeInBytes = size;
    m_IB_view.Format = DXGI_FORMAT_R32_UINT;
    break;
  }
  default: {
    break;
  }
  }
}

Buffer::~Buffer() {
  if (m_descriptor->valid) {
    m_heap->Release(*m_descriptor);
  }
  m_resource->Allocation->Release();
}

void Buffer::Map(int start, int end, void **data)
{
    D3D12_RANGE range;
    range.Begin = start;
    range.End = end;

    HRESULT result = 0;
    if (range.End > range.Begin) {
        result = m_resource->Resource->Map(0, &range, data);
    } else {
        result = m_resource->Resource->Map(0, nullptr, data);
    }

    if (FAILED(result)) {
        LOG_ERROR("Failed to map buffer from range [%d-%d]", start, end);
    }
}

void Buffer::Unmap(int start, int end)
{
    D3D12_RANGE range;
    range.Begin = start;
    range.End = end;

    if (range.End > range.Begin) {
        m_resource->Resource->Unmap(0, &range);
    } else {
        m_resource->Resource->Unmap(0, nullptr);
    }
}
} // namespace Yoda