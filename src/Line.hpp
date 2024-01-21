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
	template <typename TF, U32 Dim>
	struct Line
	{
		using Scalar = TF;
		using Vec = Vector<TF, Dim>;
		using BBox = BBox<TF, Dim>;

		static constexpr U32 dimension = Dim;

		union
		{
			struct
			{
				Vec p0;
				Vec p1;
			};
			StaticArray<Vec, 2> points;
		};

		auto operator==(const Line& other) -> B;

		Line(const Vec& p0 = Vec(), const Vec& p1 = Vec()) : p0(p0), p1(p1) {}

		auto GetPolynomialCoefficients() const -> StaticArray<Vec, 2>;
		auto GetBBox() const -> BBox;
		auto Intersects(const BBox& bBox) const -> B;
		auto EvaluateAt(Scalar t) const -> Vec;
		auto GetCentroid() const -> Vec;
		auto GetSquaredDistanceFrom(const Vec& p) const -> TF;
		auto GetDistanceFrom(const Vec& p) const -> TF;
	};
}


namespace PA
{
	template<typename TF, U32 Dim>
	inline auto Line<TF, Dim>::operator==(const Line& other) -> B
	{
		return points == other.points;
	}

	template<typename TF, U32 Dim>
	inline auto Line<TF, Dim>::GetPolynomialCoefficients() const -> StaticArray<Vec, 2>
	{
		StaticArray<Vec, 2> result =
		{
			p1 - p0,
			p0
		};
		return result;
	}


	template<typename TF, U32 Dim>
	inline auto Line<TF, Dim>::GetBBox() const -> BBox
	{
		return BBox(points);
	}


	template<typename TF, U32 Dim>
	inline auto Line<TF, Dim>::Intersects(const BBox& bBox) const -> B
	{
		return bBox.Contains(p0) || bBox.Contains(p1);
	}


	template<typename TF, U32 Dim>
	inline auto Line<TF, Dim>::EvaluateAt(Scalar t) const -> Vec
	{
		return (Scalar(1) - t) * p0 + t * p1;
	}


	template<typename TF, U32 Dim>
	inline auto Line<TF, Dim>::GetCentroid() const -> Vec
	{
		return (p0 + p1) / 2.f;
	}


	template<typename TF, U32 Dim>
	inline auto Line<TF, Dim>::GetSquaredDistanceFrom(const Vec& p) const -> TF
	{
		// Polinomial coefficients of the direction from the point
		auto coefficients = GetPolynomialCoefficients();
		coefficients[1] = coefficients[1] - p;

		auto t = -coefficients[1].Dot(coefficients[0]) / coefficients[0].Dot(coefficients[0]);

		if (t >= 0 && t <= 1)
		{
			return SquaredDistance(p, EvaluateAt(t));
		}
		else
		{
			auto dist0 = SquaredDistance(p, p0);
			auto dist1 = SquaredDistance(p, p1);
			return Min(dist0, dist1);
		}
	}


	template<typename TF, U32 Dim>
	inline auto Line<TF, Dim>::GetDistanceFrom(const Vec& p) const -> TF
	{
		return Sqrt(GetSquaredDistanceFrom(p));
	}
}