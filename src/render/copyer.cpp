#include "copyer.h"
#include "render/buffer.h"
#include "rhi/d3d12/rhi_d3d12_command_list.h"
#include <memory>

namespace Yoda {
Copyer::Copyer(std::shared_ptr<D3D12Device> device,
               std::shared_ptr<D3D12Allocator> allocator, struct Heaps *heaps)
    : m_device(device), m_allocator(allocator) {
  m_command_list =
      std::make_shared<D3D12CommandList>(device, heaps, CommandQueueType::Copy);
}
Copyer::~Copyer() {}
void Copyer::CopyHostToDeviceLocal(void* pData, uint64_t uiSize, std::shared_ptr<Buffer> pDestBuffer)
{
    std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>(m_allocator, uiSize, 0, BufferType::Copy, false);

    {
        CopyCommand command;
        command.type = CopyCommandType::HostToDeviceShared;
        command.data = pData;
        command.size = uiSize;
        command.destBuffer = buffer;

        m_commands.push_back(command);
    }
    {
        CopyCommand command;
        command.type = CopyCommandType::HostToDeviceLocal;
        command.sourceBuffer = buffer;
        command.destBuffer = pDestBuffer;

        m_commands.push_back(command);
    }
}
} // namespace Yoda