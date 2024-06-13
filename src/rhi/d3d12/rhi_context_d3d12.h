#pragma once
#include "rhi/rhi_context.h"
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include "D3D12MemAlloc.h"

using namespace Microsoft::WRL;

namespace Yoda {

class RHIContextD3D12 : public RHIContext {
public:
	RHIContextD3D12();
	~RHIContextD3D12();
	virtual size_t initialize(HWND handle) override;
	size_t initialize_allocator();
	

	size_t init_device_factory();
	size_t init_debug_layers();
	size_t init_devices();
	IDXGIAdapter1* create_adapter(uint32_t p_adapter_index) const;


    ComPtr<ID3D12DeviceFactory> device_factory;
	ComPtr<IDXGIFactory2> dxgi_factory;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12DebugDevice> debug_device;
	ComPtr<IDXGIAdapter1> main_adapter;
	ComPtr<D3D12MA::Allocator> allocator;

	DXGI_ADAPTER_DESC adapter_desc;
	HMODULE lib_d3d12 = nullptr;
	HMODULE lib_dxgi = nullptr;
};

} //namespace Yoda
