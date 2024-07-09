#include "rhi/d3d12/rhi_d3d12_pipeline.h"
#include "core/logger.h"
#include "render/shader_bytecode.h"
#include "rhi/d3d12/rhi_d3d12_device.h"
#include <array>
#include <d3d12.h>
#include <d3d12shader.h>
#include <d3dx12.h>
#include <dxcapi.h>
#include <memory>

namespace Yoda {

ID3D12ShaderReflection *GetReflection(ShaderBytecode &bytecode,
                                      D3D12_SHADER_DESC *desc) {
  IDxcUtils *pUtils;
  DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));

  DxcBuffer ShaderBuffer = {};
  ShaderBuffer.Ptr = bytecode.bytecode.data();
  ShaderBuffer.Size = bytecode.bytecode.size() * sizeof(uint32_t);
  ID3D12ShaderReflection *pReflection;
  HRESULT Result =
      pUtils->CreateReflection(&ShaderBuffer, IID_PPV_ARGS(&pReflection));
  if (FAILED(Result)) {
    LOG_ERROR("Failed to get reflection from shader");
  }
  pReflection->GetDesc(desc);
  pUtils->Release();
  return pReflection;
}

D3D12Pipeline::D3D12Pipeline(std::shared_ptr<D3D12Device> device,
                             GraphicsPipelineSpecs &specs) {
  ShaderBytecode &vertexBytecode = specs.Bytecodes[ShaderType::Vertex];
  ShaderBytecode &fragmentBytecode = specs.Bytecodes[ShaderType::Fragment];

  D3D12_SHADER_DESC VertexDesc = {};
  D3D12_SHADER_DESC PixelDesc = {};

  std::vector<D3D12_INPUT_ELEMENT_DESC> InputElementDescs;
  std::vector<std::string> InputElementSemanticNames;

  std::array<D3D12_ROOT_PARAMETER, 64> parameters = {};
  int ParameterCount = 0;

  std::array<D3D12_DESCRIPTOR_RANGE, 64> ranges = {};
  int RangeCount = 0;

  std::array<D3D12_SHADER_INPUT_BIND_DESC, 64> shader_binds = {};
  int BindCount = 0;
  ID3D12ShaderReflection *pVertexReflection =
      GetReflection(vertexBytecode, &VertexDesc);
  ID3D12ShaderReflection *pPixelReflection =
      GetReflection(fragmentBytecode, &PixelDesc);

  for (int BoundResourceIndex = 0;
       BoundResourceIndex < VertexDesc.BoundResources; BoundResourceIndex++) {
    D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc = {};
    pVertexReflection->GetResourceBindingDesc(BoundResourceIndex,
                                              &ShaderInputBindDesc);
    shader_binds[BindCount] = ShaderInputBindDesc;
    BindCount++;
  }

  for (int BoundResourceIndex = 0;
       BoundResourceIndex < PixelDesc.BoundResources; BoundResourceIndex++) {
    D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc = {};
    pPixelReflection->GetResourceBindingDesc(BoundResourceIndex,
                                             &ShaderInputBindDesc);
    shader_binds[BindCount] = ShaderInputBindDesc;
    BindCount++;
  }

  std::sort(shader_binds.begin(), shader_binds.end(),
            [](const D3D12_SHADER_INPUT_BIND_DESC &A,
               const D3D12_SHADER_INPUT_BIND_DESC &B) {
              return A.BindPoint < B.BindPoint;
            });

  for (int ShaderBindIndex = 0; ShaderBindIndex < BindCount;
       ShaderBindIndex++) {
    D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc =
        shader_binds[ShaderBindIndex];

    D3D12_ROOT_PARAMETER RootParameter = {};
    RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

    D3D12_DESCRIPTOR_RANGE Range = {};
    Range.NumDescriptors = 1;
    Range.BaseShaderRegister = ShaderInputBindDesc.BindPoint;

    switch (ShaderInputBindDesc.Type) {
    case D3D_SIT_SAMPLER:
      Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
      break;
    case D3D_SIT_TEXTURE:
      Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      break;
    case D3D_SIT_UAV_RWTYPED:
      Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
      break;
    case D3D_SIT_CBUFFER:
      Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
      break;
    default:
      LOG_ERROR("D3D12: Unsupported shader resource!");
      continue;
    }

    ranges[RangeCount] = Range;

    RootParameter.DescriptorTable.NumDescriptorRanges = 1;
    RootParameter.DescriptorTable.pDescriptorRanges = &ranges[RangeCount];
    RootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    parameters[ParameterCount] = RootParameter;

    ParameterCount++;
    RangeCount++;
  }

  D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
  RootSignatureDesc.NumParameters = ParameterCount;
  RootSignatureDesc.pParameters = parameters.data();
  RootSignatureDesc.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  ID3DBlob *pRootSignatureBlob;
  ID3DBlob *pErrorBlob;
  D3D12SerializeRootSignature(&RootSignatureDesc,
                              D3D_ROOT_SIGNATURE_VERSION_1_0,
                              &pRootSignatureBlob, &pErrorBlob);
  if (pErrorBlob) {
    LOG_ERROR("D3D12 Root Signature error! %s", pErrorBlob->GetBufferPointer());
  }
  HRESULT Result = device->get_d3d_device()->CreateRootSignature(
      0, pRootSignatureBlob->GetBufferPointer(),
      pRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
  if (FAILED(Result)) {
    LOG_ERROR("Failed to create root signature!");
  }
  pRootSignatureBlob->Release();

  D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = {};
  Desc.VS.pShaderBytecode = vertexBytecode.bytecode.data();
  Desc.VS.BytecodeLength = vertexBytecode.bytecode.size() * sizeof(uint32_t);
  Desc.PS.pShaderBytecode = fragmentBytecode.bytecode.data();
  Desc.PS.BytecodeLength = fragmentBytecode.bytecode.size() * sizeof(uint32_t);
  for (int RTVIndex = 0; RTVIndex < specs.FormatCount; RTVIndex++) {
    Desc.BlendState.RenderTarget[RTVIndex].SrcBlend = D3D12_BLEND_ONE;
    Desc.BlendState.RenderTarget[RTVIndex].DestBlend = D3D12_BLEND_ZERO;
    Desc.BlendState.RenderTarget[RTVIndex].BlendOp = D3D12_BLEND_OP_ADD;
    Desc.BlendState.RenderTarget[RTVIndex].SrcBlendAlpha = D3D12_BLEND_ONE;
    Desc.BlendState.RenderTarget[RTVIndex].DestBlendAlpha = D3D12_BLEND_ZERO;
    Desc.BlendState.RenderTarget[RTVIndex].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    Desc.BlendState.RenderTarget[RTVIndex].LogicOp = D3D12_LOGIC_OP_NOOP;
    Desc.BlendState.RenderTarget[RTVIndex].RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;

    Desc.RTVFormats[RTVIndex] = DXGI_FORMAT(specs.Formats[RTVIndex]);
    Desc.NumRenderTargets = specs.FormatCount;
  }
  Desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
  Desc.RasterizerState.FillMode = D3D12_FILL_MODE(specs.Fill);
  Desc.RasterizerState.CullMode = D3D12_CULL_MODE(specs.Cull);
  Desc.RasterizerState.DepthClipEnable = true;
  Desc.RasterizerState.FrontCounterClockwise = true;
  Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  if (specs.DepthEnabled) {
    Desc.DepthStencilState.DepthEnable = true;
    Desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    Desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC(specs.Depth);
    Desc.DSVFormat = DXGI_FORMAT(specs.DepthFormat);
  }
  Desc.SampleDesc.Count = 1;

  InputElementSemanticNames.reserve(VertexDesc.InputParameters);
  InputElementDescs.reserve(VertexDesc.InputParameters);

  for (int ParameterIndex = 0; ParameterIndex < VertexDesc.InputParameters;
       ParameterIndex++) {
    D3D12_SIGNATURE_PARAMETER_DESC ParameterDesc = {};
    pVertexReflection->GetInputParameterDesc(ParameterIndex, &ParameterDesc);

    InputElementSemanticNames.push_back(ParameterDesc.SemanticName);

    D3D12_INPUT_ELEMENT_DESC InputElement = {};
    InputElement.SemanticName = InputElementSemanticNames.back().c_str();
    InputElement.SemanticIndex = ParameterDesc.SemanticIndex;
    InputElement.InputSlot = 0;
    InputElement.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    InputElement.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    InputElement.InstanceDataStepRate = 0;

    if (ParameterDesc.Mask == 1) {
      if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
        InputElement.Format = DXGI_FORMAT_R32_UINT;
      else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
        InputElement.Format = DXGI_FORMAT_R32_SINT;
      else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
        InputElement.Format = DXGI_FORMAT_R32_FLOAT;
    } else if (ParameterDesc.Mask <= 3) {
      if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
        InputElement.Format = DXGI_FORMAT_R32G32_UINT;
      else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
        InputElement.Format = DXGI_FORMAT_R32G32_SINT;
      else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
        InputElement.Format = DXGI_FORMAT_R32G32_FLOAT;
    } else if (ParameterDesc.Mask <= 7) {
      if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
        InputElement.Format = DXGI_FORMAT_R32G32B32_UINT;
      else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
        InputElement.Format = DXGI_FORMAT_R32G32B32_SINT;
      else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
        InputElement.Format = DXGI_FORMAT_R32G32B32_FLOAT;
    } else if (ParameterDesc.Mask <= 15) {
      if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
        InputElement.Format = DXGI_FORMAT_R32G32B32A32_UINT;
      else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
        InputElement.Format = DXGI_FORMAT_R32G32B32A32_SINT;
      else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
        InputElement.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    }

    InputElementDescs.push_back(InputElement);
  }
  Desc.InputLayout.pInputElementDescs = InputElementDescs.data();
  Desc.InputLayout.NumElements =
      static_cast<uint32_t>(InputElementDescs.size());
  Desc.pRootSignature = m_rootSignature;

  Result = device->get_d3d_device()->CreateGraphicsPipelineState(
      &Desc, IID_PPV_ARGS(&m_pipeline));
  if (FAILED(Result)) {
    LOG_ERROR("D3D12: Failed creating D3D12 graphics pipeline!");
    return;
  }
  pPixelReflection->Release();
  pVertexReflection->Release();
}

D3D12Pipeline::~D3D12Pipeline()
{
    m_rootSignature->Release();
    m_pipeline->Release();
}
} // namespace Yoda