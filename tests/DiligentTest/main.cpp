#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>
#include <okami/Camera.hpp>

#include <iostream>
#include <marl/defer.h>

using namespace okami::core;
using namespace okami::graphics;

void TestBackend(GraphicsBackend backend) {

    RealtimeGraphicsParams graphicsParams;
    WindowParams windowParams;

    graphicsParams.mBackend = backend;

    switch (backend) {
        case GraphicsBackend::VULKAN:
            windowParams.mWindowTitle = "okami Diligent-Engine Test (Vulkan)";
            break;
        case GraphicsBackend::D3D11:
            windowParams.mWindowTitle = "okami Diligent-Engine Test (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            windowParams.mWindowTitle = "okami Diligent-Engine Test (D3D12)";
            break;
    }

    ResourceInterface resources;
    SystemCollection systems;
    systems.Add(CreateGLFWDisplay(&resources));
    auto display = systems.QueryInterface<IDisplay>();

    systems.Add(CreateRenderer(display, resources));
    auto renderer = systems.QueryInterface<IRenderer>();

    systems.Startup();
    {
        auto window = display->CreateWindow(windowParams);

        Frame frame;
        systems.SetFrame(frame);
        systems.LoadResources();

        RenderView view;
        view.bClear = true;
        view.mCamera = entt::null;
        view.mTarget = window->GetCanvas();
        
        while (!window->ShouldClose()) {
            renderer->SetRenderView(view);
            systems.Fork(Time{0.0, 0.0});
            systems.Join();
        }
    }
    systems.Shutdown();
}

int main() {
    Meta::Register();

    marl::Scheduler scheduler(marl::Scheduler::Config::allCores());
    scheduler.bind();
    defer(scheduler.unbind());

#if VULKAN_SUPPORTED && !PLATFORM_MACOS
    TestBackend(GraphicsBackend::VULKAN);
#endif

#if D3D11_SUPPORTED
    TestBackend(GraphicsBackend::D3D11);
#endif

#if D3D12_SUPPORTED
    TestBackend(GraphicsBackend::D3D12);
#endif
}