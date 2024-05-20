// Copyright 2024 Mihail Mladenov
//
// This file is part of PencilAnnealing.
//
// PencilAnnealing is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PencilAnnealing is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PencilAnnealing.  If not, see <http://www.gnu.org/licenses/>.


#pragma once

#include "Algebra.hpp"
#include "BBox.hpp"
#include "Algebra.hpp"
#include "Random.hpp"
#include "Algorithm.hpp"

namespace PA
{
	template <typename TPrimitive>
	struct QuadTree
	{
		using Scalar = typename TPrimitive::Scalar;
		using BBox = typename TPrimitive::BBox;
		using Vec = typename TPrimitive::Vec;
		
		struct Node
		{
			Array<TPrimitive> primitives;
			B isLeaf = false;
			StaticArray<Node*, 4> descendants = {};
		};

		static constexpr U32 dimension = TPrimitive::dimension;

		auto Build(const Array<TPrimitive>& primitives) -> V;
		auto Remove(const TPrimitive& prim) -> V;
		auto Add(const TPrimitive& prim) -> V;
		auto GetPrimitivesAround(const Vec& p) const -> Span<const TPrimitive>;
		auto GetRandomPrimitive() -> TPrimitive;
		auto Bounds(const TPrimitive& prim) -> B;
		auto GetSerializedPrimitives() -> Array<TPrimitive>;

	private:
		auto BuildRecursive(const Array<TPrimitive>& primitives, const BBox& bBox) -> Node*;
		auto GetDescendantBBoxes(const BBox& bBox) const -> StaticArray<BBox, 4>;

		Node* root = nullptr;
		BBox globalBBox = BBox(Vec(0), Vec(1));
		Array<Node*> leaves;
	};

}


namespace PA
{
	template<typename TPrimitive>
	inline auto QuadTree<TPrimitive>::Build(const Array<TPrimitive>& primitives) -> V
	{
		globalBBox = BBox(Span<const TPrimitive>(primitives.begin(), primitives.end()));
		root = BuildRecursive(primitives, globalBBox);
	}


	template<typename TPrimitive>
	inline auto QuadTree<TPrimitive>::Bounds(const TPrimitive& prim) -> B
	{
		return globalBBox.Contains(prim);
	}


	template<typename TPrimitive>
	inline auto QuadTree<TPrimitive>::GetSerializedPrimitives() -> Array<TPrimitive>
	{
		Array<TPrimitive> result;
		for (auto leafPtr : leaves)
		{
			if (!leafPtr->primitives.empty())
			{
				result.insert(result.end(), leafPtr->primitives.begin(), leafPtr->primitives.end());
			}
		}
		return result;
	}


	template<typename TPrimitive>
	inline auto QuadTree<TPrimitive>::Remove(const TPrimitive& prim) -> V
	{
		Array<Pair<Node*, BBox>> toVisit;
		toVisit.emplace_back(root, globalBBox);

		while (!toVisit.empty())
		{
			auto currentLevel = toVisit.back();
			toVisit.pop_back();

			if (currentLevel.first->isLeaf)
			{
				auto found = Find(currentLevel.first->primitives.begin(), currentLevel.first->primitives.end(), prim);
				if (found != currentLevel.first->primitives.end())
				{
					currentLevel.first->primitives.erase(found);
				}
			}
			else
			{
				auto descendantBBoxes = GetDescendantBBoxes(currentLevel.second);
				for (auto i = 0; i < 4; ++i)
				{
					if (currentLevel.first->descendants[i] && prim.Intersects(descendantBBoxes[i]))
					{
						toVisit.emplace_back(currentLevel.first->descendants[i], descendantBBoxes[i]);
					}
				}
			}
		}
	}


	template<typename TPrimitive>
	inline auto QuadTree<TPrimitive>::Add(const TPrimitive& prim) -> V
	{
		Array<Pair<Node*, BBox>> toVisit;
		toVisit.emplace_back(root, globalBBox);

		while (!toVisit.empty())
		{
			auto currentLevel = toVisit.back();
			toVisit.pop_back();

			if (currentLevel.first->isLeaf)
			{
				currentLevel.first->primitives.push_back(prim);
			}
			else
			{
				auto descendantBBoxes = GetDescendantBBoxes(currentLevel.second);
				for (auto i = 0; i < 4; ++i)
				{
					if (prim.Intersects(descendantBBoxes[i]))
					{
						if (!currentLevel.first->descendants[i])
						{
							auto newNode = new Node;
							newNode->isLeaf = true;
							leaves.push_back(newNode);
							currentLevel.first->descendants[i] = newNode;
						}
						toVisit.emplace_back(currentLevel.first->descendants[i], descendantBBoxes[i]);
					}
				}
			}
		}
	}


	template<typename TPrimitive>
	inline auto QuadTree<TPrimitive>::GetPrimitivesAround(const Vec& p) const -> Span<const TPrimitive>
	{
		Span<const TPrimitive> result;

		Array<Pair<Node*, BBox>> toVisit;
		toVisit.emplace_back(root, globalBBox);

		while (!toVisit.empty())
		{
			auto currentLevel = toVisit.back();
			toVisit.pop_back();

			if (currentLevel.first->isLeaf)
			{
				if (currentLevel.second.Contains(p))
				{
					result = currentLevel.first->primitives;
					return result;
				}
			}
			else
			{
				auto descendantBBoxes = GetDescendantBBoxes(currentLevel.second);
				for (auto i = 0; i < 4; ++i)
				{
					if (currentLevel.first->descendants[i] && descendantBBoxes[i].Contains(p))
					{
						toVisit.emplace_back(currentLevel.first->descendants[i], descendantBBoxes[i]);
						break;
					}
				}
			}
		}

		return result;
	}


	template<typename TPrimitive>
	inline auto QuadTree<TPrimitive>::GetRandomPrimitive() -> TPrimitive
	{
		auto currentNode = root;
		B found = false;
		TPrimitive result;

		while (!found)
		{
			auto randomIdx = GetUniformU32(0, leaves.size() - 1);
			auto leaf = leaves[randomIdx];
			if (!leaf->primitives.empty())
			{
				auto idx = GetUniformU32(0, leaf->primitives.size() - 1);
				found = true;
				result = leaf->primitives[idx];
			}
		}

		return result;
	}


	template<typename TPrimitive>
	inline auto QuadTree<TPrimitive>::BuildRecursive(const Array<TPrimitive>& primitives, const BBox& bBox) -> Node*
	{
		auto newNode = new Node;

		if (primitives.empty())
		{
			newNode->isLeaf = true;
			leaves.push_back(newNode);
			return newNode;
		}

		auto descendantBBoxes = GetDescendantBBoxes(bBox);

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


		if (splits[0].size() + splits[1].size() + splits[2].size() + splits[3].size() >= 2 * primitives.size())
		{
			// Partitions start to become worse.
			newNode->isLeaf = true;
			newNode->primitives.insert(newNode->primitives.end(), primitives.begin(), primitives.end());
			leaves.push_back(newNode);
			return newNode;
		}


		for (auto q = 0; q < 4; ++q)
		{
			newNode->descendants[q] = BuildRecursive(splits[q], descendantBBoxes[q]);
		}

		return newNode;
	}


	template<typename TPrimitive>
	inline auto QuadTree<TPrimitive>::GetDescendantBBoxes(const BBox& bBox) const -> StaticArray<BBox, 4>
	{
		StaticArray<BBox, 4> descendantBBoxes =
		{
			BBox(Vec2(bBox.lower[0], (bBox.lower[1] + bBox.upper[1]) / Scalar(2)), Vec2((bBox.lower[0] + bBox.upper[0]) / Scalar(2), bBox.upper[1])),
			BBox((bBox.lower + bBox.upper) / Scalar(2), bBox.upper),
			BBox(bBox.lower, (bBox.lower + bBox.upper) / Scalar(2)),
			BBox(Vec2((bBox.lower[0] + bBox.upper[0]) / Scalar(2), bBox.lower[1]), Vec2(bBox.upper[0], (bBox.lower[1] + bBox.upper[1]) / Scalar(2)))
		};

		return descendantBBoxes;
	}
}
