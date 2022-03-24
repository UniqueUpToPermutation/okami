#include <okami/diligent/Im3dGizmo.hpp>
#include <okami/diligent/RenderModule.hpp>
#include <okami/diligent/Im3dSystem.hpp>
#include <okami/Observer.hpp>

#include <marl/containers.h>

namespace okami::graphics::diligent {

    Im3dGizmo::Im3dGizmo() : 
        mWaitEvent(marl::Event::Mode::Auto),
        mId(Im3d::MakeId("Gizmo")) {
    }

    void Im3dGizmo::ResetGizmo(core::Frame& frame) {
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

    void Im3dGizmo::NoDepthCallback() {
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
    }

    void Im3dGizmo::SetFrame(core::Frame& frame) {
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

    void Im3dGizmo::RequestSync(core::SyncObject& syncObject) {
        mWrites.RequestSync(syncObject);
        mReads.RequestSync(syncObject);
        mWaits.RequestSync(syncObject);
    }

    void Im3dGizmo::Update(
        core::SyncObject& syncObject,
        core::Frame& frame) {
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

        mWaits.Wait<core::Transform, GizmoSelectTag>();

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
    }

    bool Im3dGizmo::ShouldCaptureMouse() const {
        return bShouldCaptureMouse;
    }

    bool Im3dGizmo::ShouldCaptureKeyboard() const {
        return false;
    }

    void Im3dGizmo::SetMode(GizmoSelectionMode mode) {
        bResetId = true;
        mMode = mode;
    }

    GizmoSelectionMode Im3dGizmo::GetMode() const {
        return mMode;
    }
}