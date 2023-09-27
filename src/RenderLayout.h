#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>

#include "GraphicsPipelineLayout.h"

namespace Engine {
	class RenderLayout {
	public:
		RenderLayout();
		~RenderLayout();

		void AddGraphicsPipelineLayout(GraphicsPipelineLayout graphicsPipelineLayout);
		GraphicsPipelineLayout& GetGraphicsPipelineLayout(size_t index);
	
	private:
		std::vector<GraphicsPipelineLayout> m_GraphicsPipelineLayouts;
	};
}
