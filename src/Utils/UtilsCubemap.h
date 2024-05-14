#pragma once

#include <glm/glm.hpp>

#include "Bitmap.h"

namespace Utils {

	Bitmap convertEquirectangularMapToVerticalCross(const Bitmap& b) {
		if (b.type)
	}

	// Helper function that maps integer coordinates inside a specified cube map 
	// as floating-point normalized coordinates. This helper is handy because all
	//  the faces of the vertical cross cube map have different vertical orientations.
	glm::vec3 faceCoordsToXYZ(int i, int j, int faceId, int faceSize) {
		const float A = 2.0f * float(i) / faceSize;
		const float B = 2.0f * float(j) / faceSize;

		if (faceId == 0)
			return glm::vec3(-1.0f, A - 1.0f, B - 1.0f);
		if (faceId == 1)
			return glm::vec3(A - 1.0f, -1.0f, 1.0f - B);
		if (faceId == 2)
			return glm::vec3(1.0f, A - 1.0f, 1.0f - B);
		if (faceId == 3)
			return glm::vec3(1.0f - A, 1.0f, 1.0f - B);
		if (faceId == 4)
			return glm::vec3(B - 1.0f, A - 1.0f, 1.0f);
		if (faceId == 5)
			return glm::vec3(1.0f - B, A - 1.0f, -1.0f);

		return glm::vec3(1.0f);
	}
}