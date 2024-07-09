#pragma once
#include "render/buffer.h"
#include "rhi/d3d12/rhi_d3d12_pipeline.h"
#include "rhi/rhi_context_commons.h"
#include <d3dx12.h>
#include <memory>
#include <vector>

namespace Yoda {
class D3D12Device;
class Texture;
class D3D12Pipeline;
class Buffer;

class D3D12CommandList {
public:
  D3D12CommandList(std::shared_ptr<D3D12Device> device, Heaps *heaps,
                   CommandQueueType type);
  ~D3D12CommandList();
  void begin();
  void end();
  void set_viewport(float x, float y, float width, float height);
  void clear_rendertarget(std::shared_ptr<Texture> renderTarget, float r,
                          float g, float b, float a);
  void resource_barrier(std::shared_ptr<Texture> texture,
                        TextureLayout new_layout);
  void set_topology(Topology topology);
  void bind_rendertarget(std::vector<std::shared_ptr<Texture>> colorRTs,
                         std::shared_ptr<Texture> depthRT);
  void bind_graphics_pipeline(std::shared_ptr<D3D12Pipeline> pipeline);
  void copy_buffer_to_buffer(std::shared_ptr<Buffer> dst,
                             std::shared_ptr<Buffer> src);
  void copy_texture_to_texture(std::shared_ptr<Texture> dst,
                               std::shared_ptr<Texture> src);
  ID3D12GraphicsCommandList *get_commandlist() { return m_command_list; }

protected:
  Heaps m_heaps;
  D3D12_COMMAND_LIST_TYPE m_type;
  ID3D12GraphicsCommandList
      *m_command_list; // TODO(ahi): Switch to newer version of command list to
                       // get access to DXR, Mesh shaders and Work Graphs
  ID3D12CommandAllocator *m_command_allocator;
};
} // namespace Yoda