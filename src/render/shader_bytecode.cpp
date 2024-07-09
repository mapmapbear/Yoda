#include "shader_bytecode.h"
#include "core/logger.h"
#include "render/shader_bytecode.h"
#include <dxcapi.h>
#include <wrl/client.h>

namespace Yoda {

const char *GetProfileFromType(ShaderType type) {
  switch (type) {
  case ShaderType::Vertex: {
    return "vs_6_6";
  }
  case ShaderType::Fragment: {
    return "ps_6_6";
  }
  case ShaderType::Compute: {
    return "cs_6_6";
  }
  }
  return "???";
}

std::wstring string_to_wstring(const std::string &str) {
  return std::wstring(str.begin(), str.end());
}

void ShaderCompiler::CompileShader(const std::string &path,
                                   const std::string &entryPoint,
                                   ShaderType type, ShaderBytecode &bytecode) {
  using namespace Microsoft::WRL;
  wchar_t wideTarget[512];
  swprintf_s(wideTarget, 512, L"%hs", GetProfileFromType(type));

  wchar_t wideEntry[512];
  swprintf_s(wideEntry, 512, L"%hs", entryPoint.c_str());

  ComPtr<IDxcUtils> pUtils;
  ComPtr<IDxcCompiler> pCompiler;
  if (!SUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils)))) {
    LOG_ERROR("DXC: Failed to create DXC utils instance!");
  }
  if (!SUCCEEDED(
          DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)))) {
    LOG_ERROR("DXC: Failed to create DXC compiler instance!");
  }

  ComPtr<IDxcIncludeHandler> pIncludeHandler;
  if (!SUCCEEDED(pUtils->CreateDefaultIncludeHandler(&pIncludeHandler))) {
    LOG_ERROR("DXC: Failed to create default include handler!");
  }
  ComPtr<IDxcBlobEncoding> pSourceBlob;
  std::wstring wPath = string_to_wstring(path);
  if (!SUCCEEDED(
          pUtils->LoadFile(wPath.c_str(), 0, pSourceBlob.GetAddressOf()))) {
    LOG_ERROR("DXC: Failed to create output blob!");
  }

  LPCWSTR pArgs[] = {L"-Zs", L"-Fd", L"-Fre"};

  ComPtr<IDxcOperationResult> pResult;
  if (!SUCCEEDED(pCompiler->Compile(
          pSourceBlob.Get(), L"Shader", wideEntry, wideTarget, pArgs,
          ARRAYSIZE(pArgs), nullptr, 0, pIncludeHandler.Get(), &pResult))) {
    LOG_ERROR("DXC: Failed to compile shader!");
  }

  ComPtr<IDxcBlobEncoding> pErrors;
  pResult->GetErrorBuffer(&pErrors);

  if (pErrors && pErrors->GetBufferSize() != 0) {
    ComPtr<IDxcBlobUtf8> pErrorsU8;
    pErrors->QueryInterface(IID_PPV_ARGS(&pErrorsU8));
    LOG_ERROR("Shader errors:%s", (char *)pErrorsU8->GetStringPointer());
  }

  HRESULT Status;
  pResult->GetStatus(&Status);

  ComPtr<IDxcBlob> pShaderBlob;
  pResult->GetResult(&pShaderBlob);

  bytecode.type = type;
  bytecode.bytecode.resize(pShaderBlob->GetBufferSize() / sizeof(uint32_t));
  memcpy(bytecode.bytecode.data(), pShaderBlob->GetBufferPointer(),
         pShaderBlob->GetBufferSize());

  LOG_INFO("DXC: Compiled shader %s", path.c_str());
}
} // namespace Yoda