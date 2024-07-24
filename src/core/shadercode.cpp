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

const std::wstring get_shader_model(nvrhi::ShaderType type) {
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

std::wstring string_to_wstring(const std::string &str) {
  int size_needed =
      MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
  std::wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0],
                      size_needed);
  return wstrTo;
}

void ShaderByteCode::compile_shader(nvrhi::ShaderType type, std::string &path,
                                    std::string &entry_point) {
  // using namespace Microsoft::WRL;

  // ComPtr<IDxcUtils> p_utils;
  // ComPtr<IDxcCompiler> p_compiler;
  // if (!SUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&p_utils))))
  // {
  //   LOG_ERROR("DXC: Failed to create DXC utils instance!");
  // }
  // if (!SUCCEEDED(
  //         DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&p_compiler)))) {
  //   LOG_ERROR("DXC: Failed to create DXC compiler instance!");
  // }

  // ComPtr<IDxcIncludeHandler> p_include_handler;
  // if (!SUCCEEDED(p_utils->CreateDefaultIncludeHandler(&p_include_handler))) {
  //   LOG_ERROR("DXC: Failed to create default include handler!");
  // }
  // wchar_t w_file_path[512];
  // swprintf_s(w_file_path, 512, L"%hs", path.c_str());
  // ComPtr<IDxcBlobEncoding> pSourceBlob;
  // if (!SUCCEEDED(
  //         p_utils->LoadFile(w_file_path, 0, pSourceBlob.GetAddressOf()))) {
  //   LOG_ERROR("DXC: Failed to create output blob!");
  // }

  // LPCWSTR pArgs[] = {L"-Zs", L"-Fd", L"-Fre"};
  // wchar_t wideEntry[512];
  // swprintf_s(wideEntry, 512, L"%hs", entry_point.c_str());
  // ComPtr<IDxcOperationResult> p_result;
  // if (!SUCCEEDED(p_compiler->Compile(
  //         pSourceBlob.Get(), L"Shader", wideEntry, get_entry_name(type),
  //         pArgs, ARRAYSIZE(pArgs), nullptr, 0, p_include_handler.Get(),
  //         &p_result))) {
  //   LOG_ERROR("DXC: Failed to compile shader!");
  // }
  // ComPtr<IDxcBlobEncoding> p_errors;
  // p_result->GetErrorBuffer(&p_errors);

  // if (p_errors && p_errors->GetBufferSize() != 0) {
  //   ComPtr<IDxcBlobUtf8> pErrorsU8;
  //   p_errors->QueryInterface(IID_PPV_ARGS(&pErrorsU8));
  //   LOG_ERROR("Shader errors:{}", (char *)pErrorsU8->GetStringPointer());
  // }

  // ComPtr<IDxcBlob> pShaderBlob;
  // p_result->GetResult(&pShaderBlob);

  // m_type = type;
  // size_t blob_byte_size = pShaderBlob->GetBufferSize() / sizeof(uint8_t);
  // m_byte_code.resize(blob_byte_size);
  // m_byte_code = std::vector<uint8_t>(
  //     (uint8_t *)pShaderBlob->GetBufferPointer(),
  //     ((uint8_t *)pShaderBlob->GetBufferPointer() + blob_byte_size));
  compile_shader2(type, path, entry_point);
}

void ShaderByteCode::compile_shader2(nvrhi::ShaderType type, std::string &path,
                                     std::string &entry_point) {
  using namespace Microsoft::WRL;

  ComPtr<IDxcUtils> p_utils;
  ComPtr<IDxcCompiler3> p_compiler;
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
  std::wstring wPath = string_to_wstring(path);
  ComPtr<IDxcBlobEncoding> pSourceBlob = nullptr;
  p_utils->LoadFile(wPath.c_str(), nullptr, &pSourceBlob);
  DxcBuffer Source;
  Source.Ptr = pSourceBlob->GetBufferPointer();
  Source.Size = pSourceBlob->GetBufferSize();
  Source.Encoding = DXC_CP_ACP;

  std::vector<std::wstring> args = {};
#ifdef DEBUG
  // Disable Optimization
  args.emplace_back(L"-Od");
  // Enable debug information (slim format)
  args.emplace_back(L"-Zs");
  // output shader pdb file
  args.emplace_back(L"-Fd");
  size_t lastSlash = path.find_last_of("/");
  size_t lastDot = path.find_last_of(".");
  std::string catch_shader_name =
      path.substr(lastSlash + 1, lastDot - lastSlash - 1);
  //catch_shader_name = "shader_pdbs/" + catch_shader_name;
  catch_shader_name += ".pdb";
  args.emplace_back(string_to_wstring(catch_shader_name));
#endif
  // set entry point
  args.emplace_back(L"-E");
  args.emplace_back(string_to_wstring(entry_point));
  // set shader target
  args.emplace_back(L"-T");
  std::wstring shader_model = get_shader_model(type);
  args.emplace_back(shader_model);
  // shader path
  args.emplace_back(wPath);

  std::vector<const wchar_t *> args_raw;
  args_raw.reserve(args.size());
  for (auto &x : args) {
    args_raw.push_back(x.c_str());
  }

  ComPtr<IDxcResult> p_result;
  p_compiler->Compile(
      &Source, args_raw.data(), (uint32_t)args.size(), p_include_handler.Get(),
      IID_PPV_ARGS(&p_result) // Compiler output status, buffer, and errors.
  );

  ComPtr<IDxcBlobUtf8> p_errors = nullptr;
  p_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&p_errors), nullptr);

  if (p_errors && p_errors->GetBufferSize() != 0) {
    ComPtr<IDxcBlobUtf8> pErrorsU8;
    p_errors->QueryInterface(IID_PPV_ARGS(&pErrorsU8));
    LOG_ERROR("Shader errors:{}", (char *)pErrorsU8->GetStringPointer());
  }

#ifdef DEBUG
  ComPtr<IDxcBlob> pPDB = nullptr;
  ComPtr<IDxcBlobUtf16> pPDBName = nullptr;
  p_result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pPDB), &pPDBName);
  {
    FILE *fp = NULL;
    _wfopen_s(&fp, pPDBName->GetStringPointer(), L"wb");
    fwrite(pPDB->GetBufferPointer(), pPDB->GetBufferSize(), 1, fp);
    fclose(fp);
  }

#endif
  ComPtr<IDxcBlob> pShaderBlob;
  p_result->GetResult(&pShaderBlob);

  size_t blob_byte_size = pSourceBlob->GetBufferSize() / sizeof(uint8_t);
  // m_byte_code.resize(blob_byte_size);
  m_byte_code =
      std::vector<uint8_t>((uint8_t *)pShaderBlob->GetBufferPointer(),
                           ((uint8_t *)pShaderBlob->GetBufferPointer() +
                            pShaderBlob->GetBufferSize()));
  m_type = type;
}
} // namespace Yoda