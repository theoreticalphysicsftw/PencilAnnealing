// MIT License
// 
// Copyright (c) 2024 Mihail Mladenov
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#pragma once

#include "Algebra.hpp"
#include "BBox.hpp"

namespace PA
{
	template <typename TPrimitive>
	struct QuadTree
	{
		using Scalar = TPrimitive::Scalar;
		using BBox = TPrimitive::BBox;
		using Vec = TPrimitive::Vec;
		
		struct Node
		{
			Array<TPrimitive> primitives;
			B isLeaf = false;
			StaticArray<Node*, 4> descendants = {};
		};

		static constexpr U32 dimension = TPrimitive::dimension;

		auto Build(Span<const Array<TPrimtive>> primitives) -> V;
		auto Remove(const TPrimitive& prim) -> V;
		auto Add(const TPrimitive& prim) -> V;

	private:
		auto BuildRecursive(Span<const Array<TPrimitives>> primitives, const BBox& bBox) -> Node*;

		Node* root = nullptr;
	};
}


namespace PA
{
	template<typename TPrimitive>
	inline auto QuadTree<TPrimitive>::Build(Span<const Array<TPrimtives>> primitives) -> V
	{
		auto globalBBox = BBox(primitives);
		root = BuildRecursive(primitives, globalBBox);
	}

	template<typename TPrimitive>
	inline auto QuadTree<TPrimitive>::BuildRecursive(Span<const Array<TPrimitive>> primitives, const BBox& bBox) -> Node*
	{
		auto newNode = new Node;

		if (primitives.empty())
		{
			return newNode;
		}

		StaticArray<BBox, 4> descendantBBoxes =
		{
			BBox(Vec2(bBox.lower[0], (bBox.lower[1] + bBox.upper[1]) / Scalar(2)), Vec2((bBox.lower[0] + bBox.upper[0]) / Scalar(2), bBox.upper[1])),
			BBox((bBox.lower + bBox.upper) / Scalar(2), bBox.upper),
			BBox(bBox.lower, (bBox.lower + bBox.upper) / Scalar(2)),
			BBox(Vec2((bBox.lower[0] + bBox.upper[0]) / Scalar(2), bBox.upper[1]), Vec2(bBox.upper[0], (bBox.lower[1] + bBox.upper[1]) / Scalar(2)))
		};

		StaticArray<Array<TPrimitive>, 4> splits;
		
		for (auto& primitive : primitives)
		{
			for (auto quadrant = 0u; quadrant < 4; ++quadrant)
			{
				if (primitive.GetBBox().Intersects(descendantBBoxes[quadrant]))
				{
					splits[quadrant].push_back(primitive);
				}
			}
		}

		if (splits[0].size() + splits[1].size() + splits[2].size() + splits[3].size() >= 4 * primitives.size())
		{
			// All primitives intersect all bounding boxes. At this point give up.
			newNode->isLeaf = true;
			newNode->primitives.append(primitives);
			return newNode;
		}

		for (auto q = 0; q < 4; ++q)
		{
			newNode->descendants[q] = BuildRecursive(splits[q], descendantBBoxes[q]);
		}

		return newNode;
	}
}