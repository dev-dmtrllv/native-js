#pragma once

#include "gfx/Node.hpp"

namespace NativeJS
{
	namespace GFX
	{
		class RenderTree
		{
		public:
			RenderTree();
			~RenderTree();

			Node& root();

		private:
			Node root_;
		};
	}
}