#include <okami/Transform.hpp>
#include <okami/Meta.hpp>

#include <entt/entt.hpp>

#include <glm/gtx/quaternion.hpp>

using namespace entt;

namespace okami::core {
    void Transform::Register() {
        meta<Transform>()
            .type("Transform"_hs)
            .ctor()
            .data<&Transform::mRotation>("Rotation"_hs)
            .data<&Transform::mScale>("Scale"_hs)
            .data<&Transform::mTranslation>("Translation"_hs);

        RegisterConcept<Transform, ICopyableConcept>();
    }

    glm::mat4 Transform::ToMatrix() const {
		glm::mat4 mat = glm::identity<glm::mat4>();
        mat = glm::translate(mat, mTranslation);
        mat = mat * glm::toMat4(mRotation);
        mat = glm::scale(mat, mScale);
        return mat;
    }

    Transform Transform::LookAt(
        const glm::vec3& eye, 
        const glm::vec3& target,
        const glm::vec3& up) {
        
        glm::vec3 zdir = glm::normalize(target - eye);
        glm::vec3 xdir = -glm::normalize(glm::cross(zdir, up));
        glm::vec3 ydir = -glm::normalize(glm::cross(xdir, zdir));

        auto rot = glm::toQuat(glm::mat4(
            xdir.x, ydir.x, zdir.x, 0.0f,
            xdir.y, ydir.y, zdir.y, 0.0f,
            xdir.z, ydir.z, zdir.z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        ));

        return Transform{
            eye,
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::inverse(rot)
        };
    }  

    		BoundingBox Transform::ApplyToAABB(const BoundingBox& box) const {
			glm::vec3 axes[] = { 
				{ 1.0f, 0.0f, 0.0f },
				{ 0.0f, 1.0f, 0.0f },
				{ 0.0f, 0.0f, 1.0f }
			};

			for (int i = 0; i < 3; ++i) {
				axes[i] = ApplyToTangent(axes[i]) * 
					(box.mUpper[i] - box.mLower[i]);
			}

			glm::vec3 t_lower = ApplyToPoint(box.mLower);
			glm::vec3 t_upper = ApplyToPoint(box.mUpper);

			glm::vec3 v[] = {
				t_lower,
				t_lower + axes[0],
				t_lower + axes[1],
				t_lower + axes[2],
				t_upper - axes[0],
				t_upper - axes[1],
				t_upper - axes[2],
				t_upper
			};

			glm::vec3 n_upper(
				-std::numeric_limits<float>::infinity(),
				-std::numeric_limits<float>::infinity(),
				-std::numeric_limits<float>::infinity());

			glm::vec3 n_lower(
				std::numeric_limits<float>::infinity(),
				std::numeric_limits<float>::infinity(),
				std::numeric_limits<float>::infinity());

			for (int i = 0; i < 8; ++i) {
				n_upper = glm::max(n_upper, v[i]);
				n_lower = glm::min(n_lower, v[i]);
			}

			return BoundingBox{n_lower, n_upper};
		}
}