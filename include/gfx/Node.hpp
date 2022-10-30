#pragma once

#include "framework.hpp"

namespace NativeJS
{
	namespace GFX
	{
		class RenderTree;

		class Node
		{
			public:
				Node(RenderTree& renderTree, Node* parent = nullptr, const std::vector<Node>& children = {});

			private:
				RenderTree& renderTree_;

				Node* parent_;
				std::vector<Node> children_;

			friend class RenderTree;
		};
	}
}