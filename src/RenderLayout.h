#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>

#include "CustomPipelineLayout.h"

namespace Engine {
	class RenderLayout {
	public:
		RenderLayout();
		~RenderLayout();

		void AddGraphicsPipelineLayout(GraphicsPipelineLayout graphicsPipelineLayout);
		GraphicsPipelineLayout* GetGraphicsPipelineLayout(size_t index);

		inline std::vector<GraphicsPipelineLayout>& GetGraphicsPipelineLayout() { return m_GraphicsPipelineLayouts; };
	
	private:
		std::vector<GraphicsPipelineLayout> m_GraphicsPipelineLayouts;
	};
}
