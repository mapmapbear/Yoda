#pragma once
#include "rhi/d3d12/rhi_context_d3d12.h"
#include "core/logger.h"
#include "render/texture.h"
#include "rhi/d3d12/rhi_d3d12_allocator.h"
#include "rhi/d3d12/rhi_d3d12_command_list.h"
#include "rhi/d3d12/rhi_d3d12_command_queue.h"
#include "rhi/d3d12/rhi_d3d12_descriptor_heap.h"
#include "rhi/d3d12/rhi_d3d12_device.h"
#include "rhi/d3d12/rhi_d3d12_fence.h"
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

  graphics_queue =
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
  m_swapchain = std::make_shared<D3D12Swapchain>(m_device, graphics_queue,
                                                 heaps.RTVHeap, hwnd);
  for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
    command_lists[i] = std::make_shared<D3D12CommandList>(
        m_device, heaps, CommandQueueType::Graphics);
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

std::shared_ptr<D3D12CommandList> RHIContextD3D12::get_current_command_list() 
{
  return command_lists[frame_index];
}

} // namespace Yoda
