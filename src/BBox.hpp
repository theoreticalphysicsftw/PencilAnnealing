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

namespace PA
{
	template <typename TF, U32 Dim>
	struct BBox
	{
		using Scalar = TF;
		using Vec = Vector<TF, Dim>;
		using VecSpan = Span<const Vec>;

		static constexpr U32 dimension = Dim;

		Vec lower;
		Vec upper;

		template <typename TPrimitive>
		BBox(Span<const TPrimitive> primitives);
		BBox(VecSpan points);
		BBox(Vec lower, Vec upper) : lower(lower), upper(upper) {}

		auto Intersects(const BBox& other) const -> B;
		auto Contains(const Vec& p) const -> B;
		auto Contains(const BBox& other) const -> B;
		template <typename TPrimitive>
		auto Contains(const TPrimitive& prim) const -> B;
	};

}

namespace PA
{
	template<typename TF, U32 Dim>
	inline BBox<TF, Dim>::BBox(VecSpan points)
	{
		auto infinity = Limits<Scalar>::infinity();
		Vec minTmp(infinity, infinity);
		Vec maxTmp(-infinity, -infinity);
		for (auto& point : points)
		{
			minTmp = Min(minTmp, point);
			maxTmp = Max(maxTmp, point);
		}

		lower = minTmp;
		upper = maxTmp;
	}

	template<typename TF, U32 Dim>
	template<typename TPrimitive>
	inline BBox<TF, Dim>::BBox(Span<const TPrimitive> primitives)
	{
		auto infinity = Limits<Scalar>::infinity();
		Vec minTmp(infinity, infinity);
		Vec maxTmp(-infinity, -infinity);
		for (auto& primitive : primitives)
		{
			auto bBox = primitive.GetBBox();
			minTmp = Min(minTmp, bBox.lower);
			maxTmp = Max(maxTmp, bBox.upper);
		}

		lower = minTmp;
		upper = maxTmp;
	}


	template<typename TF, U32 Dim>
	template<typename TPrimitive>
	inline auto BBox<TF, Dim>::Contains(const TPrimitive& prim) const -> B
	{
		return Contains(prim.GetBBox());
	}


	template<typename TF, U32 Dim>
	inline auto BBox<TF, Dim>::Intersects(const BBox& other) const -> B
	{
		auto upperLeft = Vec(other.lower[0], other.upper[1]);
		auto lowerRight = Vec(other.upper[0], other.lower[1]);
		auto thisUpperLeft = Vec(lower[0], upper[1]);
		auto thisLowerRight = Vec(upper[0], lower[1]);

		if
		(
			Contains(upperLeft) ||
			Contains(lowerRight) || 
			Contains(other.lower) || 
			Contains(other.upper) ||
			other.Contains(thisUpperLeft) ||
			other.Contains(thisLowerRight) ||
			other.Contains(lower) ||
			other.Contains(upper)
		)
		{
			return true;
		}

		return false;
	}


	template<typename TF, U32 Dim>
	inline auto BBox<TF, Dim>::Contains(const Vec& p) const -> B
	{
		return p[0] >= lower[0] && p[0] <= upper[0] && p[1] >= lower[1] && p[1] <= upper[1];
	}


	template<typename TF, U32 Dim>
	inline auto BBox<TF, Dim>::Contains(const BBox& other) const -> B
	{
		return Contains(other.lower) && Contains(other.upper);
	}
}