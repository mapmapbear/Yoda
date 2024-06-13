#include "application/sample_app.h"
#include "rhi/d3d12/rhi_context_d3d12.h"
#include <fstream>
#include <memory>

namespace Yoda
{
    SampleApp::SampleApp(const SampleAppConfig& config)
    {
        mShowUI = config.showUI;
        mVsyncOn = config.windowDesc.enableVSync;
    
        if (!config.headless)
        {
            auto windowDesc = config.windowDesc;
            // Create the window
            m_window = Window::create(windowDesc, this);
            // m_window->setWindowIcon(getRuntimeDirectory() / "data/framework/nvidia.ico");
            m_render_context = std::make_shared<RHIContextD3D12>();
            m_render_context->initialize(m_window->getApiHandle());
        }
    }

    SampleApp::~SampleApp()
    {
        m_window.reset();
    }
    int SampleApp::run()
    {
        if (m_window)
        {
            m_window->msgLoop();
        }
        else
        {
            // TODO
        }

        return 0;
    }

    void SampleApp::resizeFrameBuffer(uint32_t width, uint32_t height)
    {
	    if (m_window)
	    {
		    // If we have a window, resize it. This will result in a call
		    // back to handleWindowSizeChange() which in turn will resize the frame buffer.
		    m_window->resize(width, height);
	    }
        else {
            // TODO
        }

    }

    void SampleApp::renderFrame()
    {
        // Render something
    }

    void SampleApp::handleWindowSizeChange()
    {
    }

    void SampleApp::handleRenderFrame()
    {
        renderFrame();
    }
} // namespace Yoda
