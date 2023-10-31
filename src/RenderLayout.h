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

		inline std::vector<GraphicsPipelineLayout>& GetGraphicsPipelineLayouts() { return m_GraphicsPipelineLayouts; };
		inline std::vector<ComputePipelineLayout>& GetComputePipelineLayouts() { return m_ComputePipelineLayouts; };

	private:
		std::vector<GraphicsPipelineLayout> m_GraphicsPipelineLayouts;
		std::vector<ComputePipelineLayout> m_ComputePipelineLayouts;
	};
}
