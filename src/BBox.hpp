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
}