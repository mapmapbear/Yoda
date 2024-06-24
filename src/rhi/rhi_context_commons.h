#pragma once
#include <d3d12.h>
#include <memory>
#include <unordered_map>

namespace Yoda {
class D3D12DescroptorHeap;
struct Heaps {
  std::shared_ptr<D3D12DescroptorHeap> RTVHeap;
  std::shared_ptr<D3D12DescroptorHeap> DSVHeap;
  std::shared_ptr<D3D12DescroptorHeap> ShaderHeap;
  std::shared_ptr<D3D12DescroptorHeap> SamplerHeap;
};

enum class TextureFormat
{
    RGBA8 = DXGI_FORMAT_R8G8B8A8_UNORM,
    RGBA32Float = DXGI_FORMAT_R32G32B32A32_FLOAT,
    RGBA16Float = DXGI_FORMAT_R16G16B16A16_FLOAT,
    R32Depth = DXGI_FORMAT_D32_FLOAT
};

enum class TextureLayout
{
    Common = D3D12_RESOURCE_STATE_COMMON,
    ShaderResource = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
    Storage = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    Depth = D3D12_RESOURCE_STATE_DEPTH_WRITE,
    RenderTarget = D3D12_RESOURCE_STATE_RENDER_TARGET,
    CopySource = D3D12_RESOURCE_STATE_COPY_SOURCE,
    CopyDest = D3D12_RESOURCE_STATE_COPY_DEST,
    Present = D3D12_RESOURCE_STATE_PRESENT,
    DataRead = D3D12_RESOURCE_STATE_GENERIC_READ,
    DataWrite = D3D12_RESOURCE_STATE_COMMON
};

enum class TextureUsage
{
    Copy,
    RenderTarget,
    DepthTarget,
    Storage,
    ShaderResource
};

enum class Topology
{
    LineList = D3D_PRIMITIVE_TOPOLOGY_LINELIST,
    LineStrip = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
    PointList = D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
    TriangleList = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    TriangleStrip = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
};

enum class CommandQueueType {
  Graphics = D3D12_COMMAND_LIST_TYPE_DIRECT,
  Compute = D3D12_COMMAND_LIST_TYPE_COMPUTE,
  Copy = D3D12_COMMAND_LIST_TYPE_COPY
};

enum class PipelineType
{

};

enum class FillMode
{
    Solid = D3D12_FILL_MODE_SOLID,
    Line = D3D12_FILL_MODE_WIREFRAME
};

enum class CullMode
{
    Back = D3D12_CULL_MODE_BACK,
    Front = D3D12_CULL_MODE_FRONT,
    None = D3D12_CULL_MODE_NONE
};

enum class DepthOperation
{
    Greater = D3D12_COMPARISON_FUNC_GREATER,
    Less = D3D12_COMPARISON_FUNC_LESS,
    Equal = D3D12_COMPARISON_FUNC_EQUAL,
    LEqual = D3D12_COMPARISON_FUNC_LESS_EQUAL,
    None = D3D12_COMPARISON_FUNC_NONE
};

enum class ShaderType
{
    Vertex,
    Fragment,
    Compute,
    // TODO(ahi): Mesh
    // TODO(ahi): Raytracing
    // TODO(ahi): Node
};
struct GraphicsPipelineSpecs
{
    FillMode Fill;
    CullMode Cull;
    DepthOperation Depth;

    TextureFormat Formats[32];
    int FormatCount;
    TextureFormat DepthFormat;
    bool DepthEnabled;

    std::unordered_map<ShaderType, struct ShaderBytecode> Bytecodes;
};



class RHIContextCommons {
public:

};

} //namespace Yoda
