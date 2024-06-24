#pragma once
#include <d3dx12.h>
#include <memory>

namespace Yoda {
class D3D12Device;
enum class CommandQueueType;
class D3D12CommandQueue {
public:
  D3D12CommandQueue(std::shared_ptr<D3D12Device> device, CommandQueueType type);
  ~D3D12CommandQueue();
  ID3D12CommandQueue *GetQueue() { return queue; }
  D3D12_COMMAND_LIST_TYPE GetType() { return type; }

protected:
  ID3D12CommandQueue* queue;
  D3D12_COMMAND_LIST_TYPE type;
};
} // namespace Yoda