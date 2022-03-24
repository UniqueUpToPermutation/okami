#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>
#include <okami/Geometry.hpp>
#include <okami/Transform.hpp>
#include <okami/GraphicsComponents.hpp>
#include <okami/Camera.hpp>

#include <iostream>
#include <marl/defer.h>

#include <im3d.h>

using namespace okami::core;
using namespace okami::graphics;

void TestBackend(GraphicsBackend backend) {

    RealtimeGraphicsParams gfxParams;
    gfxParams.mBackend = backend;

    WindowParams wndParams;

    switch (backend) {
        case GraphicsBackend::VULKAN:
            wndParams.mWindowTitle = "okami Transform Gizmo Test (Vulkan)";
            break;
        case GraphicsBackend::D3D11:
            wndParams.mWindowTitle = "okami Transform Gizmo Test (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            wndParams.mWindowTitle = "okami Transform Gizmo Test (D3D12)";
            break;
    }

    ResourceManager resources;
    SystemCollection systems;
    
    systems.Add(CreateGLFWDisplay(&resources, gfxParams));
    auto display = systems.QueryInterface<IDisplay>();

    systems.Add(CreateRenderer(display, resources));
    auto renderer = systems.QueryInterface<IRenderer>();
    
    systems.Add(CreateIm3d(renderer));
    auto im3d = systems.QueryInterface<IIm3dSystem>();
    
    auto gizmo = systems.Add(CreateGizmoSystem(im3d));
    systems.Add(CreateFPSCameraSystem(display));

    auto displayInterface = systems.QueryInterface<IDisplay>();
    auto input = systems.QueryInterface<diligent::IGLFWWindowProvider>();
    auto gizmoInterface = systems.QueryInterface<IGizmo>();

    input->AddKeyCallback([gizmoInterface](GLFWwindow* window, 
        int key, int scancode, int action, int mods) {

        if (action == GLFW_PRESS) {
            if (key == GLFW_KEY_1) {
                gizmoInterface->SetMode(GizmoSelectionMode::TRANSLATION);
            } else if (key == GLFW_KEY_2) {
                gizmoInterface->SetMode(GizmoSelectionMode::SCALE);
            } else if (key == GLFW_KEY_3) {
                gizmoInterface->SetMode(GizmoSelectionMode::ROTATION);
            }
        }

        return false;
    });

    systems.Startup();
    {
        auto displayInterface = systems.QueryInterface<IDisplay>();
        auto vertexLayouts = systems.QueryInterface<IVertexLayoutProvider>();
        auto staticMeshLayout = vertexLayouts->GetVertexLayout<StaticMesh>();

        // Create a static mesh using the specified geometry
        // Create a geometry object from a built-in prefab
        auto geo = resources.Add(Geometry::Prefabs::MaterialBall(staticMeshLayout));
        // Load a texture from disk
        auto texture = resources.Load<Texture>("test.png");

        // Create a material for that texture
        BaseMaterial::Data materialData;
        materialData.mAlbedo = texture;
        auto material = resources.Add<BaseMaterial>(
            std::move(materialData));

        // Create a frame with the static mesh at the origin
        Frame frame;
        auto staticMeshEntity = frame.CreateEntity();
        auto& meshTransform = frame.Emplace<Transform>(staticMeshEntity);
        frame.AddTag<GizmoSelectTag>(staticMeshEntity);
        frame.Emplace<StaticMesh>(staticMeshEntity, StaticMesh{geo, material});

        // Create a camera
        auto cameraEntity = frame.CreateEntity();
        frame.Emplace<Camera>(cameraEntity);
        frame.Emplace<Transform>(cameraEntity,
            Transform::LookAt(
                glm::vec3(3.0f, 3.0f, 3.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)));
        frame.Emplace<FirstPersonController>(cameraEntity);

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

#if GL_SUPPORTED
    TestBackend(GraphicsBackend::OPENGL);
#endif

#if D3D11_SUPPORTED
    TestBackend(GraphicsBackend::D3D11);
#endif

#if D3D12_SUPPORTED
    TestBackend(GraphicsBackend::D3D12);
#endif
}