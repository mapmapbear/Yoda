#include "rhi/d3d12/rhi_d3d12_command_list.h"
#include "core/logger.h"
#include "render/buffer.h"
#include "render/texture.h"
#include "rhi/d3d12/rhi_d3d12_allocator.h"
#include "rhi/d3d12/rhi_d3d12_descriptor_heap.h"
#include "rhi/d3d12/rhi_d3d12_device.h"
#include "rhi/rhi_context_commons.h"
#include <d3d12.h>
#include <memory>

namespace Yoda {
D3D12CommandList::D3D12CommandList(std::shared_ptr<D3D12Device> device,
                                   Heaps *heaps, CommandQueueType type) {
  HRESULT result = device->get_d3d_device()->CreateCommandAllocator(
      (D3D12_COMMAND_LIST_TYPE)type, IID_PPV_ARGS(&m_command_allocator));
  if (FAILED(result)) {
    LOG_ERROR("D3D12: Failed to create command allocator!");
  }
}
D3D12CommandList::~D3D12CommandList() {
  m_command_list->Release();
  m_command_allocator->Release();
}

void D3D12CommandList::begin() {
  m_command_allocator->Reset();
  m_command_list->Reset(m_command_allocator, nullptr);
}

void D3D12CommandList::end() { m_command_list->Close(); }

void D3D12CommandList::clear_rendertarget(std::shared_ptr<Texture> renderTarget,
                                          float r, float g, float b, float a) {
  float clearValues[4] = {r, g, b, a};
  m_command_list->ClearRenderTargetView(renderTarget->m_rtv->CPU, clearValues,
                                        0, nullptr);
}

void D3D12CommandList::resource_barrier(std::shared_ptr<Texture> texture,
                                        TextureLayout new_layout) {
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = texture->get_resource()->Resource;
  barrier.Transition.StateBefore = texture->get_state();
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATES(new_layout);
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  if (barrier.Transition.StateBefore == barrier.Transition.StateAfter)
    return;

  m_command_list->ResourceBarrier(1, &barrier);

  texture->set_state(D3D12_RESOURCE_STATES(new_layout));
}

void D3D12CommandList::set_viewport(float x, float y, float width,
                                    float height) {
  D3D12_VIEWPORT Viewport = {};
  Viewport.Width = width;
  Viewport.Height = height;
  Viewport.MinDepth = 0.0f;
  Viewport.MaxDepth = 1.0f;
  Viewport.TopLeftX = x;
  Viewport.TopLeftY = y;

  D3D12_RECT Rect;
  Rect.right = width;
  Rect.bottom = height;
  Rect.top = 0.0f;
  Rect.left = 0.0f;

  m_command_list->RSSetViewports(1, &Viewport);
  m_command_list->RSSetScissorRects(1, &Rect);
}

void D3D12CommandList::set_topology(Topology topology) {
  m_command_list->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY(topology));
}
void D3D12CommandList::bind_rendertarget(
    std::vector<std::shared_ptr<Texture>> colorRTs,
    std::shared_ptr<Texture> depthRT) {
  std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvDescriptors;
  D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor;

  for (auto &renderTarget : colorRTs) {
    rtvDescriptors.push_back(renderTarget->m_rtv->CPU);
  }
  if (depthRT) {
    dsvDescriptor = depthRT->m_dsv->CPU;
  }

  m_command_list->OMSetRenderTargets(rtvDescriptors.size(),
                                     rtvDescriptors.data(), false,
                                     depthRT ? &dsvDescriptor : nullptr);
}

void D3D12CommandList::bind_graphics_pipeline(
    std::shared_ptr<D3D12Pipeline> pipeline) {
  m_command_list->SetPipelineState(pipeline->get_pipeline());
  m_command_list->SetGraphicsRootSignature(pipeline->get_root_signature());
}

void D3D12CommandList::copy_buffer_to_buffer(std::shared_ptr<Buffer> dst,
                                             std::shared_ptr<Buffer> src) {
  m_command_list->CopyResource(dst->m_resource->Resource,
                               src->m_resource->Resource);
}

void D3D12CommandList::copy_texture_to_texture(std::shared_ptr<Texture> dst,
                                               std::shared_ptr<Texture> src) {
  D3D12_TEXTURE_COPY_LOCATION BlitSource = {};
  BlitSource.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  BlitSource.pResource = src->get_resource()->Resource;
  BlitSource.SubresourceIndex = 0;

  D3D12_TEXTURE_COPY_LOCATION BlitDest = {};
  BlitDest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  BlitDest.pResource = dst->get_resource()->Resource;
  BlitDest.SubresourceIndex = 0;

  m_command_list->CopyTextureRegion(&BlitDest, 0, 0, 0, &BlitSource, nullptr);
}

} // namespace Yoda