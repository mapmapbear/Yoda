#include "core/logger.h"
#include "rhi/d3d12/rhi_d3d12_device.h"

namespace Yoda {
D3D12Device::D3D12Device() {
  if (lib_d3d12 != nullptr || lib_dxgi != nullptr)
		return;
	lib_d3d12 = LoadLibraryW(L"D3D12.dll");
	lib_dxgi = LoadLibraryW(L"DXGI.dll");
  size_t status = 0;
  status = init_device_factory();
  if (status != 0) {
    Logger::get_singleton().singletonLogger->error(
        "device factory create error !!!");
  }
#ifdef DEBUG
  status = init_debug_layers();
  if (status != 0) {
	  Logger::get_singleton().singletonLogger->error(
		  "debug layer create error !!!");
  }
#endif
  status = init_devices();
  if (status != 0) {
    Logger::get_singleton().singletonLogger->error(
        "d3d device create error !!!");
  }
}

D3D12Device::~D3D12Device() {
  device->Release();
  device_factory->Release();
  main_adapter->Release();
  debug_device->ReportLiveDeviceObjects(D3D12_RLDO_IGNORE_INTERNAL |
                                        D3D12_RLDO_DETAIL);
  debug_device->Release();
  if (lib_d3d12) {
		FreeLibrary(lib_d3d12);
	}
	if (lib_dxgi) {
		FreeLibrary(lib_dxgi);
	}
}

size_t D3D12Device::init_device_factory() {
  uint32_t agility_sdk_version = 614;
  std::string agility_sdk_path = "D3D12";
  PFN_D3D12_GET_INTERFACE d3d_D3D12GetInterface =
      (PFN_D3D12_GET_INTERFACE)(void *)GetProcAddress(lib_d3d12,
                                                      "D3D12GetInterface");
  if (!d3d_D3D12GetInterface) {
    return 0; // Fallback to the system loader.
  }
  ID3D12SDKConfiguration *sdk_config = nullptr;
  if (SUCCEEDED(d3d_D3D12GetInterface(CLSID_D3D12SDKConfiguration,
                                      IID_PPV_ARGS(&sdk_config)))) {
    ID3D12SDKConfiguration1 *sdk_config1 = nullptr;
    if (SUCCEEDED(sdk_config->QueryInterface(&sdk_config1))) {
      if (SUCCEEDED(sdk_config1->CreateDeviceFactory(
              agility_sdk_version, agility_sdk_path.data(),
              IID_PPV_ARGS(device_factory.GetAddressOf())))) {
        d3d_D3D12GetInterface(CLSID_D3D12DeviceFactory,
                              IID_PPV_ARGS(device_factory.GetAddressOf()));
      } else if (SUCCEEDED(sdk_config1->CreateDeviceFactory(
                     agility_sdk_version, ".\\",
                     IID_PPV_ARGS(device_factory.GetAddressOf())))) {
        d3d_D3D12GetInterface(CLSID_D3D12DeviceFactory,
                              IID_PPV_ARGS(device_factory.GetAddressOf()));
      }
      sdk_config1->Release();
    }
    sdk_config->Release();
  }
  return 0;
}
size_t D3D12Device::init_debug_layers() {
  ComPtr<ID3D12Debug> debug_controller;
  HRESULT res;

  if (device_factory) {
    res = device_factory->GetConfigurationInterface(
        CLSID_D3D12Debug, IID_PPV_ARGS(&debug_controller));
  } else {
    PFN_D3D12_GET_DEBUG_INTERFACE d3d_D3D12GetDebugInterface =
        (PFN_D3D12_GET_DEBUG_INTERFACE)(void *)GetProcAddress(
            lib_d3d12, "D3D12GetDebugInterface");
    if (d3d_D3D12GetDebugInterface == nullptr) {
      Logger::get_singleton().singletonLogger->error(
          "can't not get d3d12 debug layer interface !!!");
      return -1;
    }
    res = d3d_D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller));
  }
  debug_controller->EnableDebugLayer();
  return 0;
}
size_t D3D12Device::init_devices() {
  uint32_t dxgi_factory_flags = 0;
#ifdef DEBUG
  dxgi_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif
  typedef HRESULT(WINAPI * PFN_DXGI_CREATE_DXGI_FACTORY2)(UINT, REFIID,
                                                          void **);
  PFN_DXGI_CREATE_DXGI_FACTORY2 dxgi_CreateDXGIFactory2 =
      (PFN_DXGI_CREATE_DXGI_FACTORY2)(void *)GetProcAddress(
          lib_dxgi, "CreateDXGIFactory2");
  if (dxgi_CreateDXGIFactory2 == nullptr) {
    Logger::get_singleton().singletonLogger->error(
        "can't not load CreateDXGIFactory Func Address !!!");
    return -1;
  }

  HRESULT res =
      dxgi_CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory));
  if (!SUCCEEDED(res)) {
    Logger::get_singleton().singletonLogger->error(
        "can't not Create DXGIFactory !!!");
    return -1;
  }
  std::vector<IDXGIAdapter1 *> adapters;
  // IDXGIAdapter1* main_adapter = nullptr;
  main_adapter = create_adapter(adapters.size());
  res = main_adapter->GetDesc(&adapter_desc);
  if (!SUCCEEDED(res)) {
    Logger::get_singleton().singletonLogger->error(
        "Adapters desc get error !!!");
    return -1;
  }
  // Logger::get_singleton().singletonLogger->info("{}-driver-{}",
  // adapter_desc.AdapterLuid, adapter_desc.Revision);
  if (device_factory != nullptr) {
    res =
        device_factory->CreateDevice(main_adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                     IID_PPV_ARGS(device.GetAddressOf()));
  } else {
    PFN_D3D12_CREATE_DEVICE d3d_D3D12CreateDevice =
        (PFN_D3D12_CREATE_DEVICE)(void *)GetProcAddress(lib_d3d12,
                                                        "D3D12CreateDevice");
    res = d3d_D3D12CreateDevice(main_adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                IID_PPV_ARGS(device.GetAddressOf()));
  }

  res = device->QueryInterface(IID_PPV_ARGS(debug_device.GetAddressOf()));
  if (FAILED(res))
    Logger::get_singleton().singletonLogger->error(
        "D3D12: Failed to query debug device!");

  ID3D12InfoQueue *info_queue = 0;
  device->QueryInterface(IID_PPV_ARGS(&info_queue));

  res = info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
  res = info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
  res = info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);

  D3D12_MESSAGE_SEVERITY SupressSeverities[] = {D3D12_MESSAGE_SEVERITY_INFO};

  D3D12_MESSAGE_ID SupressIDs[] = {
      D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
      D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
      D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
      D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
  };

  D3D12_INFO_QUEUE_FILTER filter = {0};
  filter.DenyList.NumSeverities = ARRAYSIZE(SupressSeverities);
  filter.DenyList.pSeverityList = SupressSeverities;
  filter.DenyList.NumIDs = ARRAYSIZE(SupressIDs);
  filter.DenyList.pIDList = SupressIDs;

  info_queue->PushStorageFilter(&filter);
  info_queue->Release();

  // if don't need adapter devce infomation, please release "adapter device
  // array" !!!!
  for (uint32_t i = 0; i < adapters.size(); ++i) {
    adapters[i]->Release();
  }
  return 0;
}

IDXGIAdapter1 *D3D12Device::create_adapter(uint32_t p_adapter_index) const {
  ComPtr<IDXGIFactory6> factory_6;
  dxgi_factory.As(&factory_6);

  // TODO: Use IDXCoreAdapterList, which gives more comprehensive information.
  IDXGIAdapter1 *adapter = nullptr;
  if (factory_6) {
    if (factory_6->EnumAdapterByGpuPreference(
            p_adapter_index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&adapter)) == DXGI_ERROR_NOT_FOUND) {
      return nullptr;
    }
  } else {
    if (dxgi_factory->EnumAdapters1(p_adapter_index, &adapter) ==
        DXGI_ERROR_NOT_FOUND) {
      return nullptr;
    }
  }

  return adapter;
}
} // namespace Yoda