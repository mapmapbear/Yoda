#include "rhi/d3d12/rhi_d3d12_fence.h"
#include "core/logger.h"
#include "rhi/d3d12/rhi_d3d12_command_queue.h"
#include "rhi/d3d12/rhi_d3d12_device.h"
#include <memory>
namespace Yoda {
D3D12Fence::D3D12Fence(std::shared_ptr<D3D12Device> device) : value(0) {
  HRESULT result = device->get_d3d_device()->CreateFence(
      value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
  if (FAILED(result)) {
    LOG_ERROR("Failed to create fence!");
  }
}

D3D12Fence::~D3D12Fence()
{
    fence->Release();
}

uint64_t D3D12Fence::signal(std::shared_ptr<D3D12CommandQueue> queue){
 value++;
 queue->GetQueue()->Signal(fence, value);
 return value;
}

void D3D12Fence::wait(uint64_t target, uint64_t timeout)
{
    if (fence->GetCompletedValue() < target) {
        HANDLE event = CreateEvent(nullptr, false, false, nullptr);
        fence->SetEventOnCompletion(target, event);
        if (WaitForSingleObject(event, timeout) == WAIT_TIMEOUT) {
            LOG_ERROR("!! GPU TIMEOUT !!");
        }
    }
}
} // namespace Yoda