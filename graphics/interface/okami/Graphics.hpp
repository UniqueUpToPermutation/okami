#pragma once

#include <okami/System.hpp>

namespace okami::graphics {

    class IWindow {
    public:
        virtual bool ShouldClose() const = 0;
    };

    constexpr uint DEFAULT_WINDOW_WIDTH = 1920;
    constexpr uint DEFAULT_WINDOW_HEIGHT = 1080;

    enum class GraphicsBackend {
        VULKAN,
        OPENGL,
        D3D11,
        D3D12
    };

    constexpr GraphicsBackend DEFAULT_BACKEND = GraphicsBackend::VULKAN;

    struct RealtimeGraphicsParams {
		std::string mWindowTitle 	= "okami";
		uint mBackbufferWidth 	    = DEFAULT_WINDOW_WIDTH;
		uint mBackbufferHeight 		= DEFAULT_WINDOW_HEIGHT;
		bool bFullscreen 			= false;
		bool bShowOnCreation		= true;
		GraphicsBackend mDeviceType = DEFAULT_BACKEND;
	};

    std::unique_ptr<core::ISystem> CreateGLFWDisplay(
        const RealtimeGraphicsParams& params = RealtimeGraphicsParams());
    inline std::unique_ptr<core::ISystem> CreateDisplay(
        const RealtimeGraphicsParams& params  = RealtimeGraphicsParams()) {
        return CreateGLFWDisplay(params);
    }

    std::unique_ptr<core::ISystem> CreateRenderer(core::ISystem* displaySystem);
}