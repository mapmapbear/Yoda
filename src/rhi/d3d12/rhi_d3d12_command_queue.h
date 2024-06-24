#pragma once
#include <d3dx12.h>
#include <memory>

namespace Yoda {
class D3D12Device;
class D3D12CommandList;
enum class CommandQueueType;
class D3D12Fence;
class D3D12CommandQueue {
public:
  D3D12CommandQueue(std::shared_ptr<D3D12Device> device, CommandQueueType type);
  ~D3D12CommandQueue();
  ID3D12CommandQueue *GetQueue() { return m_queue; }
  D3D12_COMMAND_LIST_TYPE GetType() { return m_type; }
  void submit(const std::vector<std::shared_ptr<D3D12CommandList>>& buffers);
  void wait(std::shared_ptr<D3D12Fence>fence, uint64_t value);

protected:
  ID3D12CommandQueue* m_queue;
  D3D12_COMMAND_LIST_TYPE m_type;
};
} // namespace Yoda