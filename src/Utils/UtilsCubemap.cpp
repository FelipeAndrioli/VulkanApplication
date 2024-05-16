#include "UtilsCubemap.h"

#include <math.h>

constexpr auto M_PI = 3.141592653589793238462643383279502884197169399375105820974944;

namespace Utils {
	Bitmap convertEquirectangularMapToVerticalCross(const Bitmap& b) {
		/*
			from:

			|------------|
			|            |
			|____________|

			to:
				
			   	|---|
			|---|___|---|
			|___|___|___|
				|___|
				|___|
		
		*/

		//if (b.getType() == eBitmapType_2D) return Bitmap();
		if (b.getType() == eBitmapType_2D) return Bitmap();

		const int faceSize = b.getWidth() / 4;
		const int width = faceSize * 3;			// 3 horizontal planes 
		const int height = faceSize * 4;		// 4 vertical planes 

		Bitmap result(width, height, b.getDepth(), b.getFormat());

		// define the locations of individual faces inside the cross
		const glm::vec2 kFaceOffsets[] = {
			glm::vec2(faceSize, faceSize * 3),
			glm::vec2(0, faceSize),
			glm::vec2(faceSize, faceSize),
			glm::vec2(faceSize * 2, faceSize),
			glm::vec2(faceSize, 0),
			glm::vec2(faceSize, faceSize * 2)
		};

		// necessary to clamp the texture lookup
		const int clampW = b.getWidth() - 1;
		const int clampH = b.getHeight() - 1;

		// iterate over six cube map faces and each pixel inside each face
		for (int face = 0; face < 6; face++) {
			for (int i = 0; i < faceSize; i++) {
				for (int j = 0; j < faceSize; j++) {

					// calculate latitude and longitude coordinates of the Cartesian cube 
					// map coordinates
					const glm::vec3 P = faceCoordsToXYZ(i, j, face, faceSize);
					const float R = hypot(P.x, P.y);
					const float theta = atan2(P.y, P.x);
					const float phi = atan2(P.z, R);

					// map the latitude and longitude of the floating-point coordinates 
					// inside the equirectangular image
					const float Uf = float(2.0f * faceSize * (theta + M_PI) / M_PI);
					const float Vf = float(2.0f * faceSize * (M_PI / 2.0f - phi) / M_PI);

					// based on floating-point coordinates, we will get two pairs of integer
					// UV coordinates. We will use these to sample four texels for bilinear
					// interpolation.
					const int U1 = glm::clamp(int(floor(Uf)), 0, clampW);
					const int V1 = glm::clamp(int(floor(Vf)), 0, clampH);
					const int U2 = glm::clamp(U1 + 1, 0, clampW);
					const int V2 = glm::clamp(V1 + 1, 0, clampH);

					// get the fractional part for bilinear interpolation
					const float s = Uf - U1;
					const float t = Vf - V1;

					// fetch four samples from the equirectangular map
					const glm::vec4 A = b.getPixel(U1, V1);
					const glm::vec4 B = b.getPixel(U2, V1);
					const glm::vec4 C = b.getPixel(U1, V2);
					const glm::vec4 D = b.getPixel(U2, V2);

					// performing bilinear interpolation and setting the resulting pixel
					// value in the vertical cross cubemap.
					const glm::vec4 color = A * (1 - s) * (1 - t) +
						B * (s) * (1 - t) +
						C * (1 - s) * t + 
						D * (s) * t;

					result.setPixel(i + kFaceOffsets[face].x, j + kFaceOffsets[face].y, color);
				}
			}
		}

		return result;
	}

	Bitmap convertVerticalCrossToCubeMapFaces(const Bitmap& b) {
		/* cubemap faces layout (originally for OpenGL, need to test it for Vulkan)
		   ____
		   |+y|
		 __|__|__
		|-x|-z|+x|
		----------
		   |-Y|
		   |--|
		   |+z|
		   ----
		*/
		const int faceWidth = b.getWidth() / 3;
		const int faceHeight = b.getHeight() / 4;

		Bitmap cubemap(faceWidth, faceHeight, b.getComp(), b.getFormat());

		const uint8_t* src = b.getData().data();
		uint8_t* dst = cubemap.getData().data();
		
		const int pixelSize = cubemap.getComp() * Bitmap::getBytesPerComponent(cubemap.getFormat());

		for (int face = 0; face < 6; face++) {
			for (int j = 0; j < faceHeight; j++) {
				for (int i = 0; i < faceWidth; i++) {
					int x = 0;
					int y = 0;

					switch (face) {
						// positive x
						case 0:
							x = i;
							y = faceHeight + j;
							break;
						// negative x
						case 1:
							x = 2 * faceWidth + i;
							y = 1 * faceHeight + j;
							break;
						// positive y
						case 2:
							x = 2 * faceWidth - (i + 1);
							y = 1 * faceHeight - (j + 1);
							break;
						// negative y
						case 3:
							x = 2 * faceWidth - (i + 1);
							y = 3 * faceHeight - (j + 1);
							break;
						// positive z
						case 4:
							x = 2 * faceWidth - (i + 1);
							y = b.getHeight() - (j + 1);
							break;
						// negative z
						case 5:
							x = faceWidth + i;
							y = faceHeight + j;
							break;
						default:
							break;
					}

					memcpy(dst, src + (y * b.getWidth() + x) * pixelSize, pixelSize);
					dst += pixelSize;
				}
			}
		}

		return cubemap;
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