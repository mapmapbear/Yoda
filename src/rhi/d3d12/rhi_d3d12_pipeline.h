#pragma once
#include "d3d12.h"
#include "rhi/rhi_context_commons.h"
#include <memory>
#include <string>
namespace Yoda {
class D3D12Device;
class D3D12Pipeline {
public:
  D3D12Pipeline(std::shared_ptr<D3D12Device> device,
                GraphicsPipelineSpecs &specs);
  ~D3D12Pipeline();

  ID3D12PipelineState *get_pipeline() { return m_pipeline; }
  ID3D12RootSignature *get_root_signature() { return m_rootSignature; }

protected:
  ID3D12PipelineState *m_pipeline;
  ID3D12RootSignature *m_rootSignature;
  std::unordered_map<std::string, int> m_bindings;
};
} // namespace Yoda