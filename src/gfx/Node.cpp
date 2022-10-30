#include "framework.hpp"
#include "gfx/Node.hpp"
#include "gfx/RenderTree.hpp"

namespace NativeJS::GFX
{
	Node::Node(RenderTree& renderTree, Node* parent, const std::vector<Node>& children) :
		renderTree_(renderTree),
		parent_(parent),
		children_(children)
	{

	}
}