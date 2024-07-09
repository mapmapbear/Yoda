#include "core/shadercode.h"
#include "core/logger.h"
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <vector>
#include <winerror.h>
#include <winnt.h>
#include <wrl/client.h>

namespace Yoda {

const wchar_t *get_entry_name(nvrhi::ShaderType type) {
  switch (type) {
  case nvrhi::ShaderType::Vertex:
    return L"vs_6_6";
    break;
  case nvrhi::ShaderType::Pixel:
    return L"ps_6_6";
    break;
  case nvrhi::ShaderType::Compute:
    return L"cs_6_6";
    break;
  default:
    return L"";
    break;
  }
}

void ShaderByteCode::compile_shader(nvrhi::ShaderType type, std::string &path,
                                    std::string &entry_point) {
  using namespace Microsoft::WRL;

  ComPtr<IDxcUtils> p_utils;
  ComPtr<IDxcCompiler> p_compiler;
  if (!SUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&p_utils)))) {
    LOG_ERROR("DXC: Failed to create DXC utils instance!");
  }
  if (!SUCCEEDED(
          DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&p_compiler)))) {
    LOG_ERROR("DXC: Failed to create DXC compiler instance!");
  }

  ComPtr<IDxcIncludeHandler> p_include_handler;
  if (!SUCCEEDED(p_utils->CreateDefaultIncludeHandler(&p_include_handler))) {
    LOG_ERROR("DXC: Failed to create default include handler!");
  }
  wchar_t w_file_path[512];
  swprintf_s(w_file_path, 512, L"%hs", path.c_str());
  ComPtr<IDxcBlobEncoding> pSourceBlob;
  if (!SUCCEEDED(
          p_utils->LoadFile(w_file_path, 0, pSourceBlob.GetAddressOf()))) {
    LOG_ERROR("DXC: Failed to create output blob!");
  }

  LPCWSTR pArgs[] = {L"-Zs", L"-Fd", L"-Fre"};
  wchar_t wideEntry[512];
  swprintf_s(wideEntry, 512, L"%hs", entry_point.c_str());
  ComPtr<IDxcOperationResult> p_result;
  if (!SUCCEEDED(p_compiler->Compile(
          pSourceBlob.Get(), L"Shader", wideEntry, get_entry_name(type), pArgs,
          ARRAYSIZE(pArgs), nullptr, 0, p_include_handler.Get(), &p_result))) {
    LOG_ERROR("DXC: Failed to compile shader!");
  }
  ComPtr<IDxcBlobEncoding> p_errors;
  p_result->GetErrorBuffer(&p_errors);

  if (p_errors && p_errors->GetBufferSize() != 0) {
    ComPtr<IDxcBlobUtf8> pErrorsU8;
    p_errors->QueryInterface(IID_PPV_ARGS(&pErrorsU8));
    LOG_ERROR("Shader errors:{}", (char *)pErrorsU8->GetStringPointer());
  }

  ComPtr<IDxcBlob> pShaderBlob;
  p_result->GetResult(&pShaderBlob);

  m_type = type;
  size_t blob_byte_size = pShaderBlob->GetBufferSize() / sizeof(uint8_t);
  m_byte_code.resize(blob_byte_size);
  m_byte_code = std::vector<uint8_t>(
      (uint8_t *)pShaderBlob->GetBufferPointer(),
      ((uint8_t *)pShaderBlob->GetBufferPointer() + blob_byte_size));
}
} // namespace Yoda