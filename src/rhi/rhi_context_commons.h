#pragma once
#include <d3d12.h>
#include <memory>
namespace Yoda {
class D3D12DescroptorHeap;
class RHIContextCommons {
public:
struct Heaps {
  std::shared_ptr<D3D12DescroptorHeap> RTVHeap;
  std::shared_ptr<D3D12DescroptorHeap> DSVHeap;
  std::shared_ptr<D3D12DescroptorHeap> ShaderHeap;
  std::shared_ptr<D3D12DescroptorHeap> SamplerHeap;
};
};

} //namespace Yoda
