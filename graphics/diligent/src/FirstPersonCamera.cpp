#include <okami/diligent/FirstPersonCamera.hpp>
#include <okami/Transform.hpp>
#include <okami/diligent/Glfw.hpp>

#include <iostream>

using namespace okami::core;

namespace okami::graphics::diligent {

    class FirstPersonCameraSystem : 
        public ISystem,
        public IInputCapture {
    private:
        delegate_handle_t mMouseButtonHandle;
        delegate_handle_t mMousePosHandle;
        delegate_handle_t mKeyboardHandle;

        IInputProvider* mInputProvider = nullptr;
        
        marl::Event mWaitEvent;

        UpdaterReads<Transform> mReads;
        UpdaterWrites<Transform> mWrites;

        FirstPersonController::Input mInput;

        struct {
            glm::dvec2 mLastMouse;
            glm::dvec2 mDMouse;
            bool bCaptureMouse = false;
            bool bMouseMove = false;
        } mMouseData;

    public:
        FirstPersonCameraSystem(IInputProvider* inputProvider);

        void Startup(marl::WaitGroup& waitGroup) override;
        void RegisterInterfaces(InterfaceCollection& interfaces) override;
        void Shutdown() override;
        void LoadResources(marl::WaitGroup& waitGroup) override;
        void SetFrame(Frame& frame) override;
        void RequestSync(SyncObject& syncObject) override;
        void Fork(Frame& frame,
            SyncObject& syncObject,
            const Time& time) override;
        void Join(Frame& frame) override;
        void Wait() override;

        bool ShouldCaptureMouse() const override;
        bool ShouldCaptureKeyboard() const override;
    };

    FirstPersonCameraSystem::FirstPersonCameraSystem(
        IInputProvider* inputProvider) :
        mWaitEvent(marl::Event::Mode::Manual),
        mInputProvider(inputProvider) {
    }

    void FirstPersonCameraSystem::Startup(marl::WaitGroup& waitGroup) {
        mMouseButtonHandle = mInputProvider->AddMouseButtonCallback(
            [&mouse = mMouseData](IInputProvider* input, MouseButton button, KeyAction action, KeyModifiers mods) {
                if (action == KeyAction::PRESS) {
                    if (button == MouseButton::RIGHT) {
                        input->SetCursorMode(CursorMode::DISABLED);
                        if (input->IsRawMouseMotionSupported())
                            input->SetRawMouseMotion(true);
                       
                        mouse.bCaptureMouse = true;

                        mouse.mLastMouse = input->GetCursorPos();
                        mouse.mDMouse.x = 0.0;
                        mouse.mDMouse.y = 0.0;
                    }
                } else if (action == KeyAction::RELEASE) {
                    if (button == MouseButton::RIGHT) {
                        if (input->IsRawMouseMotionSupported())
                            input->SetRawMouseMotion(false);
                        input->SetCursorMode(CursorMode::NORMAL);
                        
                        mouse.bCaptureMouse = false;

                        mouse.mDMouse.x = 0.0;
                        mouse.mDMouse.y = 0.0;
                    }
                }
                
                return mouse.bCaptureMouse;
            },
            CallbackPriority::MEDIUM,
            this
        );

        mMousePosHandle = mInputProvider->AddCursorPosCallback(
            [&mouse = mMouseData](IInputProvider* input, double xpos, double ypos) {
                if (mouse.bCaptureMouse) {
                    glm::dvec2 pos(xpos, ypos);
                    mouse.mDMouse = pos - mouse.mLastMouse;
                    mouse.mLastMouse = pos;
                    mouse.bMouseMove = true;
                }
                return false;
            },
            CallbackPriority::MEDIUM,
            this
        );

        mKeyboardHandle = mInputProvider->AddKeyCallback(
            [&input = mInput](IInputProvider* inputProvider, 
                Key key, 
                int scancode, 
                KeyAction action, 
                KeyModifiers mods) {

                if (action == KeyAction::PRESS) {
                    switch (key) {
                    case Key::W:
                        input.bForward = true;
                        break;
                    case Key::S:
                        input.bBackward = true;
                        break;
                    case Key::A:
                        input.bLeft = true;
                        break;
                    case Key::D:
                        input.bRight = true;
                        break;
                    default:
                        break;
                    } 
                } else if (action == KeyAction::RELEASE) {
                    switch (key) {
                    case Key::W:
                        input.bForward = false;
                        break;
                    case Key::S:
                        input.bBackward = false;
                        break;
                    case Key::A:
                        input.bLeft = false;
                        break;
                    case Key::D:
                        input.bRight = false;
                        break;
                    default:
                        break;
                    } 
                }

                return false;
            },
            CallbackPriority::MEDIUM,
            this
        );
    }

    void FirstPersonCameraSystem::RegisterInterfaces(InterfaceCollection& interfaces) {
    }

    void FirstPersonCameraSystem::Shutdown() {
        mInputProvider->RemoveMouseButtonCallback(mMouseButtonHandle);
        mInputProvider->RemoveCursorPosCallback(mMousePosHandle);
        mInputProvider->RemoveKeyCallback(mKeyboardHandle);
    }

    void FirstPersonCameraSystem::LoadResources(marl::WaitGroup& waitGroup) {
    }
    void FirstPersonCameraSystem::SetFrame(Frame& frame) {
    }
    void FirstPersonCameraSystem::RequestSync(SyncObject& syncObject) {
        mWaitEvent.clear();

        mReads.RequestSync(syncObject);
        mWrites.RequestSync(syncObject);
    }
    void FirstPersonCameraSystem::Fork(Frame& frame,
        SyncObject& syncObject,
        const Time& time) {

        marl::schedule([inputProvider = mInputProvider,
            &reads = mReads,
            &writes = mWrites,
            &event = mWaitEvent,
            &input = mInput,
            time,
            &mouse = mMouseData,
            &frame]() {
            defer(event.signal());
            defer(reads.ReleaseHandles());
            defer(writes.ReleaseHandles());

            inputProvider->WaitForInput();

            if (mouse.bCaptureMouse && mouse.bMouseMove) {
                input.mRotateX = mouse.mDMouse.x;
                input.mRotateY = mouse.mDMouse.y;
                mouse.bMouseMove = false;
            } else {
                input.mRotateX = 0.0;
                input.mRotateY = 0.0;
            }

            reads.Read<Transform>([&]() {
                auto view = frame.Registry().view<
                    Transform, 
                    FirstPersonController>();

                for (auto e : view) {
                    auto& transform = view.get<Transform>(e);
                    auto& controller = view.get<FirstPersonController>(e);
                    controller.PrepareUpdate(input, transform, time);
                }
            });

            writes.Write<Transform>([&]() {
                auto view = frame.Registry().view<
                    Transform, 
                    FirstPersonController>();

                for (auto e : view) {
                    auto& transform = view.get<Transform>(e);
                    auto& controller = view.get<FirstPersonController>(e);
                    controller.FlushUpdate(transform);
                }
            });
        });
    }
    void FirstPersonCameraSystem::Join(Frame& frame) {
        Wait();
    }
    void FirstPersonCameraSystem::Wait() {
        mWaitEvent.wait();
    }
    bool FirstPersonCameraSystem::ShouldCaptureMouse() const {
        return mMouseData.bCaptureMouse;
    }
    bool FirstPersonCameraSystem::ShouldCaptureKeyboard() const {
        return false;
    }
}

namespace okami::graphics {
    void FirstPersonController::PrepareUpdate(
        const FirstPersonController::Input& input, 
        const Transform& transform,
        const Time& time) {
        if (!bInitialized) {
            glm::vec3 forward(0.0f, 0.0f, 1.0f);
            forward = transform.mRotation * forward;
            auto phi = -std::acos(forward.y) + glm::pi<float>() / 2.0f;
            auto theta = std::atan2(forward.x, forward.z);

            mQX = glm::angleAxis(theta, glm::vec3(0.0f, 1.0f, 0.0f));
            mQY = glm::angleAxis(-phi, glm::vec3(1.0f, 0.0f, 0.0f));

            bInitialized = true;
        }
        
        if (bEnabled) {
            auto mRight = 
                transform.ApplyToTangent(
                    glm::vec3(1.0f, 0.0f, 0.0f));
            auto mForward =
                transform.ApplyToTangent(
                    glm::vec3(0.0f, 0.0f, 1.0f));               
        
            glm::vec3 dpos(0.0f, 0.0f, 0.0f);

            if (input.bForward)
                dpos += mForward;
            if (input.bBackward)
                dpos -= mForward;
            if (input.bRight)
                dpos += mRight;
            if (input.bLeft)
                dpos -= mRight;

            if (glm::length(dpos) > 0.5f) {
                mDPos = mMoveSpeed * glm::normalize(dpos) * (float)time.mTimeElapsed;
            } else {
                mDPos = glm::vec3(0.0f, 0.0f, 0.0f);
            }

            float dRotX = input.mRotateX * (float)time.mTimeElapsed;
            float dRotY = input.mRotateY * (float)time.mTimeElapsed;

            auto dQX = glm::angleAxis(dRotX, glm::vec3(0.0f, 1.0f, 0.0f));
            auto dQY = glm::angleAxis(dRotY, glm::vec3(1.0f, 0.0f, 0.0f));
            
            mQX = mQX * dQX;
            mQY = mQY * dQY;
        } else {
            mDPos = glm::vec3(0.0f, 0.0f, 0.0f);
        }
    }

    void FirstPersonController::FlushUpdate(
        Transform& output) {
        output.mTranslation += mDPos;
        output.mRotation = mQX * mQY;
    }

    std::unique_ptr<ISystem> CreateFPSCameraSystem(IInputProvider* input) {
        return std::make_unique<diligent::FirstPersonCameraSystem>(input);
    }
}