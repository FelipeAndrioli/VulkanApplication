#pragma once

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

class Bitmap {
public:
	Bitmap() = default;
	Bitmap(int width, int height, int comp, eBitmapFormat fmt)
		: m_Width(width),
		m_Height(height),
		m_Comp(comp),
		m_Format(fmt),
		m_Data(m_Width* m_Height* m_Comp* getBytesPerComponent(m_Format)) {

		initGetSetFuncs();
	}

	Bitmap(int width, int height, int depth, int comp, eBitmapFormat fmt)
		: m_Width(width),
		m_Height(height),
		m_Depth(depth),
		m_Comp(comp),
		m_Format(fmt),
		m_Data(m_Width* m_Height* m_Depth* m_Comp* getBytesPerComponent(m_Format)) {

		initGetSetFuncs();
	}

	Bitmap(int width, int height, int comp, eBitmapFormat fmt, const void* ptr)
		: m_Width(width),
		m_Height(height),
		m_Comp(comp),
		m_Format(fmt),
		m_Data(m_Width* m_Height* m_Comp* getBytesPerComponent(m_Format)) {

		initGetSetFuncs();
		memcpy(m_Data.data(), ptr, m_Data.size());
	}

	static int getBytesPerComponent(eBitmapFormat fmt) {
		if (fmt == eBitmapFormat_UnsignedByte) return 1;
		if (fmt == eBitmapFormat_Float) return 4;

		return 0;
	}

	// TODO: rework this function to use std::function and std::bind
	void setPixel(int x, int y, const glm::vec4& c) {
		(*this.*setPixelFunc)(x, y, c);
	}
	
	// TODO: rework this function to use std::function and std::bind
	glm::vec4 getPixel(int x, int y) const {
		return ((*this.*getPixelFunc)(x, y));
	}

	eBitmapType getType() { return m_Type; }

private:
	void initGetSetFuncs() {
		if (m_Format == eBitmapFormat_UnsignedByte) {
			setPixelFunc = &Bitmap::setPixelUnsignedByte;
			getPixelFunc = &Bitmap::getPixelUnsignedByte;
		} 

		if (m_Format == eBitmapFormat_Float) {
			setPixelFunc = &Bitmap::setPixelFloat;
			getPixelFunc = &Bitmap::getPixelFloat;
		}
	}

	void setPixelFloat(int x, int y, const glm::vec4& c) const {
		const int offset = m_Comp * (y * m_Width + x);

		float* data = reinterpret_cast<float*>(m_Data.data());

		if (m_Comp > 0) data[offset + 0] = c.x;
		if (m_Comp > 1) data[offset + 1] = c.y;
		if (m_Comp > 2) data[offset + 2] = c.z;
		if (m_Comp > 3) data[offset + 3] = c.w;
	}

	glm::vec4 getPixelFloat(int x, int y) const {
		const int offset = m_Comp * (y * m_Width + x);

		return glm::vec4(
			m_Comp > 0 ? m_Data[offset + 0] : 0.0f,
			m_Comp > 1 ? m_Data[offset + 1] : 0.0f,
			m_Comp > 2 ? m_Data[offset + 2] : 0.0f,
			m_Comp > 3 ? m_Data[offset + 3] : 0.0f
		);
	}

	void setPixelUnsignedByte(int x, int y, const glm::vec4& c) {
		const int offset = m_Comp * (y * m_Width + x);

		if (m_Comp > 0) m_Data[offset + 0] = uint8_t(c.x * 255.0f);
		if (m_Comp > 1) m_Data[offset + 1] = uint8_t(c.y * 255.0f);
		if (m_Comp > 2) m_Data[offset + 2] = uint8_t(c.z * 255.0f);
		if (m_Comp > 3) m_Data[offset + 3] = uint8_t(c.w * 255.0f);
	}

	glm::vec4 getPixelUnsignedByte(int x, int y) const {
		const int offset = m_Comp * (y * m_Width + x);

		return glm::vec4(
			m_Comp > 0 ? float(m_Data[offset + 0]) / 255.0f : 0.0f,
			m_Comp > 1 ? float(m_Data[offset + 1]) / 255.0f : 0.0f,
			m_Comp > 2 ? float(m_Data[offset + 2]) / 255.0f : 0.0f,
			m_Comp > 3 ? float(m_Data[offset + 3]) / 255.0f : 0.0f
		);
	}

private:
	int m_Width = 0;
	int m_Height = 0;
	int m_Depth = 1;
	int m_Comp = 3;		// components per pixel

	eBitMapFormat m_Format = eBitMapFormat_UnsignedByte;
	eBitMapType m_Type = eBitMapType_2D;

	std::vector<uint8_t> m_Data;

	using setPixel_t = void(Bitmap::*)(int, int, const glm::vec4&);
	using getPixel_t = glm::vec4(Bitmap::*)(int, int) const;
	setPixel_t setPixelFunc = &Bitmap::setPixelUnsignedByte;
	getPixel_t getPixelFunc = &Bitmap::getPixelUnsignedByte;
};