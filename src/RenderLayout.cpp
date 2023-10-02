#include "RenderLayout.h"

namespace Engine {
	RenderLayout::RenderLayout() {

	}

	RenderLayout::~RenderLayout() {

	}

	void RenderLayout::AddGraphicsPipelineLayout(GraphicsPipelineLayout graphicsPipelineLayout) {
		m_GraphicsPipelineLayouts.push_back(graphicsPipelineLayout);
	}

	GraphicsPipelineLayout* RenderLayout::GetGraphicsPipelineLayout(size_t index) {
		if (index > m_GraphicsPipelineLayouts.size()) {
			throw std::runtime_error("Index out of bounds!");
		}

		return &m_GraphicsPipelineLayouts[index];
	}
}