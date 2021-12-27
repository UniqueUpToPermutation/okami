#include <okami/diligent/ImGuiSystem.hpp>
#include <okami/diligent/Display.hpp>

#include <ImGuiImplDiligent.hpp>
#include <backends/imgui_impl_glfw.h>

namespace okami::graphics::diligent {

    void ImGuiRenderOverlay::Startup(
        core::ISystem* renderer,
        DG::IRenderDevice* device, 
        DG::ISwapChain* swapChain) {
        auto rtv = swapChain->GetCurrentBackBufferRTV();
        auto dsv = swapChain->GetDepthBufferDSV();

        mImGuiImpl = std::make_unique<DG::ImGuiImplDiligent>(device, 
            rtv->GetDesc().Format, dsv->GetDesc().Format);
        mSurfaceTransform = swapChain->GetDesc().PreTransform;
    }

    void ImGuiRenderOverlay::QueueCommands(DG::IDeviceContext* context) {
        if (bDraw)
            mImGuiImpl->Render(context);

        bDraw = false;
    }
    
    void ImGuiRenderOverlay::Shutdown() {
        mImGuiImpl.reset();
    }

    ImGuiSystem::ImGuiSystem(IRenderer* renderer, core::ISystem* input) :
        mRenderer(renderer), 
        mInputSystem(input) {

        mRenderer->AddOverlay(&mOverlay);
    }

    ImGuiSystem::~ImGuiSystem() {
        mRenderer->RemoveOverlay(&mOverlay);
    }

    core::delegate_handle_t ImGuiSystem::Add(immedate_callback_t callback) {
        return mOnUpdate.Add(std::move(callback));
    }

    void ImGuiSystem::Remove(core::delegate_handle_t handle) {
        mOnUpdate.Remove(handle);
    }

    marl::WaitGroup& ImGuiSystem::GetUpdateWaitGroup() {
        return mUpdateWaitGroup;
    }

    void ImGuiSystem::Startup(marl::WaitGroup& waitGroup) {
        core::InterfaceCollection interfaces;
        mInputSystem->RegisterInterfaces(interfaces);

        auto displayGLFW = interfaces.Query<IGLFWWindowProvider>();

        if (displayGLFW) {
            // For high DPI stuff
            float xscale, yscale;
            glfwGetWindowContentScale(displayGLFW->GetWindowGLFW(), &xscale, &yscale);
            ImGui::GetIO().FontGlobalScale = (xscale + yscale) / 2.0;

            // We have a glfw display!
            ImGui_ImplGlfw_InitForOther(displayGLFW->GetWindowGLFW(), true);
        } else {
            throw std::runtime_error("Input system does not implement any supported interfaces!");
        }
    }

    void ImGuiSystem::RegisterInterfaces(core::InterfaceCollection& interfaces) {
        interfaces.Add<IImGuiCallback>(this);
    }

    void ImGuiSystem::Shutdown() {
        core::InterfaceCollection interfaces;
        mInputSystem->RegisterInterfaces(interfaces);

        auto displayGLFW = interfaces.Query<IGLFWWindowProvider>();

        if (displayGLFW) {
            // We have a glfw display!
            ImGui_ImplGlfw_Shutdown();
        } else {
            throw std::runtime_error("Input system does not implement any supported interfaces!");
        }
    }

    void ImGuiSystem::LoadResources(marl::WaitGroup& waitGroup) {
    }
    void ImGuiSystem::SetFrame(core::Frame& frame) {
    }
    void ImGuiSystem::RequestSync(core::SyncObject& syncObject) {
    }
    void ImGuiSystem::Fork(core::Frame& frame, 
        core::SyncObject& syncObject,
        const core::Time& time) {
        // This will be called by the overlay after renderering is done
        // May also be called by other systems using ImGui to ensure that
        // the ImGui update happens last.
        mUpdateFinished.clear();
        marl::schedule([
            renderer = mRenderer,
            &update = mOnUpdate,
            &overlay = mOverlay,
            updateWait = mUpdateWaitGroup,
            updateFinished = mUpdateFinished]() {
            defer(updateFinished.signal());

            renderer->Wait();
            updateWait.wait();

            auto size = renderer->GetRenderArea();

            ImGui::GetIO().DisplaySize = ImVec2(size.x, size.y);
            ImGui_ImplGlfw_NewFrame();
            overlay.mImGuiImpl->NewFrame(size.x, size.y, overlay.mSurfaceTransform);
            update();
            overlay.mImGuiImpl->EndFrame();

            overlay.bDraw = true;
        });
    }

    void ImGuiSystem::Wait() {
        mUpdateFinished.wait();
    }

    void ImGuiSystem::Join(core::Frame& frame) {
        Wait();
    }
}

namespace okami::graphics {
    std::unique_ptr<core::ISystem> CreateImGui(
        IRenderer* renderer,
        core::ISystem* input) {
        return std::make_unique<diligent::ImGuiSystem>(renderer, input);
    }
}