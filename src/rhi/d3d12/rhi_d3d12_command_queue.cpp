#include "rhi/d3d12/rhi_d3d12_command_queue.h"
#include "rhi/d3d12/rhi_d3d12_device.h"
#include "rhi/rhi_context_commons.h"
#include "rhi/d3d12/rhi_d3d12_command_list.h"
#include "rhi/d3d12/rhi_d3d12_fence.h"
#include "core/logger.h"
#include <memory>
#include <winerror.h>

namespace Yoda {
D3D12CommandQueue::D3D12CommandQueue(std::shared_ptr<D3D12Device> device,
                                     CommandQueueType type)
    : m_type(D3D12_COMMAND_LIST_TYPE(type)) {
  D3D12_COMMAND_QUEUE_DESC desc = {};
  desc.Type = D3D12_COMMAND_LIST_TYPE(type);
  HRESULT result =
      device->get_d3d_device()->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_queue));
  if (FAILED(result)) {
    Logger::get_singleton().singletonLogger->error(
        "Failed to create command queue!");
  }
}
D3D12CommandQueue::~D3D12CommandQueue() { m_queue->Release(); }

void D3D12CommandQueue::submit(
    const std::vector<std::shared_ptr<D3D12CommandList>> &buffers) {
  std::vector<ID3D12CommandList *> lists;
  for (auto &buffer : buffers) {
    lists.push_back(buffer->get_commandlist());
  }

  m_queue->ExecuteCommandLists(lists.size(), lists.data());
}

void D3D12CommandQueue::wait(std::shared_ptr<D3D12Fence> fence, uint64_t value)
{
    m_queue->Wait(fence->get_fence(), value);
}
} // namespace Yoda