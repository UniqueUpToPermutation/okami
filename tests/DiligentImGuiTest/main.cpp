#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>
#include <okami/Geometry.hpp>
#include <okami/Transform.hpp>
#include <okami/GraphicsComponents.hpp>
#include <okami/Camera.hpp>

#include <iostream>
#include <random>

#include <marl/defer.h>
#include <imgui.h>

using namespace okami::core;
using namespace okami::graphics;

void TestBackend(GraphicsBackend backend) {

    RealtimeGraphicsParams gfxParams;
    gfxParams.mBackend = backend;

    WindowParams windowParams;

    switch (backend) {
        case GraphicsBackend::VULKAN:
            windowParams.mWindowTitle = "okami Diligent-Engine ImGui Test (Vulkan)";
            break;
        case GraphicsBackend::D3D11:
            windowParams.mWindowTitle = "okami Diligent-Engine ImGui Test (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            windowParams.mWindowTitle = "okami Diligent-Engine ImGui Test (D3D12)";
            break;
    }

    ResourceInterface resources;
    SystemCollection systems;

    systems.Add(CreateGLFWDisplay(&resources, gfxParams));
    auto display = systems.QueryInterface<IDisplay>();

    systems.Add(CreateRenderer(display, resources));
    auto renderer = systems.QueryInterface<IRenderer>();

    systems.Add(CreateImGui(display, renderer));
    auto imgui = systems.QueryInterface<IImGuiSystem>();

    systems.Startup();
    {
        auto mainWindow = display->CreateWindow(windowParams);

        windowParams.mBackbufferWidth = 1024;
        windowParams.mBackbufferHeight = 756;
        windowParams.mWindowTitle = "Secondary Window";
        windowParams.bIsPrimary = false;
        auto secondaryWindow = display->CreateWindow(windowParams);

        bool bShowDemoWindow1 = true;
        bool bShowDemoWindow2 = true;
        
        imgui->AddOverlayTo(mainWindow);
        imgui->AddOverlayTo(secondaryWindow);

        imgui->Add(mainWindow, [&bShowDemoWindow1]() {
            if (bShowDemoWindow1)
                ImGui::ShowDemoWindow(&bShowDemoWindow1);
        });

        imgui->Add(secondaryWindow, [&bShowDemoWindow2]() {
            if (bShowDemoWindow2)
                ImGui::ShowDemoWindow(&bShowDemoWindow2);
        });

        // Geometry and texture are available to use after this is called.
        Frame frame;
        systems.SetFrame(frame);
        systems.LoadResources();

        RenderView rv1;
        rv1.bClear = true;
        rv1.mCamera = entt::null;
        rv1.mTarget = mainWindow->GetCanvas();

        RenderView rv2;
        rv2.bClear = true;
        rv2.mCamera = entt::null;
        rv2.mTarget = secondaryWindow->GetCanvas();

        Clock clock;
        while (!mainWindow->ShouldClose()) {
            auto time = clock.GetTime();

            if (secondaryWindow && secondaryWindow->ShouldClose()) {
                secondaryWindow->Close();
                secondaryWindow = nullptr;
            }

            if (secondaryWindow != nullptr)
                renderer->SetRenderViews({rv1, rv2});
            else 
                renderer->SetRenderView(rv1);
                
            systems.Fork(time);
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