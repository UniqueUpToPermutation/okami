#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>
#include <okami/Geometry.hpp>
#include <okami/Transform.hpp>
#include <okami/StaticMesh.hpp>

#include <iostream>
#include <marl/defer.h>

using namespace okami::core;
using namespace okami::graphics;

void TestBackend(GraphicsBackend backend) {

    RealtimeGraphicsParams params;
    params.mDeviceType = backend;

    switch (backend) {
        case GraphicsBackend::VULKAN:
            params.mWindowTitle = "okami Diligent-Engine Texture Test (Vulkan)";
            break;
        case GraphicsBackend::OPENGL:
            params.mWindowTitle = "okami Diligent-Engine Texture Test (OpenGL)";
            break;
        case GraphicsBackend::D3D11:
            params.mWindowTitle = "okami Diligent-Engine Texture Test (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            params.mWindowTitle = "okami Diligent-Engine Texture Test (D3D12)";
            break;
    }

    ResourceInterface resources;
    SystemCollection systems;
    auto display = systems.Add(CreateDisplay(params));
    auto renderer = systems.Add(CreateRenderer(display, resources));

    systems.Startup();
    {
        auto displayInterface = systems.QueryInterface<IDisplay>();
        auto vertexLayouts = systems.QueryInterface<IVertexLayoutProvider>();
        auto staticMeshLayout = vertexLayouts->GetVertexLayout<StaticMesh>();

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
        auto geo = resources.Add(
            Geometry(staticMeshLayout, std::move(data)));

        auto texture = resources.Load<Texture>("test.png");

        BaseMaterial::Data materialData;
        materialData.mAlbedo = texture;

        auto material = resources.Add<BaseMaterial>(
            std::move(materialData));

        Frame frame;
        auto entity = frame.CreateEntity();
        frame.Emplace<Transform>(entity);
        frame.Emplace<StaticMesh>(entity, StaticMesh{geo, material});

        systems.LoadResources(&frame);
        
        while (!displayInterface->ShouldClose()) {
            systems.BeginExecute(&frame, Time{0.0, 0.0});
            systems.EndExecute();
        }
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