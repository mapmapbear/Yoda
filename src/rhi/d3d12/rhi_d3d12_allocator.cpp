#include "rhi_d3d12_allocator.h"
#include "rhi/d3d12/rhi_d3d12_device.h"
#include "core/logger.h"

namespace Yoda {
D3D12Allocator::D3D12Allocator(std::shared_ptr<D3D12Device> device) {
  D3D12MA::ALLOCATOR_DESC allocator_desc = {};
  allocator_desc.pDevice = device->get_d3d_device();
  allocator_desc.pAdapter = device->get_adapter().Get();
  allocator_desc.Flags = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;
  D3D12MA::Allocator *alloctorPtr = (m_allocator.get());
  HRESULT res = D3D12MA::CreateAllocator(&allocator_desc, &alloctorPtr);
  if (!SUCCEEDED(res)) {
    Logger::get_singleton().singletonLogger->error(
        "D3D12MA::CreateAllocator failed with error ");
  }
}
D3D12Allocator::~D3D12Allocator()
{
    m_allocator->Release();
}

GPUResource D3D12Allocator::Allocate(D3D12MA::ALLOCATION_DESC *allocDesc, D3D12_RESOURCE_DESC *resourceDesc, D3D12_RESOURCE_STATES state)
{
    GPUResource resource;

    HRESULT result = m_allocator->CreateResource(allocDesc, resourceDesc, state, nullptr, &resource.Allocation, IID_PPV_ARGS(&resource.Resource));
    if (FAILED(result)) {
        LOG_ERROR("D3D12: Failed to allocate resource!");
    }
    return resource;
}
} // namespace Yoda