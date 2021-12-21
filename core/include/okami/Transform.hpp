#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

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
	};
}