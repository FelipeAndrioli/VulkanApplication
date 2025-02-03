#include "./TextureLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "./Bitmap.h"
#include "./UtilsCubemap.h"
#include "./Helper.h"

#include "../Core/GraphicsDevice.h"
#include "../Core/Graphics.h"

using namespace Utils;

namespace TextureLoader {
	 Texture LoadTexture(const char* texturePath, Texture::TextureType textureType, bool flipTextureVertically, bool generateMipMaps) {

		Texture texture = {};

		int texWidth = 0;
		int texHeight = 0;
		int texChannels = 0;

		stbi_set_flip_vertically_on_load(flipTextureVertically);
		stbi_uc* pixels = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		VkDeviceSize imageSize = texWidth * texHeight * 4;

		uint32_t mipLevels = 1;

		if (generateMipMaps)
			mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

		if (!pixels) {
			throw std::runtime_error("Failed to load texture image!");
		}

		ImageDescription desc = {
			.Width = static_cast<uint32_t>(texWidth),
			.Height = static_cast<uint32_t>(texHeight),
			.MipLevels = static_cast<uint32_t>(mipLevels),
			.LayerCount = 1,
			.Format = VK_FORMAT_R8G8B8A8_SRGB,
			.Tiling = VK_IMAGE_TILING_OPTIMAL,
			.Usage = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
			.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
			.ViewType = VK_IMAGE_VIEW_TYPE_2D,
			.MsaaSamples = VK_SAMPLE_COUNT_1_BIT,
			.ImageType = VK_IMAGE_TYPE_2D,
			.AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT
		};

		GraphicsDevice* device = GetDevice();
		device->CreateTexture(desc, texture, textureType, pixels, imageSize);

		stbi_image_free(pixels);

		texture.Name = Helper::get_filename(texturePath);
		texture.Type = textureType;
		
		return texture;
	}

	uint32_t bytesPerTexFormat(VkFormat format) {
		switch (format) {
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_UNORM:
			return 1;
		case VK_FORMAT_R16_SFLOAT:
			return 2;
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SRGB:
			return 4;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return 4 * sizeof(uint16_t);
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 4 * sizeof(float);
		default:
			break;
		}

		return 0;
	}

	static void float24to32(int w, int h, const float* img24, float* img32) {
		const int numPixels = w * h;

		for (int i = 0; i < numPixels; i++) {
			*img32++ = *img24++;
			*img32++ = *img24++;
			*img32++ = *img24++;
			*img32++ = 1.0f;
		}
	}

	Texture LoadCubemapTexture(const char* texturePath) {
		Texture texture = {};
		const uint32_t mipLevels = 1;

		int width;
		int height;
		int comp;

		stbi_set_flip_vertically_on_load(false);

		const float* img = stbi_loadf(texturePath, &width, &height, &comp, 3);

		if (!img) {
			throw std::runtime_error("Failed to load image!");
		}

		// stbi_loadf only load 3 components, we need to convert it from float (8) 24 (RGB) float (8) to 32 (RGBA)
		// setting the fourth component to 1.0f, Vulkan doesn't have a R24G24B24 format
		std::vector<float> img32(width * height * 4);
		float24to32(width, height, img, img32.data());
		comp = 4;

		Bitmap in(width, height, comp, eBitmapFormat_Float, img32.data());
		Bitmap out = convertEquirectangularMapToVerticalCross(in);

		stbi_image_free((void*)img);

		Bitmap cubemap = convertVerticalCrossToCubeMapFaces(out);

		VkFormat texFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

		// update texture image
		uint32_t bytesPerPixel = bytesPerTexFormat(texFormat);
		uint32_t layerCount = 6;

		VkDeviceSize layerSize = cubemap.getWidth() * cubemap.getHeight() * bytesPerPixel;
		VkDeviceSize imageSize = layerSize * layerCount;

		ImageDescription desc = {
			.Width = static_cast<uint32_t>(cubemap.getWidth()),
			.Height = static_cast<uint32_t>(cubemap.getHeight()),
			.MipLevels = static_cast<uint32_t>(mipLevels),
			.LayerCount = static_cast<uint32_t>(layerCount),
			.Format = texFormat,
			.Tiling = VK_IMAGE_TILING_OPTIMAL,
			.Usage = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
			.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			.AspectFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
			.ViewType = VK_IMAGE_VIEW_TYPE_CUBE,
			.ImageType = VK_IMAGE_TYPE_2D,
			.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
		};

		GraphicsDevice* device = GetDevice();
		device->CreateTexture(desc, texture, Texture::TextureType::CUBEMAP_SINGLE, cubemap.Data.data(), imageSize);

		return texture;
	}

	Texture LoadCubemapTexture(std::vector<std::string> texturePaths) {
		/* - Correct order to load
		right
		left
		top				
		bottom	
		front		
		back	
		*/

		int width = 0;
		int height = 0;
		int channels = 0;

		uint32_t mipLevels = 1;
		const uint32_t layerCount = 6;

		unsigned char* pixels[layerCount] = {};
		
		stbi_set_flip_vertically_on_load(false);

		for (int i = 0; i < texturePaths.size(); i++) {
			pixels[i] = stbi_load(texturePaths[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);

			if (!pixels) {
				std::cout << "Failed to load texture: " << texturePaths[i].c_str() << '\n';
				throw std::runtime_error("Failed to load texture.");
			}
		}

		VkDeviceSize layerSize = width * height * 4;
		VkDeviceSize imageSize = layerSize * layerCount;
		
		std::vector<unsigned char> cubemapData(imageSize);

		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < layerSize; j++) {
				cubemapData[j + (i * layerSize)] = pixels[i][j];
			}
			stbi_image_free(pixels[i]);
		}
	
		Texture texture = {};

		ImageDescription desc = {
			.Width = static_cast<uint32_t>(width),
			.Height = static_cast<uint32_t>(height),
			.MipLevels = static_cast<uint32_t>(mipLevels),
			.LayerCount = static_cast<uint32_t>(layerCount),
			.Format = VK_FORMAT_R8G8B8A8_SRGB,
			.Tiling = VK_IMAGE_TILING_OPTIMAL,
			.Usage = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
			.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			.AspectFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
			.ViewType = VK_IMAGE_VIEW_TYPE_CUBE,
			.MsaaSamples = VK_SAMPLE_COUNT_1_BIT,
			.ImageType = VK_IMAGE_TYPE_2D,
			.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
		};

		GraphicsDevice* device = GetDevice();
		device->CreateTexture(desc, texture, Texture::TextureType::CUBEMAP_MULTI, cubemapData.data(), imageSize);

		return texture;
	}
}
