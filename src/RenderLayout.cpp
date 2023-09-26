#include "RenderLayout.h"

namespace Engine {
	RenderLayout::RenderLayout() {

	}

	RenderLayout::~RenderLayout() {

	}

	void RenderLayout::AddGraphicsPipelineLayout(GraphicsPipelineLayout graphicsPipelineLayout) {
		m_GraphicsPipelineLayouts.push_back(graphicsPipelineLayout);
	}
}