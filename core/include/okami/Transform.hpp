#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

namespace okami::core {
	class Transform {
	private:
		glm::vec3 mTranslation;
		glm::vec3 mScale;
		glm::quat mRotation;

	public:
		inline Transform() : 
			mTranslation(0.0f, 0.0f, 0.0f),
			mScale(1.0f, 1.0f, 1.0f),
			mRotation(0.0f, 0.0f, 0.0f, 1.0f) {
		}

		inline Transform(glm::vec3 translation) :
			mTranslation(translation),
			mScale(1.0f, 1.0f, 1.0f),
			mRotation(0.0f, 0.0f, 0.0f, 1.0f) {
		}
			
		inline Transform(glm::vec3 translation,
			glm::quat rotation) : 
			mTranslation(translation),
			mScale(1.0f, 1.0f, 1.0f),
			mRotation(0.0f, 0.0f, 0.0f, 1.0f) {
		}

		inline Transform(glm::vec3 translation,
			glm::quat rotation,
			glm::vec3 scale) :
			mTranslation(translation),
			mScale(scale),
			mRotation(rotation) {
		}

		inline Transform& SetTranslation(const glm::vec3& t) {
			mTranslation = t;
			return *this;
		}

		inline Transform& SetTranslation(const float x, const float y, const float z) {
			SetTranslation(glm::vec3(x, y, z));
			return *this;
		}

		inline Transform& SetRotation(const glm::quat& q) {
			mRotation = q;
			return *this;
		}

		inline Transform& SetRotation(const glm::vec3& axis, const float angle) {
			mRotation = glm::angleAxis(angle, axis);
			return *this;
		}

		inline Transform& SetScale(const glm::vec3& s) {
			mScale = s;
			return *this;
		}

		inline Transform& SetScale(const float x, const float y, const float z) {
			SetScale(glm::vec3(x, y, z));
			return *this;
		}

		inline Transform& SetScale(const float s) {
			SetScale(glm::vec3(s, s, s));
			return *this;
		}

		inline glm::vec3 GetTranslation() const {
			return mTranslation;
		}

		inline glm::vec3 GetScale() const {
			return mScale;
		}

		inline glm::quat GetRotation() const {
			return mRotation;
		}

		glm::mat4 ToMatrix() const;

		template <typename Archive>
		void serialize(Archive& arr) {
			arr(mTranslation);
			arr(mScale);
			arr(mRotation);
		}

		static void Register();
	};
}