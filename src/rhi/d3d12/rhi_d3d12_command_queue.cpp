#include "rhi/d3d12/rhi_d3d12_command_queue.h"
#include "rhi/d3d12/rhi_d3d12_device.h"
#include "core/logger.h"
#include <memory>
#include <winerror.h>
namespace Yoda {
D3D12CommandQueue::D3D12CommandQueue(std::shared_ptr<D3D12Device> device,
                                     CommandQueueType type)
    : type(D3D12_COMMAND_LIST_TYPE(type)) {
  D3D12_COMMAND_QUEUE_DESC desc = {};
  desc.Type = D3D12_COMMAND_LIST_TYPE(type);
  HRESULT result = device->get_d3d_device()->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue));
  if(FAILED(result)) {
    Logger::get_singleton().singletonLogger->error("Failed to create command queue!");
  }
}
D3D12CommandQueue::~D3D12CommandQueue() {
    queue->Release();
}
} // namespace Yoda