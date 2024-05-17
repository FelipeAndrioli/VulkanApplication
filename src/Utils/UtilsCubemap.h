#pragma once

#include <glm/glm.hpp>

#include "Bitmap.h"

namespace Engine::Utils {
	Bitmap convertEquirectangularMapToVerticalCross(const Bitmap& b);
	Bitmap convertVerticalCrossToCubeMapFaces(const Bitmap& b);
	glm::vec3 faceCoordsToXYZ(int i, int j, int faceId, int faceSize);
}