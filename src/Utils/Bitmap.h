#pragma once

#include <string.h>
#include <vector>

#include <glm/glm.hpp>

enum eBitmapType {
	eBitmapType_2D,
	eBitmapType_Cube
};

enum eBitmapFormat {
	eBitmapFormat_UnsignedByte,
	eBitmapFormat_Float
};

// R/RG/RGB/RGBA bitmaps
class Bitmap {
public:
	Bitmap() = default;
	Bitmap(int width, int height, int comp, eBitmapFormat format) : m_Width(width), m_Height(height), m_Comp(comp), m_Format(format),
		Data(width * height * comp * getBytesPerComponent(format)) {
		initGetSetFuncs();
	}

	Bitmap(int width, int height, int depth, int comp, eBitmapFormat format) : m_Width(width), m_Height(height), m_Depth(depth),
		m_Comp(comp), m_Format(format), Data(width * height * depth * comp * getBytesPerComponent(format)) {
		initGetSetFuncs();
	}

	Bitmap(int width, int height, int comp, eBitmapFormat format, const void* data) : m_Width(width), m_Height(height),
		m_Comp(comp), m_Format(format), Data(width * height * comp * getBytesPerComponent(format)) {

		initGetSetFuncs();
		memcpy(Data.data(), data, Data.size());
	}

	int getWidth() const { return m_Width; }
	int getHeight() const { return m_Height; }
	int getDepth() const { return m_Depth; }
	int getComp() const { return m_Comp; }

	eBitmapFormat getFormat() const { return m_Format; }
	eBitmapType getType() const { return m_Type; }
	void setType(eBitmapType type) { m_Type = type; }

	std::vector<uint8_t> Data;

	static int getBytesPerComponent(eBitmapFormat format) {
		if (format == eBitmapFormat_UnsignedByte) return 1;
		if (format == eBitmapFormat_Float) return 4;
		return 0;
	}

	void setPixel(int x, int y, const glm::vec4& color) {
		(*this.*setPixelFunc)(x, y, color);
	}

	glm::vec4 getPixel(int x, int y) const {
		return ((*this.*getPixelFunc)(x, y));
	}

private:
	using setPixel_t = void(Bitmap::*)(int, int, const glm::vec4&);
	using getPixel_t = glm::vec4(Bitmap::*)(int, int) const;
	setPixel_t setPixelFunc = &Bitmap::setPixelUnsignedByte;
	getPixel_t getPixelFunc = &Bitmap::getPixelUnsignedByte;

	void initGetSetFuncs() {
		switch (m_Format) {
		case eBitmapFormat_UnsignedByte:
			setPixelFunc = &Bitmap::setPixelUnsignedByte;
			getPixelFunc = &Bitmap::getPixelUnsignedByte;
			break;
		case eBitmapFormat_Float:
			setPixelFunc = &Bitmap::setPixelFloat;
			getPixelFunc = &Bitmap::getPixelFloat;
			break;
		}
	}

	void setPixelFloat(int x, int y, const glm::vec4& color) {
		const int offset = m_Comp * (y * m_Width + x);
		float* data = reinterpret_cast<float*>(Data.data());
		if (m_Comp > 0) data[offset + 0] = color.x;
		if (m_Comp > 1) data[offset + 1] = color.y;
		if (m_Comp > 2) data[offset + 2] = color.z;
		if (m_Comp > 3) data[offset + 3] = color.w;
	}

	glm::vec4 getPixelFloat(int x, int y) const {
		const int offset = m_Comp * (y * m_Width + x);
		const float* data = reinterpret_cast<const float*>(Data.data());

		return glm::vec4(
			m_Comp > 0 ? data[offset + 0] : 0.0f,
			m_Comp > 1 ? data[offset + 1] : 0.0f,
			m_Comp > 2 ? data[offset + 2] : 0.0f,
			m_Comp > 3 ? data[offset + 3] : 0.0f
		);
	}

	void setPixelUnsignedByte(int x, int y, const glm::vec4& color) {
		const int offset = m_Comp * (y * m_Width + x);

		if (m_Comp > 0) Data[offset + 0] = uint8_t(color.x * 255.0f);
		if (m_Comp > 1) Data[offset + 1] = uint8_t(color.y * 255.0f);
		if (m_Comp > 2) Data[offset + 2] = uint8_t(color.z * 255.0f);
		if (m_Comp > 3) Data[offset + 3] = uint8_t(color.w * 255.0f);
	}

	glm::vec4 getPixelUnsignedByte(int x, int y) const {
		const int offset = m_Comp * (y * m_Width + x);

		return glm::vec4(
			m_Comp > 0 ? float(Data[offset + 0]) / 255.0f : 0.0f,
			m_Comp > 1 ? float(Data[offset + 1]) / 255.0f : 0.0f,
			m_Comp > 2 ? float(Data[offset + 2]) / 255.0f : 0.0f,
			m_Comp > 3 ? float(Data[offset + 3]) / 255.0f : 0.0f
		);
	}

private:
	int m_Width = 0;
	int m_Height = 0;
	int m_Depth = 1;
	int m_Comp = 3;

	eBitmapFormat m_Format = eBitmapFormat_UnsignedByte;
	eBitmapType m_Type = eBitmapType_2D;
};
