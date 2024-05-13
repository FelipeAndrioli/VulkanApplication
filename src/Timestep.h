#pragma once

namespace Engine {
	class Timestep {
	public:
		Timestep(float time = 0.0f) : m_Time(time) {};
		~Timestep() {};

		inline float GetSeconds() { return m_Time; };
		inline float GetMilliseconds() { return m_Time * 1000.0f; };
	private:
		float m_Time;
	};
}
