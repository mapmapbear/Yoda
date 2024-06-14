#pragma once
#include <memory>
#include <d3dx12.h>

namespace Yoda {
class D3D12Device;
class D3D12CommandQueue;

class D3D12Fence {
public:
  D3D12Fence(std::shared_ptr<D3D12Device> device);
  ~D3D12Fence();
  uint64_t signal(std::shared_ptr<D3D12CommandQueue> queue);
  void wait(uint64_t target, uint64_t timeout);
  ID3D12Fence *get_fence() { return fence; }
  uint64_t get_value() { return value; }

protected:
  uint64_t value;
  ID3D12Fence *fence;
};
} // namespace Yoda