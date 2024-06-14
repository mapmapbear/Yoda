#pragma once
#include "rhi_context_commons.h"
#include <Windows.h>

namespace Yoda {
    class RHIContext : public RHIContextCommons
    {
    public:
        virtual size_t initialize(HWND handle) = 0;
        // virtual size_t command_queue_create(CommandQueueType type) = 0;
    };
}
