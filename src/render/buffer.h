#pragma once
#include "texture.h"
#include <memory>

namespace Yoda {
class D3D12Allocator;
class D3D12CommandList;
class D3D12DescroptorHeap;

class Buffer {
public:
  Buffer(std::shared_ptr<D3D12Allocator> allocator, uint64_t size,
         uint64_t stride, BufferType type, bool readback);
  ~Buffer();
  void BuildConstantBuffer(std::shared_ptr<D3D12Device> device,
                           std::shared_ptr<D3D12DescroptorHeap> heap);
  void BuildStorage(std::shared_ptr<D3D12Device> device,
                    std::shared_ptr<D3D12DescroptorHeap> heap);

  void Map(int start, int end, void **data);
  void Unmap(int start, int end);

  void SetState(D3D12_RESOURCE_STATES eState) { m_state = eState; }

protected:
  friend class D3D12CommandList;
  std::shared_ptr<D3D12DescroptorHeap> m_heap;

  BufferType _type;
  uint64_t _size;

  struct D3D12DescroptorHeap::Descriptor *m_descriptor;
  struct GPUResource *m_resource;
  D3D12_RESOURCE_STATES m_state;

  D3D12_VERTEX_BUFFER_VIEW m_VB_view;
  D3D12_INDEX_BUFFER_VIEW m_IB_view;
  D3D12_CONSTANT_BUFFER_VIEW_DESC m_CBV_desc;
  D3D12_UNORDERED_ACCESS_VIEW_DESC m_UAV_desc;
};
} // namespace Yoda
