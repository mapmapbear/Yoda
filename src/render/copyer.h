#pragma once
#include "rhi/d3d12/rhi_context_d3d12.h"
#include <memory>
#include <vector>
namespace Yoda {
class D3D12DescroptorHeap;
class D3D12Device;
class D3D12Allocator;
class D3D12CommandList;
class Texture;
class Buffer;

class Copyer {
public:
  Copyer(std::shared_ptr<D3D12Device> device,
         std::shared_ptr<D3D12Allocator> allocator, struct Heaps* heaps);
  ~Copyer();
  void CopyHostToDeviceLocal(void *pData, uint64_t uiSize,
                             std::shared_ptr<Buffer> pDestBuffer);

protected:
  friend class RHIContextD3D12;
  std::shared_ptr<D3D12Device> m_device;
  std::shared_ptr<D3D12Allocator> m_allocator;
  std::shared_ptr<D3D12CommandList> m_command_list;

  enum class CopyCommandType {
    HostToDeviceShared,
    HostToDeviceLocal,
    BufferToBuffer,
    TextureToTexture
  };

  struct CopyCommand {
    CopyCommandType type;
    void *data;
    uint64_t size;

    std::shared_ptr<Texture> sourceTexture;
    std::shared_ptr<Texture> destTexture;

    std::shared_ptr<Buffer> sourceBuffer;
    std::shared_ptr<Buffer> destBuffer;
  };
  std::vector<CopyCommand> m_commands;
};
} // namespace Yoda