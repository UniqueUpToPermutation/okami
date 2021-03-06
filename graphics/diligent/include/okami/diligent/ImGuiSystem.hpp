#pragma once

#include <okami/System.hpp>
#include <okami/Graphics.hpp>
#include <okami/Input.hpp>
#include <okami/diligent/RenderModule.hpp>
#include <okami/Event.hpp>

#include <RefCntAutoPtr.hpp>
#include <ImGuiImplDiligent.hpp>
#include <ImGuiDiligentRenderer.hpp>

#include <imgui.h>

namespace okami::graphics::diligent {

    class ImGuiRenderOverlay final :
        public IOverlayModule {
    public:
        DG::SURFACE_TRANSFORM mSurfaceTransform;

        static constexpr DG::Uint32 DefaultInitialVBSize = 1024;
        static constexpr DG::Uint32 DefaultInitialIBSize = 2048;

        std::array<ICursor*, ImGuiMouseCursor_COUNT>& mMouseCursors;

        ImFontAtlas mSharedAtlas;
        ImGuiContext* mFirstContext = nullptr;
        ImGuiIO mDefaultIO;

        struct ImGuiImpl final : public core::IInputCapture {
            IWindow* mWindow = nullptr;

            core::delegate_handle_t mMouseButtonCallbackHandle = 0;
            core::delegate_handle_t mMouseButtonScrollHandle = 0;
            core::delegate_handle_t mKeyHandle = 0;
            core::delegate_handle_t mCharHandle = 0;

            std::array<ICursor*, ImGuiMouseCursor_COUNT>& mMouseCursors;
            std::array<bool, ImGuiMouseButton_COUNT> mMouseJustPressed;
            
            ImGuiContext* mContext;
            ImGuiIO mIO;

            double mTime = 0.0;

            RenderModuleGlobals mLastFrameGlobals;

            bool ShouldCaptureMouse() const override;
            bool ShouldCaptureKeyboard() const override;

            void NewFrame(const core::Time& time, ImGuiIO& io);
            void EndFrame(ImGuiIO& io);
            void UpdateMouseCursor();
            void UpdateGamepads();
            void UpdateMousePosAndButtons();

            void KeyCallback(
                core::Key key, 
                int scancode, 
                core::KeyAction action, 
                core::KeyModifiers mods);
            void ScrollCallback(
                double xoffset, 
                double yoffset);
            void CharCallback(
                unsigned int c);
            void MouseButtonCallback(
                core::MouseButton button,
                core::KeyAction action,
                core::KeyModifiers mods);
            
            ImGuiImpl(
                IWindow* window,
                std::array<ICursor*, ImGuiMouseCursor_COUNT>& cursors,
                ImGuiContext* context,
                const ImGuiIO& defaultIO);
            ~ImGuiImpl();
        };

        core::UserDataEvent<IWindow*> mOnGenerateRenderLists;
        std::unordered_map<IWindow*,
            std::unique_ptr<ImGuiImpl>> mImGuiImpls;

        marl::Event mRenderReady;

        std::unique_ptr<DG::ImGuiDiligentRenderer> mRenderer;

        void Startup(
            core::ISystem* renderer,
            DG::IRenderDevice* device,
            const RenderModuleParams& params) override;
        void QueueCommands(
            DG::IDeviceContext* context,
            const core::Frame& frame,
            const RenderView& view,
            const RenderCanvas& target,
            const RenderPass& pass,
            const RenderModuleGlobals& globals) override;
        void Shutdown() override;

        void AddWindow(IWindow* window);
        void RemoveWindow(IWindow* window);

        core::delegate_handle_t AddCallback(
            IWindow* window, 
            immedate_callback_t callback);
        void RemoveCallback(core::delegate_handle_t handle);

        void OnAttach(RenderCanvas* canvas) override;
        void OnDettach(RenderCanvas* canvas) override;

        void DettachAll();

        void UpdateCanvas(ImGuiImpl* impl, const core::Time& time);
        void UpdateAllCanvases(const core::Time& time);

        void Update(core::ResourceManager*) override;
        void WaitUntilReady(core::SyncObject& obj) override;
        
        bool IsIdle() override;
        void WaitOnPendingTasks() override;

        inline ImGuiRenderOverlay(
            std::array<ICursor*, ImGuiMouseCursor_COUNT>& cursors) :
            mRenderReady(marl::Event::Mode::Manual),
            mMouseCursors(cursors) {    
        }
    };

    class ImGuiSystem final : 
        public core::ISystem,
        public IImGuiSystem {
    private:
        ImGuiRenderOverlay mOverlay;
        IRenderer* mRenderer;
        IDisplay* mDisplay;

        std::array<ICursor*, ImGuiMouseCursor_COUNT> mMouseCursors;

    public:
        ImGuiSystem(
            IDisplay* display,
            IRenderer* renderer);
        ImGuiSystem(
            IDisplay* display,
            IRenderer* renderer, 
            IWindow* window);
        ~ImGuiSystem();

        IRenderCanvasAttachment* AddOverlayTo(IWindow* window) override;
        core::delegate_handle_t Add(
            IWindow* window,
            immedate_callback_t callback) override;
        void Remove(core::delegate_handle_t handle) override;

        void Startup(marl::WaitGroup& waitGroup) override;
        void RegisterInterfaces(core::InterfaceCollection& interfaces) override;
        void Shutdown() override;
        void LoadResources(marl::WaitGroup& waitGroup) override;
        void SetFrame(core::Frame& frame) override;
        void RequestSync(core::SyncObject& syncObject) override;
        void Fork(core::Frame& frame, 
            core::SyncObject& syncObject,
            const core::Time& time) override;
        void Join(core::Frame& frame) override;
        void Wait() override;
    };
}