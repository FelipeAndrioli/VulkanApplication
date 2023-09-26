#pragma once

#include <iostream>
#include <vector>

#include "GraphicsPipelineLayout.h"

namespace Engine {
	class RenderLayout {
	public:
		RenderLayout();
		~RenderLayout();

		void AddGraphicsPipelineLayout(GraphicsPipelineLayout graphicsPipelineLayout);
	
	private:
		std::vector<GraphicsPipelineLayout> m_GraphicsPipelineLayouts;
	};
}
