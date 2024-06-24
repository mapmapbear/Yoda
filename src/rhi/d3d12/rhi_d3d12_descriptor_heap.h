#pragma once
#include <d3dx12.h>
#include <memory>

namespace Yoda {
enum class DescriptorHeapType {
  RenderTarget = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
  DepthTarget = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
  ShaderResource = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
  Sampler = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
};
class D3D12Device;

class D3D12DescroptorHeap {
public:
  struct Descriptor {
    bool valid;
    int heap_index;
    D3D12_CPU_DESCRIPTOR_HANDLE CPU;
    D3D12_GPU_DESCRIPTOR_HANDLE GPU;
    D3D12DescroptorHeap *parent_heap;

    Descriptor();
    Descriptor(D3D12DescroptorHeap *parentHeap, int index);
  };

public:
  D3D12DescroptorHeap(std::shared_ptr<D3D12Device> device,
                      DescriptorHeapType type, uint32_t size);
  ~D3D12DescroptorHeap();
  Descriptor Allocate();
  void Release(Descriptor descriptor);
  ID3D12DescriptorHeap *GetHeap() { return heap; }

protected:
  ID3D12DescriptorHeap *heap;
  D3D12_DESCRIPTOR_HEAP_TYPE _type;
  int increment_size;
  uint32_t heap_size;

  std::vector<bool> table;
};

} // namespace Yoda