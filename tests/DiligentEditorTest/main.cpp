#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>
#include <okami/Geometry.hpp>
#include <okami/Transform.hpp>
#include <okami/GraphicsComponents.hpp>
#include <okami/Camera.hpp>
#include <okami/diligent/Im3dGizmo.hpp>
#include <okami/diligent/FirstPersonCamera.hpp>
#include <marl/defer.h>

using namespace okami::core;
using namespace okami::graphics;

void TestBackend(GraphicsBackend backend) {

    RealtimeGraphicsParams params;
    params.mDeviceType = backend;

    switch (backend) {
        case GraphicsBackend::VULKAN:
            params.mWindowTitle = "okami Editor Test (Vulkan)";
            break;
        case GraphicsBackend::OPENGL:
            params.mWindowTitle = "okami Editor Test (OpenGL)";
            break;
        case GraphicsBackend::D3D11:
            params.mWindowTitle = "okami Editor Test (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            params.mWindowTitle = "okami Editor Test (D3D12)";
            break;
    }

    ResourceManager resources;
    SystemCollection systems;
    auto display = systems.Add(CreateGLFWDisplay(params));
    auto renderer = systems.Add(CreateRenderer(display, resources));
    renderer->EnableInterface<IEntityPick>();

    auto imgui = systems.Add(CreateImGui(renderer, display));
    auto im3d = systems.Add(CreateIm3d(renderer, display));
    auto gizmo = systems.Add(CreateGizmoSystem(im3d));
    auto fpsCamera = systems.Add(CreateFPSCameraSystem(display));
    auto editor = systems.Add(CreateEditorSystem(renderer, display, imgui, im3d, gizmo));

    auto displayInterface = systems.QueryInterface<IDisplay>();

    systems.Startup();
    {
        Frame frame;

        // Create a camera
        auto cameraEntity = frame.CreateEntity();
        frame.Emplace<Camera>(cameraEntity);
        frame.Emplace<Transform>(cameraEntity,
            Transform::LookAt(
                glm::vec3(3.0f, 3.0f, 3.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)));
        frame.Emplace<FirstPersonController>(cameraEntity);
        frame.AddTag<EditorCameraTag>(cameraEntity);

        // Geometry is a available to use after this is called
        systems.SetFrame(frame);
        systems.LoadResources();

        Clock clock;
        while (!displayInterface->ShouldClose()) {
            systems.Fork(clock.GetTime());
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

/*#if GL_SUPPORTED
    TestBackend(GraphicsBackend::OPENGL);
#endif*/

#if D3D11_SUPPORTED
    TestBackend(GraphicsBackend::D3D11);
#endif

#if D3D12_SUPPORTED
    TestBackend(GraphicsBackend::D3D12);
#endif
}