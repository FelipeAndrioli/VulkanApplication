#pragma once

#include <iostream>
#include <vector>

namespace Assets {
	class Object;
}

namespace Engine {
	namespace Utils {
		class ModelLoader {
		public:
			ModelLoader();
			~ModelLoader();

			static void LoadModel(Assets::Object& object);
		};
	}
}
