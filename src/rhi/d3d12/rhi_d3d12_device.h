#pragma once
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <memory>
#include <wrl/client.h>

using namespace Microsoft::WRL;

namespace Yoda {
class D3D12Device {
public:
  D3D12Device();
  ~D3D12Device();

  size_t init_device_factory();
  size_t init_debug_layers();
  size_t init_devices();
  IDXGIAdapter1 *create_adapter(uint32_t p_adapter_index) const;
  ComPtr<ID3D12Device> get_d3d_device() { return device; }
  ComPtr<IDXGIAdapter1> get_adapter() { return main_adapter; }
  ComPtr<IDXGIFactory2> get_factory() { return dxgi_factory; }

protected:
  ComPtr<ID3D12DeviceFactory> device_factory;
  ComPtr<IDXGIFactory2> dxgi_factory;
  ComPtr<ID3D12Device> device;
  ComPtr<ID3D12DebugDevice> debug_device;
  ComPtr<IDXGIAdapter1> main_adapter;

  DXGI_ADAPTER_DESC adapter_desc;
  HMODULE lib_d3d12 = nullptr;
  HMODULE lib_dxgi = nullptr;
};
} // namespace Yoda
