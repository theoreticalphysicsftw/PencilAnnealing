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
	struct QuadraticBezier
	{
		using Scalar = TF;
		using Vec = Vector<TF, Dim>;
		using BBox = BBox<TF, Dim>;

		Vec P0;
		Vec P1;
		Vec P2;

		QuadraticBezier(const Vec& P0, const Vec& P1, const Vec& P2) : P0(P0), P1(P1), P2(P2) {}

		auto GetPolynomialCoefficients() const -> StaticArray<Vec, 3>;
		auto GetBBox() const -> BBox;
		auto EvaluateAt(Scalar t) const -> Vec;
	};
}


namespace PA
{
	template<typename TF, U32 Dim>
	inline auto QuadraticBezier<TF, Dim>::GetPolynomialCoefficients() const -> StaticArray<Vec, 3>
	{
		StaticArray<Vec, 3> result =
		{
			P0 - P1 * Scalar(2) + P2,
			(P1 - P0) * Scalar(2),
			P0
		};
		return result;
	}


	template<typename TF, U32 Dim>
	inline auto QuadraticBezier<TF, Dim>::GetBBox() const -> BBox
	{
		return BBox({ &P0, &P2 + 1 });
	}


	template<typename TF, U32 Dim>
	inline auto QuadraticBezier<TF, Dim>::EvaluateAt(Scalar t) const -> Vec
	{
		return (Scalar(1) - t) * (Scalar(1) - t) * P0 + Scalar(2) (Scalar(1) - t) * t * P1 + t * t * P2;
	}
}