#include "rhi_d3d12_allocator.h"
#include "rhi/d3d12/rhi_d3d12_device.h"
#include "core/logger.h"

namespace Yoda {
D3D12Allocator::D3D12Allocator(std::shared_ptr<D3D12Device> device) {
  D3D12MA::ALLOCATOR_DESC allocator_desc = {};
  allocator_desc.pDevice = device->get_d3d_device().Get();
  allocator_desc.pAdapter = device->get_adapter().Get();
  allocator_desc.Flags = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;
  D3D12MA::Allocator *alloctorPtr = (allocator.get());
  HRESULT res = D3D12MA::CreateAllocator(&allocator_desc, &alloctorPtr);
  if (!SUCCEEDED(res)) {
    Logger::get_singleton().singletonLogger->error(
        "D3D12MA::CreateAllocator failed with error ");
  }
}
D3D12Allocator::~D3D12Allocator()
{
    allocator->Release();
}
} // namespace Yoda