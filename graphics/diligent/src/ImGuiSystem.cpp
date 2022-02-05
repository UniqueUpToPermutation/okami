#include <okami/diligent/ImGuiSystem.hpp>
#include <okami/diligent/GraphicsUtils.hpp>

#include <ImGuiImplDiligent.hpp>
#include <ImGuiDiligentRenderer.hpp>
#include <backends/imgui_impl_glfw.h>

using namespace okami::core;

namespace okami::graphics::diligent {

    static const char* ImGui_ImplOkami_GetClipboardText(void* user_data) {
        return reinterpret_cast<ImGuiRenderOverlay::ImGuiImpl*>
            (user_data)->mWindow->GetClipboardText();
    }

    static void ImGui_ImplOkami_SetClipboardText(void* user_data, const char* text) {
        reinterpret_cast<ImGuiRenderOverlay::ImGuiImpl*>
            (user_data)->mWindow->SetClipboardText(text);
    }

    void ImGuiRenderOverlay::ImGuiImpl::MouseButtonCallback(
        core::MouseButton button, 
        core::KeyAction action,
        core::KeyModifiers mods) {

        if (action == core::KeyAction::PRESS && 
                (uint)button >= 0 && 
                (uint)button < mMouseJustPressed.size()) {
            mMouseJustPressed[(int)button] = true;
        }
    }

    void ImGuiRenderOverlay::ImGuiImpl::ScrollCallback(
        double xoffset, 
        double yoffset) {
        mIO.MouseWheelH += (float)xoffset;
        mIO.MouseWheel += (float)yoffset;
    }

    void ImGuiRenderOverlay::ImGuiImpl::KeyCallback(
        core::Key key,
        int scancode,
        core::KeyAction action,
        core::KeyModifiers mods) {

        ImGuiIO& io = mIO;
        if (action == core::KeyAction::PRESS)
            io.KeysDown[(int)key] = true;
        if (action == core::KeyAction::RELEASE)
            io.KeysDown[(int)key] = false;

        // Modifiers are not reliable across systems
        io.KeyCtrl = io.KeysDown[(int)core::Key::LEFT_CONTROL] 
            || io.KeysDown[(int)core::Key::RIGHT_CONTROL];
        io.KeyShift = io.KeysDown[(int)core::Key::LEFT_SHIFT] 
            || io.KeysDown[(int)core::Key::RIGHT_SHIFT];
        io.KeyAlt = io.KeysDown[(int)core::Key::LEFT_ALT] 
            || io.KeysDown[(int)core::Key::RIGHT_ALT];
#ifdef _WIN32
        io.KeySuper = false;
#else
        io.KeySuper = io.KeysDown[(int)core::Key::LEFT_SUPER] 
            || io.KeysDown[(int)core::Key::RIGHT_SUPER];
#endif
    }

    void ImGuiRenderOverlay::ImGuiImpl::CharCallback(unsigned int c) {
        ImGuiIO& io = mIO;
        io.AddInputCharacter(c);
    }

    bool ImGuiRenderOverlay::ImGuiImpl::ShouldCaptureMouse() const {
        return mIO.WantCaptureMouse;
    }

    bool ImGuiRenderOverlay::ImGuiImpl::ShouldCaptureKeyboard() const {
        return mIO.WantCaptureKeyboard;
    }

    void ImGuiRenderOverlay::ImGuiImpl::NewFrame(const core::Time& time, ImGuiIO& globalIO) {
        ImGuiIO& io = mIO;
        IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

        // Setup display size (every frame to accommodate for window resizing)
        auto win_size = mWindow->GetWindowSize();
        auto frame_size = mWindow->GetFramebufferSize();

        io.DisplaySize = ImVec2((float)win_size.x, (float)win_size.y);
        if (win_size.x > 0 && win_size.y > 0)
            io.DisplayFramebufferScale = ImVec2(
                (float)frame_size.x / win_size.x, 
                (float)frame_size.y / win_size.y);

        // Setup time step
        io.DeltaTime = time.mTimeElapsed;
        mTime = time.mTotalTime;

        UpdateMousePosAndButtons();
        UpdateMouseCursor();

        // Update game controllers (if enabled and available)
        UpdateGamepads();

        // Copy over the internal IO state
        globalIO = io;
    }

    void ImGuiRenderOverlay::ImGuiImpl::EndFrame(ImGuiIO& globalIO) {
        // Copy back the internal IO state
        mIO = globalIO;
    }

    ImGuiRenderOverlay::ImGuiImpl::ImGuiImpl(
        IWindow* window,
        std::array<ICursor*, ImGuiMouseCursor_COUNT>& cursors,
        ImGuiContext* context,
        const ImGuiIO& defaultIO) : 
        mWindow(window),
        mMouseCursors(cursors),
        mContext(context),
        mIO(defaultIO) {

        ImGuiIO& io = mIO;

        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
        io.BackendPlatformName = "imgui_impl_okami";

        // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
        io.KeyMap[ImGuiKey_Tab] = (int)core::Key::TAB;
        io.KeyMap[ImGuiKey_LeftArrow] = (int)core::Key::LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = (int)core::Key::RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = (int)core::Key::UP;
        io.KeyMap[ImGuiKey_DownArrow] = (int)core::Key::DOWN;
        io.KeyMap[ImGuiKey_PageUp] = (int)core::Key::PAGE_UP;
        io.KeyMap[ImGuiKey_PageDown] = (int)core::Key::PAGE_DOWN;
        io.KeyMap[ImGuiKey_Home] = (int)core::Key::HOME;
        io.KeyMap[ImGuiKey_End] = (int)core::Key::END;
        io.KeyMap[ImGuiKey_Insert] = (int)core::Key::INSERT;
        io.KeyMap[ImGuiKey_Delete] = (int)core::Key::DELETE;
        io.KeyMap[ImGuiKey_Backspace] = (int)core::Key::BACKSPACE;
        io.KeyMap[ImGuiKey_Space] = (int)core::Key::SPACE;
        io.KeyMap[ImGuiKey_Enter] = (int)core::Key::ENTER;
        io.KeyMap[ImGuiKey_Escape] = (int)core::Key::ESCAPE;
        io.KeyMap[ImGuiKey_KeyPadEnter] = (int)core::Key::NUMPAD_ENTER;
        io.KeyMap[ImGuiKey_A] = (int)core::Key::A;
        io.KeyMap[ImGuiKey_C] = (int)core::Key::C;
        io.KeyMap[ImGuiKey_V] = (int)core::Key::V;
        io.KeyMap[ImGuiKey_X] = (int)core::Key::X;
        io.KeyMap[ImGuiKey_Y] = (int)core::Key::Y;
        io.KeyMap[ImGuiKey_Z] = (int)core::Key::Z;

        io.SetClipboardTextFn = ImGui_ImplOkami_SetClipboardText;
        io.GetClipboardTextFn = ImGui_ImplOkami_GetClipboardText;
        io.ClipboardUserData = this;
        io.ImeWindowHandle = mWindow->GetWin32Window();
        io.FontGlobalScale = mWindow->GetContentScale();

        mMouseButtonCallbackHandle = mWindow->AddMouseButtonCallback(
            [this](IInputProvider* input, 
                core::MouseButton button, 
                core::KeyAction action,
                core::KeyModifiers mods) {
            
            MouseButtonCallback(button, action, mods);
            return mIO.WantCaptureMouse;
        }, CallbackPriority::GUI, this);

        mMouseButtonScrollHandle = mWindow->AddScrollCallback(
            [this](IInputProvider* input,
                double xoffset, 
                double yoffset) {
            
            ScrollCallback(xoffset, yoffset);
            return mIO.WantCaptureMouse;
        }, CallbackPriority::GUI, this);

        mKeyHandle = mWindow->AddKeyCallback(
            [this](IInputProvider* input,
                core::Key key,
                int scancode,
                core::KeyAction action,
                core::KeyModifiers mods) {

            KeyCallback(key, scancode, action, mods);
            return mIO.WantCaptureKeyboard;
        }, CallbackPriority::GUI, this);

        mCharHandle = mWindow->AddCharCallback(
            [this](IInputProvider* input,
                unsigned int codepoint) {

            CharCallback(codepoint);
            return mIO.WantCaptureKeyboard;
        }, CallbackPriority::GUI, this);
    }

    void ImGuiRenderOverlay::ImGuiImpl::UpdateMouseCursor() {
        ImGuiIO& io = mIO;
        if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || 
            mWindow->GetCursorMode() == CursorMode::DISABLED)
            return;

        ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
        if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
        {
            // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
            mWindow->SetCursorMode(CursorMode::HIDDEN);
        }
        else
        {
            // Show OS mouse cursor
            // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
            mWindow->SetCursor(mMouseCursors[imgui_cursor] ? 
                mMouseCursors[imgui_cursor] : mMouseCursors[ImGuiMouseCursor_Arrow]);
            mWindow->SetCursorMode(CursorMode::NORMAL);
        }
    }

    void ImGuiRenderOverlay::ImGuiImpl::UpdateGamepads() {
        ImGuiIO& io = mIO;
        memset(io.NavInputs, 0, sizeof(io.NavInputs));
        if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
            return;

        // Update gamepad inputs
        #define MAP_BUTTON(NAV_NO, BUTTON_NO)       { if (buttons_count > BUTTON_NO && buttons[BUTTON_NO] == GLFW_PRESS) io.NavInputs[NAV_NO] = 1.0f; }
        #define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { float v = (axes_count > AXIS_NO) ? axes[AXIS_NO] : V0; v = (v - V0) / (V1 - V0); if (v > 1.0f) v = 1.0f; if (io.NavInputs[NAV_NO] < v) io.NavInputs[NAV_NO] = v; }
        int axes_count = 0, buttons_count = 0;
        const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
        const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttons_count);
        MAP_BUTTON(ImGuiNavInput_Activate,   0);     // Cross / A
        MAP_BUTTON(ImGuiNavInput_Cancel,     1);     // Circle / B
        MAP_BUTTON(ImGuiNavInput_Menu,       2);     // Square / X
        MAP_BUTTON(ImGuiNavInput_Input,      3);     // Triangle / Y
        MAP_BUTTON(ImGuiNavInput_DpadLeft,   13);    // D-Pad Left
        MAP_BUTTON(ImGuiNavInput_DpadRight,  11);    // D-Pad Right
        MAP_BUTTON(ImGuiNavInput_DpadUp,     10);    // D-Pad Up
        MAP_BUTTON(ImGuiNavInput_DpadDown,   12);    // D-Pad Down
        MAP_BUTTON(ImGuiNavInput_FocusPrev,  4);     // L1 / LB
        MAP_BUTTON(ImGuiNavInput_FocusNext,  5);     // R1 / RB
        MAP_BUTTON(ImGuiNavInput_TweakSlow,  4);     // L1 / LB
        MAP_BUTTON(ImGuiNavInput_TweakFast,  5);     // R1 / RB
        MAP_ANALOG(ImGuiNavInput_LStickLeft, 0,  -0.3f,  -0.9f);
        MAP_ANALOG(ImGuiNavInput_LStickRight,0,  +0.3f,  +0.9f);
        MAP_ANALOG(ImGuiNavInput_LStickUp,   1,  +0.3f,  +0.9f);
        MAP_ANALOG(ImGuiNavInput_LStickDown, 1,  -0.3f,  -0.9f);
        #undef MAP_BUTTON
        #undef MAP_ANALOG
        if (axes_count > 0 && buttons_count > 0)
            io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
        else
            io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    }

    void ImGuiRenderOverlay::ImGuiImpl::UpdateMousePosAndButtons() {
        // Update buttons
        ImGuiIO& io = mIO;
        for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)

        {
            // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
            io.MouseDown[i] = mMouseJustPressed[i] || 
                mWindow->GetState((MouseButton)i) != KeyState::RELEASE;
            mMouseJustPressed[i] = false;
        }

        // Update mouse position
        const ImVec2 mouse_pos_backup = io.MousePos;
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

        if (mWindow->IsFocused())
        {
            if (io.WantSetMousePos)
            {
                mWindow->SetCursorPos(
                    (double)mouse_pos_backup.x, 
                    (double)mouse_pos_backup.y);
            }
            else
            {
                double mouse_x, mouse_y;
                auto mouse = mWindow->GetCursorPos();
                io.MousePos = ImVec2((float)mouse.x, (float)mouse.y);
            }
        }
    }

    ImGuiRenderOverlay::ImGuiImpl::~ImGuiImpl() {
        mWindow->RemoveMouseButtonCallback(mMouseButtonCallbackHandle);
        mWindow->RemoveScrollCallback(mMouseButtonScrollHandle);
        mWindow->RemoveKeyCallback(mKeyHandle);
        mWindow->RemoveCharCallback(mCharHandle);

        ImGui::DestroyContext(mContext);
    }

    void ImGuiRenderOverlay::Startup(
        core::ISystem* renderer,
        DG::IRenderDevice* device,
        const RenderModuleParams& params) {

        mFirstContext = ImGui::CreateContext(&mSharedAtlas);
        ImGui::SetCurrentContext(mFirstContext);

        auto renderPassFormatProvider = renderer->QueryInterface<IRenderPassFormatProvider>();

        if (renderPassFormatProvider == nullptr) {
            throw std::runtime_error("Renderer does not implement IRenderPassFormatProvider!");
        }        

        auto rtFormat = 
            renderPassFormatProvider->GetFormat(RenderAttribute::COLOR);
        auto dsvFormat = 
            renderPassFormatProvider->GetDepthFormat(RenderPass::Final());

        mRenderer = std::make_unique<DG::ImGuiDiligentRenderer>(
            device, 
            rtFormat, 
            dsvFormat,
            DefaultInitialVBSize,
            DefaultInitialIBSize);

        mRenderer->CreateDeviceObjects();
        mRenderer->CreateFontsTexture();

        mDefaultIO = ImGui::GetIO();
    }

    void ImGuiRenderOverlay::QueueCommands(
        DG::IDeviceContext* context, 
        const core::Frame& frame,
        const RenderView& view,
        const RenderPass& pass,
        const RenderModuleGlobals& globals) {
        assert(pass.IsFinal());

        mRenderReady.wait();

        for (auto& [key, impl] : mImGuiImpls) {
            auto canvas = impl->mWindow->GetCanvas();
            if (canvas == view.mTarget) {
                ImGui::SetCurrentContext(impl->mContext);
                ImGui::Render();

                auto props = canvas->GetProperties();
                mRenderer->NewFrame(
                    props.mWidth, 
                    props.mHeight, 
                    ToDiligent(props.mTransform));
                mRenderer->RenderDrawData(context, ImGui::GetDrawData());
                mRenderer->EndFrame();
            }
        }
    }
    
    void ImGuiRenderOverlay::Shutdown() {
        mRenderer.reset();

        if (mFirstContext) {
            ImGui::DestroyContext(mFirstContext);
        }

        std::vector<IWindow*> windows;
        for (auto& [window, impl] : mImGuiImpls) {
            windows.emplace_back(window);
        }

        for (auto window : windows) {
            window->GetCanvas()->RemoveOverlay(this);
        }
    }

    void ImGuiRenderOverlay::UpdateCanvas(ImGuiImpl* impl, const Time& time) {
        impl->mWindow->WaitForInput();

        ImGui::SetCurrentContext(impl->mContext);
        
        impl->NewFrame(time, ImGui::GetIO());
        ImGui::NewFrame();
        mOnGenerateRenderLists.InvokeOnly(impl->mWindow);
        ImGui::EndFrame();
        impl->EndFrame(ImGui::GetIO());
    }

    void ImGuiRenderOverlay::UpdateAllCanvases(const Time& time) {
        for (auto& [key, impl] : mImGuiImpls) {
            UpdateCanvas(impl.get(), time);
        }
    }

    void ImGuiRenderOverlay::AddWindow(
        IWindow* window) {

        if (mImGuiImpls.find(window) != mImGuiImpls.end()) {
            throw std::runtime_error("Overlay has already been attached to canvas!");
        }

        ImGuiContext* context = mFirstContext;

        if (!context) {
            context = ImGui::CreateContext(&mSharedAtlas);
        } else {
            mFirstContext = nullptr;
        }
        
        auto impl = std::make_unique<ImGuiImpl>(
            window, mMouseCursors, context, mDefaultIO);
        mImGuiImpls.emplace(window, std::move(impl));
    }

    void ImGuiRenderOverlay::RemoveWindow(
        IWindow* window) {

        mImGuiImpls.erase(window);
        mOnGenerateRenderLists.RemoveAll(window);
    }

    core::delegate_handle_t ImGuiRenderOverlay::AddCallback(
        IWindow* window, 
        immedate_callback_t callback) {
        
#ifndef NDEBUG
        auto it = mImGuiImpls.find(window);

        if (it == mImGuiImpls.end()) {
            throw std::runtime_error("The ImGuiSystem must first " 
                "be attached to the designated render canvas!");
        }
#endif

        return mOnGenerateRenderLists.Add(window, std::move(callback));
    }

    void ImGuiRenderOverlay::RemoveCallback(
        core::delegate_handle_t handle) {
        mOnGenerateRenderLists.Remove(handle);
    }

    ImGuiSystem::ImGuiSystem(
        IDisplay* display,
        IRenderer* renderer) :
        mRenderer(renderer),
        mDisplay(display),
        mOverlay(mMouseCursors) {
        mRenderer->AddOverlay(&mOverlay);
    }

    ImGuiSystem::ImGuiSystem(
        IDisplay* display,
        IRenderer* renderer, 
        IWindow* window) : 
        ImGuiSystem(display, renderer) {
        mRenderer->AddOverlay(&mOverlay);
        mOverlay.AddWindow(window);
    }

    ImGuiSystem::~ImGuiSystem() {
        mRenderer->RemoveOverlay(&mOverlay);
    }

    core::delegate_handle_t ImGuiSystem::Add(
        IWindow* window, 
        immedate_callback_t callback) {
        return mOverlay.AddCallback(window, std::move(callback));
    }

    IRenderCanvasAttachment* ImGuiSystem::AddOverlayTo(IWindow* window) {
        mOverlay.AddWindow(window);
        IRenderCanvasAttachment* ptr = &mOverlay;
        window->GetCanvas()->AddOverlay(ptr);
        return ptr;
    }

    void ImGuiSystem::Remove(
        core::delegate_handle_t handle) {
        mOverlay.RemoveCallback(handle);
    }

    void ImGuiSystem::Startup(marl::WaitGroup& waitGroup) {
        // Create mouse cursors
        // (By design, on X11 cursors are user configurable and some cursors may be missing. When a cursor doesn't exist,
        // GLFW will emit an error which will often be printed by the app, so we temporarily disable error reporting.
        // Missing cursors will return NULL and our _UpdateMouseCursor() function will use the Arrow cursor instead.)
        mMouseCursors[ImGuiMouseCursor_Arrow] = 
            mDisplay->CreateStandardCursor(StandardCursor::ARROW);
        mMouseCursors[ImGuiMouseCursor_TextInput] = 
            mDisplay->CreateStandardCursor(StandardCursor::IBEAM);
        mMouseCursors[ImGuiMouseCursor_ResizeNS] = 
            mDisplay->CreateStandardCursor(StandardCursor::VRESIZE);
        mMouseCursors[ImGuiMouseCursor_ResizeEW] = 
            mDisplay->CreateStandardCursor(StandardCursor::HRESIZE);
        mMouseCursors[ImGuiMouseCursor_Hand] =
            mDisplay->CreateStandardCursor(StandardCursor::HAND);
        mMouseCursors[ImGuiMouseCursor_ResizeAll] =
            mDisplay->CreateStandardCursor(StandardCursor::RESIZE_ALL);
        mMouseCursors[ImGuiMouseCursor_ResizeNESW] =
            mDisplay->CreateStandardCursor(StandardCursor::RESIZE_NESW);
        mMouseCursors[ImGuiMouseCursor_ResizeNWSE] = 
            mDisplay->CreateStandardCursor(StandardCursor::RESIZE_NWSE);
        mMouseCursors[ImGuiMouseCursor_NotAllowed] = 
            mDisplay->CreateStandardCursor(StandardCursor::NOT_ALLOWED);
    }

    void ImGuiSystem::RegisterInterfaces(core::InterfaceCollection& interfaces) {
        interfaces.Add<IImGuiSystem>(this);
    }

    void ImGuiSystem::Shutdown() {
        for (auto& cursor : mMouseCursors) {
            mDisplay->DestroyCursor(cursor);
            cursor = nullptr;
        }
    }

    void ImGuiSystem::LoadResources(marl::WaitGroup& waitGroup) {
    }

    void ImGuiSystem::SetFrame(core::Frame& frame) {
    }

    void ImGuiSystem::RequestSync(core::SyncObject& syncObject) {
        mOverlay.mRenderReady.clear();
    }

    void ImGuiSystem::Fork(core::Frame& frame, 
        core::SyncObject& syncObject,
        const core::Time& time) {

        marl::schedule([
            time,
            &overlay = mOverlay]() {
            defer(overlay.mRenderReady.signal());

            overlay.UpdateAllCanvases(time);
        });
    }

    void ImGuiSystem::Wait() {
        mOverlay.mRenderReady.wait();
    }

    void ImGuiSystem::Join(core::Frame& frame) {
        Wait();
    }

    void ImGuiRenderOverlay::OnAttach(RenderCanvas* canvas) {
    }

    void ImGuiRenderOverlay::OnDettach(RenderCanvas* canvas) {
        RemoveWindow(canvas->GetWindow());
    }

    void ImGuiRenderOverlay::Update(bool bAllowBlock) {
    }

    void ImGuiRenderOverlay::WaitUntilReady(core::SyncObject& obj) {
        mRenderReady.wait();
    }
}

namespace okami::graphics {
    std::unique_ptr<core::ISystem> CreateImGui(
        IDisplay* display,
        IRenderer* renderer, 
        IWindow* window) {

        return std::make_unique<diligent::ImGuiSystem>(
            display, renderer, window);
    }

    std::unique_ptr<core::ISystem> CreateImGui(
        IDisplay* display,
        IRenderer* renderer) {

        return std::make_unique<diligent::ImGuiSystem>(
            display, renderer);
    }
}
