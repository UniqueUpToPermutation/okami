#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

namespace okami {
	struct BoundingBox {
		glm::vec3 mLower;
		glm::vec3 mUpper;
	};

	struct BoundingBox2D {
		glm::vec2 mLower;
		glm::vec2 mUpper;
	};
}