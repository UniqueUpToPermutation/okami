#include <okami/diligent/ImGuiSystem.hpp>
#include <okami/diligent/Display.hpp>

#include <ImGuiImplDiligent.hpp>
#include <backends/imgui_impl_glfw.h>

namespace okami::graphics::diligent {

    void ImGuiRenderOverlay::Startup(
        core::ISystem* renderer,
        DG::IRenderDevice* device, 
        DG::ISwapChain* swapChain,
        const RenderModuleParams& params) {
        auto rtv = swapChain->GetCurrentBackBufferRTV();
        auto dsv = swapChain->GetDepthBufferDSV();

        mImGuiImpl = std::make_unique<DG::ImGuiImplDiligent>(device, 
            rtv->GetDesc().Format, dsv->GetDesc().Format);
        mSurfaceTransform = swapChain->GetDesc().PreTransform;
    }

    void ImGuiRenderOverlay::QueueCommands(
        DG::IDeviceContext* context, 
        RenderPass pass,
        const RenderModuleGlobals& globals) {
        assert(pass == RenderPass::OVERLAY);

        mRenderReady.wait();
        mImGuiImpl->Render(context);
        mRenderFinished.signal();
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

    void ImGuiSystem::Startup(marl::WaitGroup& waitGroup) {
        core::InterfaceCollection interfaces;
        mInputSystem->RegisterInterfaces(interfaces);

        auto glfwInterface = interfaces.Query<IGLFWWindowProvider>();

        if (glfwInterface) {
            // For high DPI stuff
            float xscale, yscale;
            glfwGetWindowContentScale(glfwInterface->GetWindowGLFW(), &xscale, &yscale);
            ImGui::GetIO().FontGlobalScale = glfwInterface->GetContentScale();

            // We have a glfw display!
            ImGui_ImplGlfw_InitForOther(glfwInterface->GetWindowGLFW(), false);

            mMouseButtonCallbackHandle = glfwInterface->AddMouseButtonCallback(
                [](GLFWwindow* window, int button, int action, int mods) {
                
                ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
                return ImGui::GetIO().WantCaptureMouse;
            }, CallbackPriority::GUI, this);

            mMouseButtonScrollHandle = glfwInterface->AddScrollCallback(
                [](GLFWwindow* window, double xoffset, double yoffset) {
                
                ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
                return ImGui::GetIO().WantCaptureMouse;
            }, CallbackPriority::GUI, this);

            mKeyHandle = glfwInterface->AddKeyCallback(
                [](GLFWwindow* window, int key, int scancode, int action, int mods) {

                ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
                return ImGui::GetIO().WantCaptureKeyboard;
            }, CallbackPriority::GUI, this);

            mCharHandle = glfwInterface->AddCharCallback(
                [](GLFWwindow* window, unsigned int codepoint) {

                ImGui_ImplGlfw_CharCallback(window, codepoint);
                return ImGui::GetIO().WantCaptureKeyboard;
            }, CallbackPriority::GUI, this);

            mWaitForInput = [glfwInterface]() {
                glfwInterface->WaitForInput();
            };

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

        auto glfwInterface = interfaces.Query<IGLFWWindowProvider>();

        if (glfwInterface) {
            glfwInterface->RemoveMouseButtonCallback(mMouseButtonCallbackHandle);
            glfwInterface->RemoveScrollCallback(mMouseButtonScrollHandle);
            glfwInterface->RemoveKeyCallback(mKeyHandle);
            glfwInterface->RemoveCharCallback(mCharHandle);

            // We have a glfw display!
            ImGui_ImplGlfw_Shutdown();

        } else {
            throw std::runtime_error("Input system does not implement any supported interfaces!");
        }

        mWaitForInput = nullptr;
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

        mOverlay.mRenderReady.clear();
        marl::schedule([
            renderer = mRenderer,
            &update = mOnUpdate,
            &overlay = mOverlay,
            inputWait = mWaitForInput]() {
            defer(overlay.mRenderReady.signal());

            if (inputWait)
                inputWait();

            auto size = renderer->GetRenderArea();

            ImGui::GetIO().DisplaySize = ImVec2(size.x, size.y);
            ImGui_ImplGlfw_NewFrame();
            overlay.mImGuiImpl->NewFrame(size.x, size.y, overlay.mSurfaceTransform);
            update();
            overlay.mImGuiImpl->EndFrame();
        });
    }

    void ImGuiSystem::Wait() {
        mOverlay.mRenderFinished.wait();
    }

    void ImGuiSystem::Join(core::Frame& frame) {
        Wait();
    }

    bool ImGuiSystem::ShouldCaptureMouse() const {
        return ImGui::GetIO().WantCaptureMouse;
    }  
    bool ImGuiSystem::ShouldCaptureKeyboard() const {
        return ImGui::GetIO().WantCaptureKeyboard;
    }
}

namespace okami::graphics {
    std::unique_ptr<core::ISystem> CreateImGui(
        core::ISystem* renderer,
        core::ISystem* input) {

        core::InterfaceCollection interfaces(renderer);
        auto rendererInterface = interfaces.Query<IRenderer>();

        if (!rendererInterface) {
            throw std::runtime_error("Renderer system does not expose "
                "IRenderer interface!");
        }

        return std::make_unique<diligent::ImGuiSystem>(rendererInterface, input);
    }
}
