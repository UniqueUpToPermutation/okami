#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

#include <okami/BoundingBox.hpp>

namespace okami::core {
	struct Transform {
	public:
		glm::vec3 mTranslation = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 mScale = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::quat mRotation = glm::identity<glm::quat>();

		template <typename Archive>
		void serialize(Archive& arr) {
			arr(mTranslation);
			arr(mScale);
			arr(mRotation);
		}

		glm::mat4 ToMatrix() const;

		static void Register();

		static Transform LookAt(
			const glm::vec3& eye, 
			const glm::vec3& target,
			const glm::vec3& up);

		inline glm::vec3 ApplyToPoint(const glm::vec3& vec) const {
			return mRotation * (mScale * vec) + mTranslation;
		}

		inline glm::vec3 ApplyToTangent(const glm::vec3& vec) const {
			return mRotation * (mScale * vec);
		}

		inline glm::vec3 ApplyToCotangent(const glm::vec3& vec) const {
			return mScale * (glm::inverse(mRotation) * vec);
		}

		inline glm::vec3 ApplyToNormal(const glm::vec3& vec) const {
			return (1.0f / mScale) * (mRotation * vec);
		}

		inline glm::vec4 Apply(const glm::vec4& vec) const {
			return glm::vec4(
				ApplyToTangent(glm::vec3(vec.x, vec.y, vec.z)) + mTranslation * vec.w, 
				vec.w);
		}

		inline glm::vec2 Apply(const glm::vec2& vec) const {
			glm::vec3 v3 = ApplyToPoint(glm::vec3(vec, 0.0f));
			return glm::vec2(v3.x, v3.y);
		}

		BoundingBox ApplyToAABB(const BoundingBox& box) const;

		inline BoundingBox2D ApplyToAABB(const BoundingBox2D& box) const {
			BoundingBox box3d{
				glm::vec3(box.mLower, 0.0f),
				glm::vec3(box.mUpper, 0.0f)
			};

			box3d = ApplyToAABB(box3d);

			return BoundingBox2D{
				{ box3d.mLower.x, box3d.mLower.y },
				{ box3d.mUpper.x, box3d.mUpper.y }
			};
		}

		inline Transform& SetTranslate(float x, float y, float z) {
			mTranslation = glm::vec3(x, y, z);
			return *this;
		}

		inline Transform& SetScale(float x, float y, float z) {
			mScale = glm::vec3(x, y, z);
			return *this;
		}

		inline Transform& SetRotation(const glm::quat& value) {
			mRotation = value;
			return *this;
		}
	};
}