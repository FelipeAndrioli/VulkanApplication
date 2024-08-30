#include "UtilsCubemap.h"

#include <cstdio>
#include <glm/glm.hpp>
#include <glm/ext.hpp> 

namespace Utils {

	static constexpr float PI = 3.14159265359f;

	glm::vec3 faceCoordsToXYZ(int i, int j, int faceId, int faceSize) {
		const float A = 2.0f * float(i) / faceSize;
		const float B = 2.0f * float(j) / faceSize;

		if (faceId == 0) return glm::vec3(-1.0f, A - 1.0f, B - 1.0f);
		if (faceId == 1) return glm::vec3(A - 1.0f, -1.0f, 1.0f - B);
		if (faceId == 2) return glm::vec3(1.0f, A - 1.0f, 1.0f - B);
		if (faceId == 3) return glm::vec3(1.0f - A, 1.0f, 1.0f - B);
		if (faceId == 4) return glm::vec3(B - 1.0f, A - 1.0f, 1.0f);
		if (faceId == 5) return glm::vec3(1.0f - B, A - 1.0f, -1.0f);

		return glm::vec3();
	}

	Bitmap convertEquirectangularMapToVerticalCross(const Bitmap& b) {
		if (b.getType() != eBitmapType_2D) return Bitmap();

		const int faceSize = b.getWidth() / 4;

		const int width = faceSize * 3;
		const int height = faceSize * 4;

		Bitmap result(width, height, b.getComp(), b.getFormat());

		const glm::vec2 kFaceOffsets[] = {
			glm::vec2(faceSize, faceSize * 3),
			glm::vec2(0, faceSize),
			glm::vec2(faceSize, faceSize),
			glm::vec2(faceSize * 2, faceSize),
			glm::vec2(faceSize, 0),
			glm::vec2(faceSize, faceSize * 2)
		};

		const int clampW = b.getWidth() - 1;
		const int clampH = b.getHeight() - 1;

		for (int face = 0; face < 6; face++) {
			for (int i = 0; i < faceSize; i++) {
				for (int j = 0; j < faceSize; j++) {

					const glm::vec3 P = faceCoordsToXYZ(i, j, face, faceSize);
					const float R = hypot(P.x, P.y);
					const float theta = atan2(P.y, P.x);
					const float phi = atan2(P.z, R);

					//	float point source coordinates
					const float Uf = float(2.0f * faceSize * (theta + PI) / PI);
					const float Vf = float(2.0f * faceSize * (PI / 2.0f - phi) / PI);

					// 4-samples for bilinear interpolation
					const int U1 = glm::clamp(int(floor(Uf)), 0, clampW);
					const int V1 = glm::clamp(int(floor(Vf)), 0, clampH);
					const int U2 = glm::clamp(U1 + 1, 0, clampW);
					const int V2 = glm::clamp(V1 + 1, 0, clampH);

					// fractional part
					const float s = Uf - U1;
					const float t = Vf - V1;

					// fetch 4-samples
					const glm::vec4 A = b.getPixel(U1, V1);
					const glm::vec4 B = b.getPixel(U2, V1);
					const glm::vec4 C = b.getPixel(U1, V2);
					const glm::vec4 D = b.getPixel(U2, V2);

					// bilinear interpolation
					const glm::vec4 color = A * (1 - s) * (1 - t) + B * (s) * (1 - t) + C * (1 - s) * t + D * (s) * (t);

					result.setPixel(i + kFaceOffsets[face].x, j + kFaceOffsets[face].y, color);
				}
			}
		}

		return result;
	}

	Bitmap convertVerticalCrossToCubeMapFaces(const Bitmap& b) {
		const int faceWidth = b.getWidth() / 3;
		const int faceHeight = b.getHeight() / 4;

		Bitmap cubemap(faceWidth, faceHeight, 6, b.getComp(), b.getFormat());
		cubemap.setType(eBitmapType_Cube);

		const uint8_t* src = b.Data.data();
		uint8_t* dst = cubemap.Data.data();

		/*
				------
				| +Y |
		 ----------------
		 | -X | -Z | +X |
		 ----------------
				| -Y |
				------
				| +Z |
				------
		*/

		const int pixelSize = cubemap.getComp() * Bitmap::getBytesPerComponent(cubemap.getFormat());

		for (int face = 0; face < 6; face++) {
			for (int j = 0; j < faceHeight; j++) {
				for (int i = 0; i < faceWidth; i++) {
					int x = 0;
					int y = 0;

					switch (face) {
						// GL_TEXTURE_CUBE_MAP_POSITIVE_X
					case 0:
						x = i;
						y = faceHeight + j;
						break;

						// GL_TEXTURE_CUBE_MAP_NEGATIVE_X
					case 1:
						x = 2 * faceWidth + i;
						y = 1 * faceHeight + j;
						break;

						// GL_TEXTURE_CUBE_MAP_POSITIVE_Y
					case 2:
						x = 2 * faceWidth - (i + 1);
						y = 1 * faceHeight - (j + 1);
						break;

						// GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
					case 3:
						x = 2 * faceWidth - (i + 1);
						y = 3 * faceHeight - (j + 1);
						break;

						// GL_TEXTURE_CUBE_MAP_POSITIVE_Z
					case 4:
						x = 2 * faceWidth - (i + 1);
						y = b.getHeight() - (j + 1);
						break;

						// GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
					case 5:
						x = faceWidth + i;
						y = faceHeight + j;
						break;
					}

					memcpy(dst, src + (y * b.getWidth() + x) * pixelSize, pixelSize);

					dst += pixelSize;
				}
			}
		}

		return cubemap;
	}
}
