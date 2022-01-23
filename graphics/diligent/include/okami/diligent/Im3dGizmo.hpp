#pragma once

#include <okami/System.hpp>
#include <okami/Graphics.hpp>
#include <okami/Observer.hpp>
#include <okami/Transform.hpp>

#include <im3d.h>

namespace okami::graphics::diligent {

    struct GizmoSelectTagLocal {
    };

    struct Im3dGizmoTransform {
        glm::mat3 mRotation = glm::mat3(1.0f);
        glm::vec3 mTranslation = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 mScale = glm::vec3(1.0f, 1.0f, 1.0f);
    };

    class Im3dGizmo : 
        public core::IInputCapture,
        public IGizmo {
    private:    
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

        IIm3dSystem* mIm3dInterface;

        Im3d::Id mId;

        bool bActive = false;
        bool bResetId = false;
        bool bShouldCaptureMouse = false;

    public:
        Im3dGizmo();

        void ResetGizmo(core::Frame& frame);
        void NoDepthCallback();
        void SetFrame(core::Frame& frame);
        void RequestSync(core::SyncObject& syncObject);
        void Update(core::SyncObject& syncObject,
            core::Frame& frame);
        bool ShouldCaptureMouse() const override;
        bool ShouldCaptureKeyboard() const override;
        void SetMode(GizmoSelectionMode mode) override;
        GizmoSelectionMode GetMode() const override;
    };
}