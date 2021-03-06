#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>
#include <okami/Geometry.hpp>
#include <okami/Transform.hpp>
#include <okami/GraphicsComponents.hpp>

#include <iostream>
#include <marl/defer.h>

using namespace okami::core;
using namespace okami::graphics;

void TestBackend(GraphicsBackend backend) {

    RealtimeGraphicsParams gfxParams;
    gfxParams.mBackend = backend;

    WindowParams windowParams;

    switch (backend) {
        case GraphicsBackend::VULKAN:
            windowParams.mWindowTitle = "okami Diligent-Engine Texture Test (Vulkan)";
            break;
        case GraphicsBackend::D3D11:
            windowParams.mWindowTitle = "okami Diligent-Engine Texture Test (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            windowParams.mWindowTitle = "okami Diligent-Engine Texture Test (D3D12)";
            break;
    }

    ResourceManager resources;
    SystemCollection systems;

    systems.Add(CreateGLFWDisplay(&resources, gfxParams));
    auto display = systems.QueryInterface<IDisplay>();

    systems.Add(CreateRenderer(display, resources));
    auto renderer = systems.QueryInterface<IRenderer>();

    systems.Startup();
    {
        auto window = display->CreateWindow(windowParams);

        auto displayInterface = systems.QueryInterface<IDisplay>();
        auto vertexLayouts = systems.QueryInterface<IVertexLayoutProvider>();
        auto staticMeshLayout = vertexLayouts->GetVertexLayout<StaticMesh>();

        // Create a geometry object from user-specified data
        Geometry::Data<> data;
        data.mPositions = {
            glm::vec3(-0.5f, 0.5f, 0.0f),
            glm::vec3(0.5f, 0.5f, 0.0f),
            glm::vec3(-0.5f, -0.5f, 0.0f),

            glm::vec3(-0.5f, -0.5f, 0.0f),
            glm::vec3(0.5f, 0.5f, 0.0f),
            glm::vec3(0.5f, -0.5f, 0.0f)
        };
        data.mUVs.emplace_back(std::vector<glm::vec2>{
            glm::vec2(0.0f, 0.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(0.0f, 1.0f),

            glm::vec2(0.0f, 1.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f)
        });

        Frame frame;
        resources.Add(&frame);
        
        auto geo = resources.Add(
            Geometry(staticMeshLayout, std::move(data)), frame);

        // Load a texture from disk
        auto texture = resources.Add(Texture("test.png"), frame);

         // Create a material for that texture
        FlatSurface materialSurface;
        materialSurface.mTexture = texture;
        auto material = resources.Add(
            Material<StaticMesh>(materialSurface));

        // Create a mesh at the origin
        auto entity = frame.CreateEntity();
        frame.Emplace<Transform>(entity);
        frame.Emplace<StaticMesh>(entity, StaticMesh{geo, material});

        // Geometry and texture are available to use after this is called.
        systems.SetFrame(frame);
        systems.LoadResources();

        RenderView rv;
        rv.bClear = true;
        rv.mCamera = entt::null;
        rv.mTargetId = window->GetCanvas()->GetResourceId();
        
        Clock clock;
        while (!window->ShouldClose()) {
            renderer->SetRenderView(rv);
            systems.Fork(clock.GetTime());
            systems.Join();
        }

        // Releases all resources used by the frame
        resources.Free(&frame);
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