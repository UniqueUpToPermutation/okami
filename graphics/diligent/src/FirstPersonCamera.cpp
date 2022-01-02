#include <okami/diligent/FirstPersonCamera.hpp>
#include <okami/Transform.hpp>
#include <okami/diligent/Display.hpp>

#include <iostream>

namespace okami::graphics::diligent {

    class FirstPersonCameraSystem : 
        public core::ISystem,
        public IInputCapture {
    private:
        core::delegate_handle_t mMouseButtonHandle;
        core::delegate_handle_t mMousePosHandle;
        core::delegate_handle_t mKeyboardHandle;

        IGLFWWindowProvider* mWindowProvider;
        
        marl::Event mWaitEvent;

        core::UpdaterReads<core::Transform> mReads;
        core::UpdaterWrites<core::Transform> mWrites;

        FirstPersonController::Input mInput;

        struct {
            double mLastMouseX;
            double mLastMouseY;
            double mDMouseX;
            double mDMouseY;
            bool bCaptureMouse = false;
            bool bMouseMove = false;
        } mMouseData;

    public:
        FirstPersonCameraSystem(core::ISystem* inputProvider);

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
        void Request(const entt::meta_type& interfaceType) override;

        bool ShouldCaptureMouse() const override;
        bool ShouldCaptureKeyboard() const override;
    };

    FirstPersonCameraSystem::FirstPersonCameraSystem(
        core::ISystem* inputProvider) :
        mWaitEvent(marl::Event::Mode::Manual) {

        core::InterfaceCollection collection;
        inputProvider->RegisterInterfaces(collection);

        mWindowProvider = collection.Query<IGLFWWindowProvider>();
    
        if (!mWindowProvider) {
            throw std::runtime_error("Input provider does not implement"
                "IGLFWWindowProvider");
        }
    }
    void FirstPersonCameraSystem::Startup(marl::WaitGroup& waitGroup) {
        mMouseButtonHandle = mWindowProvider->AddMouseButtonCallback(
            [&mouse = mMouseData](GLFWwindow* window, int button, int action, int mods) {
                if (action == GLFW_PRESS) {
                    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                        if (glfwRawMouseMotionSupported())
                            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                        mouse.bCaptureMouse = true;

                        glfwGetCursorPos(window,
                            &mouse.mLastMouseX,
                            &mouse.mLastMouseY);

                        mouse.mDMouseX = 0.0;
                        mouse.mDMouseY = 0.0;
                    }
                } else if (action == GLFW_RELEASE) {
                    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                        if (glfwRawMouseMotionSupported())
                            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
                        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                        
                        mouse.bCaptureMouse = false;

                        mouse.mDMouseX = 0.0;
                        mouse.mDMouseY = 0.0;
                    }
                }
                
                return mouse.bCaptureMouse;
            },
            CallbackPriority::MEDIUM,
            this
        );

        mMousePosHandle = mWindowProvider->AddCursorPosCallback(
            [&mouse = mMouseData](GLFWwindow* window, double xpos, double ypos) {
                if (mouse.bCaptureMouse) {
                    mouse.mDMouseX = xpos - mouse.mLastMouseX;
                    mouse.mDMouseY = ypos - mouse.mLastMouseY;

                    mouse.mLastMouseX = xpos;
                    mouse.mLastMouseY = ypos;

                    mouse.bMouseMove = true;
                }
                return false;
            },
            CallbackPriority::MEDIUM,
            this
        );

        mKeyboardHandle = mWindowProvider->AddKeyCallback(
            [&input = mInput](GLFWwindow* window, int key, 
                int scancode, int action, int mods) {

                if (action == GLFW_PRESS) {
                    switch (key) {
                    case GLFW_KEY_W:
                        input.bForward = true;
                        break;
                    case GLFW_KEY_S:
                        input.bBackward = true;
                        break;
                    case GLFW_KEY_A:
                        input.bLeft = true;
                        break;
                    case GLFW_KEY_D:
                        input.bRight = true;
                        break;
                    default:
                        break;
                    } 
                } else if (action == GLFW_RELEASE) {
                    switch (key) {
                    case GLFW_KEY_W:
                        input.bForward = false;
                        break;
                    case GLFW_KEY_S:
                        input.bBackward = false;
                        break;
                    case GLFW_KEY_A:
                        input.bLeft = false;
                        break;
                    case GLFW_KEY_D:
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

    void FirstPersonCameraSystem::RegisterInterfaces(core::InterfaceCollection& interfaces) {
    }

    void FirstPersonCameraSystem::Shutdown() {
        mWindowProvider->RemoveMouseButtonCallback(mMouseButtonHandle);
        mWindowProvider->RemoveCursorPosCallback(mMousePosHandle);
        mWindowProvider->RemoveKeyCallback(mKeyboardHandle);
    }

    void FirstPersonCameraSystem::LoadResources(marl::WaitGroup& waitGroup) {
    }
    void FirstPersonCameraSystem::SetFrame(core::Frame& frame) {
    }
    void FirstPersonCameraSystem::RequestSync(core::SyncObject& syncObject) {
        mWaitEvent.clear();

        mReads.RequestSync(syncObject);
        mWrites.RequestSync(syncObject);
    }
    void FirstPersonCameraSystem::Fork(core::Frame& frame,
        core::SyncObject& syncObject,
        const core::Time& time) {

        marl::schedule([window = mWindowProvider,
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

            window->WaitForInput();

            if (mouse.bCaptureMouse && mouse.bMouseMove) {
                input.mRotateX = mouse.mDMouseX;
                input.mRotateY = mouse.mDMouseY;
                mouse.bMouseMove = false;
            } else {
                input.mRotateX = 0.0;
                input.mRotateY = 0.0;
            }

            reads.Read<core::Transform>([&]() {
                auto view = frame.Registry().view<
                    core::Transform, 
                    FirstPersonController>();

                for (auto e : view) {
                    auto& transform = view.get<core::Transform>(e);
                    auto& controller = view.get<FirstPersonController>(e);
                    controller.PrepareUpdate(input, transform, time);
                }
            });

            writes.Write<core::Transform>([&]() {
                auto view = frame.Registry().view<
                    core::Transform, 
                    FirstPersonController>();

                for (auto e : view) {
                    auto& transform = view.get<core::Transform>(e);
                    auto& controller = view.get<FirstPersonController>(e);
                    controller.FlushUpdate(transform);
                }
            });
        });
    }
    void FirstPersonCameraSystem::Join(core::Frame& frame) {
        Wait();
    }
    void FirstPersonCameraSystem::Wait() {
        mWaitEvent.wait();
    }
    void FirstPersonCameraSystem::Request(const entt::meta_type& interfaceType) {
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
        const core::Transform& transform,
        const core::Time& time) {
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
        core::Transform& output) {
        output.mTranslation += mDPos;
        output.mRotation = mQX * mQY;
    }

    std::unique_ptr<core::ISystem> CreateFPSCameraSystem(core::ISystem* inputSystem) {
        return std::make_unique<diligent::FirstPersonCameraSystem>(inputSystem);
    }
}