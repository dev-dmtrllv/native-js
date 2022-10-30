#include "framework.hpp"
#include "gfx/RenderTree.hpp"

namespace NativeJS::GFX
{
	RenderTree::RenderTree() :
		root_(*this)
	{

	}

	RenderTree::~RenderTree()
	{

	}

	Node& RenderTree::root()
	{
		return root_;
	}
}