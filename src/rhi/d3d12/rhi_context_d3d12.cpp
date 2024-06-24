#pragma once
#include "rhi/d3d12/rhi_context_d3d12.h"
#include "render/buffer.h"
#include "render/copyer.h"
#include "render/texture.h"
#include "rhi/d3d12/rhi_d3d12_allocator.h"
#include "rhi/d3d12/rhi_d3d12_command_list.h"
#include "rhi/d3d12/rhi_d3d12_command_queue.h"
#include "rhi/d3d12/rhi_d3d12_descriptor_heap.h"
#include "rhi/d3d12/rhi_d3d12_device.h"
#include "rhi/d3d12/rhi_d3d12_fence.h"
#include "rhi/d3d12/rhi_d3d12_pipeline.h"
#include "rhi/d3d12/rhi_d3d12_swapchain.h"
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <d3d12.h>
#include <imgui.h>
#include <memory>
#include <vcruntime.h>
#include <windef.h>


namespace Yoda {

RHIContextD3D12::RHIContextD3D12(HWND hwnd) {
  m_device = std::make_shared<D3D12Device>();
  m_alloctor = std::make_shared<D3D12Allocator>(m_device);

  graphic_queue =
      std::make_shared<D3D12CommandQueue>(m_device, CommandQueueType::Graphics);
  compute_queue =
      std::make_shared<D3D12CommandQueue>(m_device, CommandQueueType::Compute);
  copy_queue =
      std::make_shared<D3D12CommandQueue>(m_device, CommandQueueType::Copy);
  heaps.RTVHeap = std::make_shared<D3D12DescroptorHeap>(
      m_device, DescriptorHeapType::RenderTarget, 1024);
  heaps.DSVHeap = std::make_shared<D3D12DescroptorHeap>(
      m_device, DescriptorHeapType::DepthTarget, 1024);
  heaps.ShaderHeap = std::make_shared<D3D12DescroptorHeap>(
      m_device, DescriptorHeapType::ShaderResource, 1'000'000);
  heaps.SamplerHeap = std::make_shared<D3D12DescroptorHeap>(
      m_device, DescriptorHeapType::Sampler, 512);
  graphics_fence = new FencePair;
  compute_fence = new FencePair;
  copy_fence = new FencePair;
  graphics_fence->fence = std::make_shared<D3D12Fence>(m_device);
  compute_fence->fence = std::make_shared<D3D12Fence>(m_device);
  copy_fence->fence = std::make_shared<D3D12Fence>(m_device);
  graphics_fence->value = 0;
  compute_fence->value = 0;
  copy_fence->value = 0;
  m_swapchain = std::make_shared<D3D12Swapchain>(m_device, graphic_queue,
                                                 heaps.RTVHeap, hwnd);
  for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
    command_lists[i] = std::make_shared<D3D12CommandList>(
        m_device, &heaps, CommandQueueType::Graphics);
    frame_values[i] = 0;
  }
  font_descriptor = std::make_shared<Descriptor>(heaps.ShaderHeap->Allocate());
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &IO = ImGui::GetIO();
  IO.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  IO.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad;            // Enable Gamepad Controls
  IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking

  ImGui::StyleColorsDark();
  ImGuiStyle &Style = ImGui::GetStyle();

  if (IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    Style.WindowRounding = 0.0f;
    Style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  ImGui_ImplWin32_EnableDpiAwareness();
  auto deivePtr = m_device->get_d3d_device();
  ImGui_ImplDX12_Init(deivePtr, FRAMES_IN_FLIGHT, DXGI_FORMAT_R8G8B8A8_UNORM,
                      heaps.ShaderHeap->GetHeap(), font_descriptor->CPU,
                      font_descriptor->GPU);
  ImGui_ImplWin32_Init(hwnd);
}
RHIContextD3D12::~RHIContextD3D12() {}

void RHIContextD3D12::begin_frame() {
  frame_index = m_swapchain->AcquireImage();
  graphics_fence->fence->wait(frame_values[frame_index], 10'000'000);
  m_alloctor->get_allocator()->SetCurrentFrameIndex(frame_index);
}
std::shared_ptr<Texture> RHIContextD3D12::get_back_buffer() {
  return m_swapchain->GetTexture(frame_index);
}

std::shared_ptr<D3D12CommandList> RHIContextD3D12::get_current_command_list() {
  return command_lists[frame_index];
}

std::shared_ptr<D3D12Pipeline>
RHIContextD3D12::create_graphics_pipeline(GraphicsPipelineSpecs &specs) {
  return std::make_shared<D3D12Pipeline>(m_device, specs);
}

std::shared_ptr<Buffer> RHIContextD3D12::create_buffer(uint64_t size,
                                                       uint64_t stride,
                                                       BufferType type,
                                                       bool readback) {
  return std::make_shared<Buffer>(m_alloctor, size, stride, type, readback);
}

std::shared_ptr<Copyer> RHIContextD3D12::create_copyer() {
  return std::make_shared<Copyer>(m_device, m_alloctor, &heaps);
}

void RHIContextD3D12::FlushCopyerList(std::shared_ptr<Copyer> copyer) {
  copyer->m_command_list->begin();

  for (auto &command : copyer->m_commands) {
    auto cmd_list = copyer->m_command_list;

    switch (command.type) {
    case Copyer::CopyCommandType::HostToDeviceShared: {
      void *pData;
      command.destBuffer->Map(0, 0, &pData);
      memcpy(pData, command.data, command.size);
      command.destBuffer->Unmap(0, 0);
      break;
    }
    case Copyer::CopyCommandType::BufferToBuffer:
    case Copyer::CopyCommandType::HostToDeviceLocal: {
      cmd_list->copy_buffer_to_buffer(command.destBuffer, command.sourceBuffer);
      break;
    }
    case Copyer::CopyCommandType::TextureToTexture: {
      cmd_list->copy_texture_to_texture(command.destTexture, command.sourceTexture);
      break;
    }
    }
  }

  copyer->m_command_list->end();
  excute_command_list({copyer->m_command_list}, CommandQueueType::Copy);
  WaitForPreviousDeviceSubmit(CommandQueueType::Copy);
  copyer->m_commands.clear();
}

void RHIContextD3D12::excute_command_list(const std::vector<std::shared_ptr<D3D12CommandList>>& buffers, CommandQueueType type)
{
    switch (type) {
        case CommandQueueType::Graphics: {
            graphic_queue->submit(buffers);
            graphics_fence->value = graphics_fence->fence->signal(graphic_queue);
            break;
        }
        case CommandQueueType::Compute:  {
            compute_queue->submit(buffers);
            compute_fence->value = compute_fence->fence->signal(compute_queue);
            break;
        }
        case CommandQueueType::Copy: {
            copy_queue->submit(buffers);
            copy_fence->value = copy_fence->fence->signal(copy_queue);
            break;
        }
    }
}

void RHIContextD3D12::WaitForPreviousDeviceSubmit(CommandQueueType type)
{
    switch (type) {
        case CommandQueueType::Graphics: {
            graphic_queue->wait(graphics_fence->fence, graphics_fence->value);
            break;
        }
        case CommandQueueType::Compute:  {
            compute_queue->wait(compute_fence->fence, compute_fence->value);
            break;
        }
        case CommandQueueType::Copy: {
            copy_queue->wait(copy_fence->fence, copy_fence->value);
            break;
        }
    }
}


} // namespace Yoda
