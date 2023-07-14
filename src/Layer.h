#pragma once

namespace Engine {
	class Layer {
	public:
		virtual ~Layer() = default;

		virtual void OnAttach() {};
		virtual void OnDetach() {};

		virtual void OnUpdate(float time) {};
		virtual void OnUIRender() {};
	};
}
