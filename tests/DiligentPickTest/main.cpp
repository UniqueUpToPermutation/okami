#include <okami/Okami.hpp>
#include <okami/Graphics.hpp>
#include <okami/Geometry.hpp>
#include <okami/Transform.hpp>
#include <okami/GraphicsComponents.hpp>
#include <okami/Camera.hpp>

#include <okami/diligent/Display.hpp>
#include <okami/diligent/Im3dSystem.hpp>
#include <okami/diligent/ImGuiSystem.hpp>

#include <iostream>
#include <random>

#include <marl/defer.h>

using namespace okami::core;
using namespace okami::graphics;
using namespace okami::graphics::diligent;

void TestBackend(GraphicsBackend backend) {

    RealtimeGraphicsParams params;
    params.mDeviceType = backend;

    switch (backend) {
        case GraphicsBackend::VULKAN:
            params.mWindowTitle = "okami Diligent-Engine Entity Pick Test (Vulkan)";
            break;
        case GraphicsBackend::OPENGL:
            params.mWindowTitle = "okami Diligent-Engine Entity Pick Test (OpenGL)";
            break;
        case GraphicsBackend::D3D11:
            params.mWindowTitle = "okami Diligent-Engine Entity Pick Test (D3D11)";
            break;
        case GraphicsBackend::D3D12:
            params.mWindowTitle = "okami Diligent-Engine Entity Pick Test (D3D12)";
            break;
    }

    ResourceInterface resources;
    SystemCollection systems;
    auto display = systems.Add(CreateGLFWDisplay(params));
    auto renderer = systems.Add(CreateRenderer(display, resources));
    renderer->Request<IEntityPick>();
    
    auto glfwInterface = systems.QueryInterface<IGLFWWindowProvider>();
    auto displayInterface = systems.QueryInterface<IDisplay>();

    // The order in which these are created determines the order in
    // which their respective overlays are drawn.
    systems.Add(CreateIm3d(renderer));
    systems.Add(CreateImGui(renderer, display));
    
    systems.Startup();
    {
        entt::entity selectedEntity = entt::null;
        auto entityPick = systems.QueryInterface<IEntityPick>();
        auto vertexLayouts = systems.QueryInterface<IVertexLayoutProvider>();
        auto staticMeshLayout = vertexLayouts->GetVertexLayout<StaticMesh>();

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

        auto createStaticMesh = [&](float x, float y, float z) {
            auto staticMeshEntity = frame.CreateEntity();
            frame.Emplace<Transform>(staticMeshEntity,
                Transform()
                    .SetTranslate(x, y, z)
                    .SetRotation(
                        glm::angleAxis(glm::pi<float>() / 2.0f, 
                        glm::vec3(0.0, 1.0, 0.0))));
            frame.Emplace<StaticMesh>(staticMeshEntity, 
                StaticMesh{geo, material});
        };

        createStaticMesh(0.0, 0.0, 0.0);
        createStaticMesh(-3.0, 0.0, 3.0);
        createStaticMesh(3.0, 0.0, 3.0);

        // Create a camera
        auto cameraEntity = frame.CreateEntity();
        auto& camera = frame.Emplace<okami::core::Camera>(cameraEntity);
        auto& cameraTransform = frame.Emplace<Transform>(cameraEntity);
        cameraTransform.mTranslation = glm::vec3(0.0f, 2.0f, -4.0f);

        // Geometry and texture are available to use after this is called.
        systems.SetFrame(frame);
        systems.LoadResources();

        auto im3dInterface = systems.QueryInterface<IIm3dCallback>();
        im3dInterface->Add([&selectedEntity, &frame, &geo]() {
            if (selectedEntity != entt::null &&
                selectedEntity != entt::entity(0)) {
                auto transform = frame.TryGet<Transform>(selectedEntity);
                if (transform) {
                    auto aabb = transform->ApplyToAABB(
                         geo->GetBoundingBox());
                    Im3d::SetSize(3.0f);
                    Im3d::DrawAlignedBox(
                        ToIm3d(aabb.mLower), ToIm3d(aabb.mUpper));
                }
            }
        });

        auto imguiInterface = systems.QueryInterface<IImGuiCallback>();
        imguiInterface->Add([&selectedEntity]() {
            std::stringstream ss;
            ss << "Pick Result: ";
            if (selectedEntity == entt::null || 
                (entt::id_type)selectedEntity == 0)
                ss << "null";
            else
                ss << (entt::id_type)selectedEntity;
            ImGui::Begin("Picking");
            ImGui::Text(ss.str().c_str());
            ImGui::End();
        });

        Future<entt::entity> queryResult;
        glfwInterface->AddMouseButtonCallback([&queryResult, &entityPick](
            GLFWwindow* window, int button, int action, int mods) {

            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            queryResult = entityPick->Pick(xpos, ypos);

            return false;
        });

        Clock clock;
        while (!displayInterface->ShouldClose()) {
            auto time = clock.GetTime();

            systems.Fork(time);
            systems.Join();

            if (queryResult) {
                selectedEntity = queryResult.Get();
                queryResult = Future<entt::entity>();
            }
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
    //TestBackend(GraphicsBackend::OPENGL);
#endif

#if D3D11_SUPPORTED
    TestBackend(GraphicsBackend::D3D11);
#endif

#if D3D12_SUPPORTED
    TestBackend(GraphicsBackend::D3D12);
#endif
}