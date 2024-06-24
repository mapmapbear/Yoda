#pragma once
#include "D3D12MemAlloc.h"
#include <d3d12.h>
#include <memory>

namespace Yoda {
struct GPUResource {
  ID3D12Resource *Resource;
  D3D12MA::Allocation *Allocation;
};

class D3D12Device;
class D3D12Allocator {
public:
  D3D12Allocator(std::shared_ptr<D3D12Device> device);
  ~D3D12Allocator();
  std::shared_ptr<D3D12MA::Allocator> get_allocator() { return m_allocator; }
  GPUResource Allocate(D3D12MA::ALLOCATION_DESC *allocDesc,
                       D3D12_RESOURCE_DESC *resourceDesc,
                       D3D12_RESOURCE_STATES state);

protected:
  std::shared_ptr<D3D12MA::Allocator> m_allocator;
};
} // namespace Yoda