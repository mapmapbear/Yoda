#pragma once
#include "rhi_context_commons.h"
#include <Windows.h>

namespace Yoda {
    class RHIContext : public RHIContextCommons
    {
    public:
        virtual size_t initialize(HWND handle) = 0;
        virtual void begin_frame() = 0;
    };
}
