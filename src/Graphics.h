#pragma once

namespace Engine::Graphics {

	struct Viewport {
		float top_left_x = 0;
		float top_left_y = 0;
		float width = 0;
		float height = 0;
		float min_depth = 0;
		float max_depth = 1;
	};

	struct Rect {
		int left = 0;
		int top = 0;
		int right = 0;
		int bottom = 0;
	};
}