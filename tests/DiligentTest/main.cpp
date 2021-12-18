#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>

#include <iostream>
#include <marl/defer.h>

using namespace okami::core;
using namespace okami::graphics;

void TestBackend(GraphicsBackend backend) {

    RealtimeGraphicsParams params;
    params.mDeviceType = backend;

    switch (backend) {
        case GraphicsBackend::VULKAN:
            params.mWindowTitle = "okami (Vulkan)";
            break;
        case GraphicsBackend::OPENGL:
            params.mWindowTitle = "okami (OpenGL)";
            break;
        case GraphicsBackend::D3D11:
            params.mWindowTitle = "okami (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            params.mWindowTitle = "okami (D3D12)";
            break;
    }

    SystemCollection systems;
    auto display = systems.Add(CreateDisplay(params));
    auto renderer = systems.Add(CreateRenderer(display));

    Frame frame;

    systems.Startup();

    auto window = systems.QueryInterface<IDisplay>();
    systems.LoadResources(&frame);
    
    while (!window->ShouldClose()) {
        systems.BeginExecute(&frame, Time{0.0, 0.0});
        systems.EndExecute();
    }

    systems.Shutdown();
}

int main() {
    Meta::Register();

    marl::Scheduler scheduler(marl::Scheduler::Config::allCores());
    scheduler.bind();
    defer(scheduler.unbind());

#if GL_SUPPORTED
    TestBackend(GraphicsBackend::OPENGL);
#endif

#if VULKAN_SUPPORTED
    TestBackend(GraphicsBackend::VULKAN);
#endif

#if D3D11_SUPPORTED
    TestBackend(GraphicsBackend::D3D11);
#endif

#if D3D12_SUPPORTED
    TestBackend(GraphicsBackend::D3D12);
#endif
}