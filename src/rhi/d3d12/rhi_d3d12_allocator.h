#pragma once
#include <memory>
#include "D3D12MemAlloc.h"
namespace Yoda {
class D3D12Device;
class D3D12Allocator {
public:
  D3D12Allocator(std::shared_ptr<D3D12Device> device);
  ~D3D12Allocator();

protected:
  std::shared_ptr<D3D12MA::Allocator> allocator;
};
} // namespace Yoda