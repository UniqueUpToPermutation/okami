#include <okami/diligent/Im3dGizmo.hpp>
#include <okami/diligent/ImGuiSystem.hpp>
#include <okami/diligent/Im3dSystem.hpp>
#include <okami/diligent/Display.hpp>
#include <okami/diligent/Im3dGizmo.hpp>

using namespace okami::core;
using namespace okami::graphics::diligent;

namespace okami::graphics::diligent {
    
    class EditorSystem : 
        public ISystem {
    public:
        IGLFWWindowProvider* mGlfw;
        IImGuiCallback* mImgui;
        IIm3dCallback* mIm3d;
        IEntityPick* mEntityPick;
        IGizmo* mGizmo;
        
        delegate_handle_t mImguiDelegate;
        delegate_handle_t mIm3dDelegate;

        UpdaterReads<EditorCameraTag, Transform> mReads;

        marl::Event mOnReadFinished;

        Transform mEditorCameraTransform;

        float mGridSpacing = 1.0f;
        int mGridSize = 20;

        inline EditorSystem(
            IEntityPick* entityPick,
            IGLFWWindowProvider* glfw,
            IImGuiCallback* imgui,
            IIm3dCallback* im3d,
            IGizmo* gizmo) :
            mEntityPick(entityPick),
            mGlfw(glfw),
            mImgui(imgui),
            mIm3d(im3d),
            mGizmo(gizmo) {
        }
        
        void RenderImGui() {
            if (ImGui::BeginMainMenuBar()) {

                if (ImGui::BeginMenu("File")) {
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Edit")) {
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Selection")) {
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("View")) {
                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }
        }

        void DrawGrid() {
            glm::vec3 center;
            center.y = 0.0f;
            center.x = std::floor(mEditorCameraTransform.mTranslation.x / mGridSpacing);
            center.z = std::floor(mEditorCameraTransform.mTranslation.z / mGridSpacing);
            
            glm::vec3 dx(mGridSpacing, 0.0f, 0.0f);
            glm::vec3 dz(0.0f, 0.0f, mGridSpacing);

            Im3d::BeginLines();

            for (int i = -mGridSize; i <= mGridSize; ++i) {
                Im3d::Vertex(ToIm3d(center + (float)i * dx + (float)mGridSize * dz));
                Im3d::Vertex(ToIm3d(center + (float)i * dx - (float)mGridSize * dz));
            }

            for (int i = -mGridSize; i <= mGridSize; ++i) {
                Im3d::Vertex(ToIm3d(center + (float)i * dz + (float)mGridSize * dx));
                Im3d::Vertex(ToIm3d(center + (float)i * dz - (float)mGridSize * dx));
            }

            Im3d::End();
        }

        void RenderIm3d() {
            DrawGrid();
        }

        void Startup(marl::WaitGroup& waitGroup) override {
            mImguiDelegate = mImgui->Add([this]() {
                RenderImGui();
            });
            mIm3dDelegate = mIm3d->Add([this]() {
                mOnReadFinished.wait();
                RenderIm3d();
            });
        }

        void RegisterInterfaces(InterfaceCollection& interfaces) override {
        }

        void Shutdown() override {
            mImgui->Remove(mImguiDelegate);
            mIm3d->Remove(mIm3dDelegate);
        }

        void LoadResources(marl::WaitGroup& waitGroup) override {
        }

        void SetFrame(Frame& frame) override {
        }

        void RequestSync(SyncObject& syncObject) override {
            mReads.RequestSync(syncObject);
        }

        void Fork(Frame& frame,
            SyncObject& syncObject,
            const Time& time) override {

            marl::schedule([this, &frame]() {
                defer(mOnReadFinished.signal());
                defer(mReads.ReleaseHandles());

                entt::entity mCamera = entt::null;

                mReads.Read<MultiRead<EditorCameraTag, Transform>>([&]() {
                    auto cameras = frame.Registry().view<EditorCameraTag>();

                    if (cameras.empty()) {
                        std::cout << "Warning: There is no camera in this "
                            "scene with EditorCameraTag assigned!" << std::endl;
                    } else {
                        auto transform = frame.Registry().try_get<Transform>(cameras.front());

                        if (transform) {
                            mEditorCameraTransform = *transform;
                        } else {
                            mEditorCameraTransform = Transform();
                        }
                    }
                });
            });
        }

        void Join(Frame& frame) override {
            Wait();
        }

        void Wait() override {
        }

        void EnableInterface(const entt::meta_type& interfaceType) override {
        }
    };
    
}

namespace okami::graphics {
    std::unique_ptr<ISystem> CreateEditorSystem(
        ISystem* entityPick,
        ISystem* input,
        ISystem* imgui,
        ISystem* im3d,
        ISystem* gizmo
    ) {
        IEntityPick* _entityPick = entityPick->QueryInterface<IEntityPick>();
        IGLFWWindowProvider* _input = input->QueryInterface<IGLFWWindowProvider>();
        IImGuiCallback* _imgui = imgui->QueryInterface<IImGuiCallback>();
        IIm3dCallback* _im3d = im3d->QueryInterface<IIm3dCallback>();
        IGizmo* _gizmo = gizmo->QueryInterface<IGizmo>();

        if (!_entityPick) 
            throw std::runtime_error("entityPick does not implement IEntityPick!");
        if (!_input)
            throw std::runtime_error("input does not implement IGLFWWindowProvider!");
        if (!_imgui)
            throw std::runtime_error("imgui does not implement IImGuiCallback!");
        if (!_im3d)
            throw std::runtime_error("im3d does not implement IIm3dCallback!");
        if (!_gizmo)
            throw std::runtime_error("gizmo does not implement IGizmo!");

        return std::make_unique<EditorSystem>(
            _entityPick, _input, _imgui, _im3d, _gizmo);
    }
}