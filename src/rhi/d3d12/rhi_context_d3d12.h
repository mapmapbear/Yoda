#pragma once
#include "rhi/rhi_context.h"
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <memory>
#include <wrl/client.h>

using namespace Microsoft::WRL;

namespace Yoda {


class D3D12Device;
class D3D12Allocator;
class D3D12CommandQueue;
class D3D12DescroptorHeap;
class D3D12Fence;
struct FencePair
{
    std::shared_ptr<D3D12Fence> fence;
    uint64_t value;
};
class RHIContextD3D12 : public RHIContext {
	
public:
	RHIContextD3D12();
	~RHIContextD3D12();
	virtual size_t initialize(HWND handle) override;

	std::shared_ptr<D3D12Device> m_device;
	std::shared_ptr<D3D12Allocator> alloctor;

	std::shared_ptr<D3D12CommandQueue> graphics_queue;
	std::shared_ptr<D3D12CommandQueue> compute_queue;
	std::shared_ptr<D3D12CommandQueue> copy_queue;

	Heaps heaps;
	std::shared_ptr<FencePair> graphics_fence;
	std::shared_ptr<FencePair> compute_fence;
	std::shared_ptr<FencePair> copy_fence;
};

} //namespace Yoda
