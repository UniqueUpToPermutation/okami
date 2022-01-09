#include <okami/diligent/Im3dGizmo.hpp>
#include <okami/diligent/RenderModule.hpp>
#include <okami/diligent/Im3dSystem.hpp>
#include <okami/Observer.hpp>

#include <marl/containers.h>

namespace okami::graphics::diligent {

    struct GizmoSelectTagLocal {
    };

    struct Im3dGizmoTransform {
        glm::mat3 mRotation = glm::identity<glm::mat3>();
        glm::vec3 mTranslation = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 mScale = glm::vec3(1.0f, 1.0f, 1.0f);
    };

    class Im3dGizmoSystem : 
        public core::ISystem,
        public IInputCapture,
        public IGizmo {
    public:    
        marl::Event mWaitEvent;
        marl::Event mTransformUpdated;

        Im3dGizmoTransform mTransform;
        Im3dGizmoTransform mLastTransform;

        core::Observer<GizmoSelectTag, 
            core::ObserverType::ON_CONSTRUCT, 4U> mConstructObs;
        core::Observer<GizmoSelectTag,
            core::ObserverType::ON_DESTROY, 4U> mDestroyObs;

        core::UpdaterWrites<core::Transform> mWrites;
        core::UpdaterReads<> mReads;
        core::UpdaterWaits<core::Transform,
            GizmoSelectTag> mWaits;

        core::delegate_handle_t mIm3dHandle;
        core::delegate_handle_t mMousePosHandle;

        GizmoSelectionMode mMode = GizmoSelectionMode::TRANSLATION;

        IIm3dCallback* mIm3dInterface;

        Im3d::Id mId;

        bool bActive = false;
        bool bResetId = false;
        bool bShouldCaptureMouse = false;

        Im3dGizmoSystem(IIm3dCallback* im3d) : 
            mWaitEvent(marl::Event::Mode::Auto),
            mIm3dInterface(im3d),
            mId(Im3d::MakeId("Gizmo")) {
        }

        void ResetGizmo(core::Frame& frame) {
            auto view = frame.Registry().view<
                GizmoSelectTagLocal, 
                core::Transform>();

            glm::vec3 averageTranslate(0.0f, 0.0f, 0.0f);
            uint totalCount = 0;

            for (auto e : view) {
                auto& transform = view.get<core::Transform>(e);
                averageTranslate += transform.mTranslation;
                totalCount++;
            }

            if (totalCount == 0) {
                bActive = false;
            } else {
                bActive = true;
                
                averageTranslate /= totalCount;

                mTransform = Im3dGizmoTransform();
                mTransform.mTranslation = averageTranslate;
            }
        }

        void Startup(marl::WaitGroup& waitGroup) override {
            if (!mIm3dInterface->SupportsInput()) {
                throw std::runtime_error("Im3dGizmo can only be used with "
                    "an Im3dSystem that supports input!"
                    " Please initialize Im3dSystem with an input system!");
            }

            mIm3dHandle = mIm3dInterface->AddNoDepth([this]() {
                defer(mTransformUpdated.signal());

                if (bResetId) {
                    Im3d::GetContext().resetId();
                    bResetId = false;
                }

                auto transformS = glm::mat4(mTransform.mRotation);
                transformS[3][0] = mTransform.mTranslation[0];
                transformS[3][1] = mTransform.mTranslation[1];
                transformS[3][2] = mTransform.mTranslation[2];

                auto transformR = glm::identity<glm::mat4>();
                transformR = glm::translate(
                    transformR, mTransform.mTranslation);

                if (bActive) {
                    switch (mMode) {
                    case GizmoSelectionMode::TRANSLATION:
                        Im3d::GizmoTranslation(mId, 
                            &mTransform.mTranslation[0],
                            false);
                        break;
                    case GizmoSelectionMode::ROTATION:
                        Im3d::PushMatrix(ToIm3d(transformR));
                        Im3d::GizmoRotation(mId,
                            &mTransform.mRotation[0][0],
                            false);
                        Im3d::PopMatrix();
                        break;
                    case GizmoSelectionMode::SCALE:
                        Im3d::PushMatrix(ToIm3d(transformS));
                        Im3d::GizmoScale(mId,
                            &mTransform.mScale[0]);
                        Im3d::PopMatrix();
                        break;
                    }
                }
            });
        }

        void RegisterInterfaces(core::InterfaceCollection& interfaces) override {
            interfaces.Add<IGizmo>(this);
        }
        
        void Shutdown() override {
            mIm3dInterface->Remove(mIm3dHandle);
        }

        void LoadResources(marl::WaitGroup& waitGroup) override {
        }

        void SetFrame(core::Frame& frame) override {
            mConstructObs.attach(frame);
            mDestroyObs.attach(frame);

            auto tagged = frame.Registry().view<GizmoSelectTag>();
            frame.Registry().clear<GizmoSelectTagLocal>();
            for (auto e : tagged) {
                frame.Registry().emplace<GizmoSelectTagLocal>(e);
            }

            if (tagged.size() > 0) {
                bActive = true;
            }
        }

        void RequestSync(core::SyncObject& syncObject) override {
            mWrites.RequestSync(syncObject);
            mReads.RequestSync(syncObject);
            mWaits.RequestSync(syncObject);
            mWaitEvent.clear();
        }

        void Fork(core::Frame& frame,
            core::SyncObject& syncObject,
            const core::Time& time) override {
            marl::schedule([this, &frame,
                &reads = mReads,
                &writes = mWrites,
                &waits = mWaits,
                &syncObject,
                time]() {
                defer(mWaitEvent.signal());
                defer(mReads.ReleaseHandles());
                defer(mWrites.ReleaseHandles());

                mTransformUpdated.wait();

                mWrites.Write<core::Transform>([&]() {
                    auto view = frame.Registry().view<
                        GizmoSelectTagLocal, core::Transform>();
                    switch (mMode) {
                    case GizmoSelectionMode::TRANSLATION:
                    {
                        auto diff = mTransform.mTranslation - 
                            mLastTransform.mTranslation;
                        for (auto e : view) {
                            auto& transform = view.get<core::Transform>(e);
                            transform.mTranslation += diff;
                        }
                        break;
                    }
                    case GizmoSelectionMode::ROTATION:
                    {
                        auto rotNew = glm::quat_cast(mTransform.mRotation);
                        auto rotOld = glm::quat_cast(mLastTransform.mRotation);
                        auto diffRot = rotNew * glm::inverse(rotOld);

                        for (auto e : view) {
                            auto& transform = view.get<core::Transform>(e);
                            transform.mRotation = diffRot * transform.mRotation;
                        }
                        break;
                    }
                    case GizmoSelectionMode::SCALE:
                        auto diff = mTransform.mScale / mLastTransform.mScale;

                        for (auto e : view) {
                            auto& transform = view.get<core::Transform>(e);
                            transform.mScale = diff * transform.mScale;
                        }
                        break;                        
                    }
                });
                    
                mLastTransform = mTransform;

                waits.Wait<core::Transform, GizmoSelectTag>();

                // Replicate changes to gizmo selection locally
                {
                    bool bShouldReset = false;
                    for (auto e : mConstructObs) {
                        frame.Registry().emplace<GizmoSelectTagLocal>(e);
                        bShouldReset = true;
                    }

                    for (auto e : mDestroyObs) {
                        frame.Registry().remove<GizmoSelectTagLocal>(e);
                        bShouldReset = true;
                    }

                    // If there were changes made to the selection,
                    // reset the Gizmo's position / activation
                    if (bShouldReset) {
                        mConstructObs.clear();
                        mDestroyObs.clear();
                        ResetGizmo(frame);
                    }
                }
            });
        }

        void Join(core::Frame& frame) override {
            Wait();
        }

        void Wait() override {
            mWaitEvent.wait();
        }

        void EnableInterface(const entt::meta_type& interfaceType) override {
        }

        bool ShouldCaptureMouse() const override {
            return bShouldCaptureMouse;
        }

        bool ShouldCaptureKeyboard() const override {
            return false;
        }

        void SetMode(GizmoSelectionMode mode) override {
            bResetId = true;
            mMode = mode;
        }

        GizmoSelectionMode GetMode() const override {
            return mMode;
        }
    };
}

namespace okami::graphics {
    std::unique_ptr<core::ISystem> CreateGizmoSystem(
        core::ISystem* im3dSystem) {

        core::InterfaceCollection collection(im3dSystem);
        auto im3d = collection.Query<IIm3dCallback>();

        if (!im3d) {
            throw std::runtime_error("IIm3dCallback interface is not exposed!");
        }

        return std::make_unique<diligent::Im3dGizmoSystem>(im3d);
    }
}