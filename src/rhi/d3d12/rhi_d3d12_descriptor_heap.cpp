#include "rhi/d3d12/rhi_d3d12_descriptor_heap.h"
#include "core/logger.h"
#include "rhi/d3d12/rhi_d3d12_device.h"
#include <d3d12.h>
#include <memory>
#include <vector>

namespace Yoda {

const std::string D3D12HeapTypeToStr(D3D12_DESCRIPTOR_HEAP_TYPE Type) {
  switch (Type) {
  case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
    return "D3D12_DESCRIPTOR_HEAP_TYPE_RTV";
  case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
    return "D3D12_DESCRIPTOR_HEAP_TYPE_DSV";
  case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
    return "D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV";
  case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
    return "D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER";
  default:
    break;
  }
  return "?????";
}

D3D12DescroptorHeap::D3D12DescroptorHeap(std::shared_ptr<D3D12Device> device,
                                         DescriptorHeapType type, uint32_t size)
    : _type(D3D12_DESCRIPTOR_HEAP_TYPE(type)), heap_size(size) {
  table = std::vector<bool>(size, false);
  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.Type = _type;
  desc.NumDescriptors = size;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  if (_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
      _type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  HRESULT result = device->get_d3d_device()->CreateDescriptorHeap(
      &desc, IID_PPV_ARGS(&heap));
  if (FAILED(result)) {
    LOG_ERROR("Failed to create descriptor heap!");
  }
  increment_size =
      device->get_d3d_device()->GetDescriptorHandleIncrementSize(_type);

  LOG_INFO("D3D12: Allocated descriptor heap of type {} and size {}",
           D3D12HeapTypeToStr(_type), (int)heap_size);
}

D3D12DescroptorHeap::~D3D12DescroptorHeap() {
  table.clear();
  heap->Release();
}

D3D12DescroptorHeap::Descriptor D3D12DescroptorHeap::Allocate() {
  size_t index = -1;
  for (size_t i = 0; i < heap_size; ++i) {
    if (table[i] == false) {
      table[i] = true;
      index = i;
      break;
    }
  }
  if (index == -1) {
    LOG_ERROR("Failed to find suitable descriptor!");
    return Descriptor(this, -1);
  } else {
    
    return Descriptor(this, index);
  }
}

D3D12DescroptorHeap::Descriptor::Descriptor()
    : parent_heap(nullptr), heap_index(-1), valid(false) {}

D3D12DescroptorHeap::Descriptor::Descriptor(D3D12DescroptorHeap *parentHeap,
                                            int index)
    : parent_heap(parentHeap), heap_index(index) {
  if (heap_index == -1) {
    valid = false;
    return;
  }

  CPU = parentHeap->heap->GetCPUDescriptorHandleForHeapStart();
  CPU.ptr += heap_index * parentHeap->increment_size;

  if (parentHeap->_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
      parentHeap->_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
    GPU = parentHeap->heap->GetGPUDescriptorHandleForHeapStart();
    GPU.ptr += heap_index * parentHeap->increment_size;
  }

  valid = true;
}
void D3D12DescroptorHeap::Release(Descriptor descriptor) {
    table[descriptor.heap_index] = false;
    descriptor.valid = false;
    descriptor.parent_heap = nullptr;
}
}; // namespace Yoda