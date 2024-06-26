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

namespace PA
{
	template <typename TF, U32 Dim>
	struct QuadraticBezier
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
				Vec p2;
			};
			StaticArray<Vec, 3> points;
		};
		
		QuadraticBezier() {} 
		QuadraticBezier(const Vec& p0, const Vec& p1, const Vec& p2) : p0(p0), p1(p1), p2(p2) {}

		auto GetPolynomialCoefficients() const -> StaticArray<Vec, 3>;
		auto GetBBox() const -> BBox;
		auto EvaluateAt(Scalar t) const -> Vec;
		auto GetCentroid() const -> Vec;
		auto GetSquaredDistanceFrom(const Vec& p) const -> TF;
		auto GetDistanceFrom(const Vec& p) const -> TF;
        auto Split(Scalar t) const -> Pair<QuadraticBezier, QuadraticBezier>;

        auto operator[](U32 idx) -> Vec&;
        auto operator[](U32 idx) const -> const Vec&;
	};

	template <typename TF>
	auto GetRandom2DQuadraticBezierInRange(TF MaxSpan, TF range0 = TF(0), TF range1 = TF(1)) -> QuadraticBezier<TF, 2>;

	template <typename TF>
	auto GetBezierPassingThrough(const Vector<TF, 2>& p0, const Vector<TF, 2>& p1, const Vector<TF, 2>& p2) -> QuadraticBezier<TF, 2>;
}


namespace PA
{
	template<typename TF, U32 Dim>
    auto QuadraticBezier<TF, Dim>::operator[](U32 idx) -> Vec&
    {
        return points[idx];
    }


	template<typename TF, U32 Dim>
    auto QuadraticBezier<TF, Dim>::operator[](U32 idx) const -> const Vec&
    {
        return points[idx];
    }


	template<typename TF, U32 Dim>
	inline auto QuadraticBezier<TF, Dim>::GetPolynomialCoefficients() const -> StaticArray<Vec, 3>
	{
		StaticArray<Vec, 3> result =
		{
			p0 - p1 * Scalar(2) + p2,
			(p1 - p0) * Scalar(2),
			p0
		};
		return result;
	}


	template<typename TF, U32 Dim>
	inline auto QuadraticBezier<TF, Dim>::GetBBox() const -> BBox
	{
		return BBox(points);
	}


	template<typename TF, U32 Dim>
	inline auto QuadraticBezier<TF, Dim>::EvaluateAt(Scalar t) const -> Vec
	{
		return (Scalar(1) - t) * (Scalar(1) - t) * p0 + Scalar(2) * (Scalar(1) - t) * t * p1 + t * t * p2;
	}


	template<typename TF, U32 Dim>
	inline auto QuadraticBezier<TF, Dim>::GetCentroid() const -> Vec
	{
		return (p0 + p1 + p2) / 3.f;
	}


	template<typename TF, U32 Dim>
	inline auto QuadraticBezier<TF, Dim>::GetSquaredDistanceFrom(const Vec& p) const -> TF
	{
		// Polinomial coefficients of the direction from the point
		auto coefficients = GetPolynomialCoefficients();
		coefficients[2] = coefficients[2] - p;

		// Now get the polynomial coefficients of the direction dotted with the derivative
		auto a = TF(2) * coefficients[0].Dot(coefficients[0]);
		auto b = TF(3) * coefficients[0].Dot(coefficients[1]);
		auto c = TF(2) * coefficients[0].Dot(coefficients[2]) + coefficients[1].Dot(coefficients[1]);
		auto d = coefficients[1].Dot(coefficients[2]);

		auto roots = SolveCubic(a, b, c, d);

		U32 testPointsCount = 0u;
		StaticArray<Vec, 5> testPoints;
		for (auto i = 0; i < 3; ++i)
		{
			if (roots[i] >= TF(0) && roots[i] <= TF(1))
			{
				testPoints[testPointsCount++] = EvaluateAt(roots[i]);
			}
		}
		testPoints[testPointsCount++] = p0;
		testPoints[testPointsCount++] = p2;

		auto minSqDist = SquaredDistance(testPoints[0], p);
		for (auto i = 1u; i < testPointsCount; ++i)
		{
			minSqDist = Min(minSqDist, SquaredDistance(testPoints[i], p));
		}

		return minSqDist;
	}


	template<typename TF, U32 Dim>
	inline auto QuadraticBezier<TF, Dim>::GetDistanceFrom(const Vec& p) const -> TF
	{
		return Sqrt(GetSquaredDistanceFrom(p));
	}

    template<typename TF, U32 Dim>
	inline auto QuadraticBezier<TF, Dim>::Split(Scalar t) const -> Pair<QuadraticBezier, QuadraticBezier>
	{
		Vec b[3][3];

		for (auto i = 0u; i < 3; ++i)
		{
			b[0][i] = points[i];
		}

		for (auto i = 1u; i < 3u; ++i)
		{
			for (auto j = 0u; j < (3 - i); ++j)
			{
				b[i][j] = (1 - t) * b[i - 1][j] + t * b[i - 1][j + 1];
			}
		}

		return MakePair(QuadraticBezier(b[0][0], b[1][0], b[2][0]), QuadraticBezier(b[2][0], b[1][1], b[0][2]));
	}

	template<typename TF>
	auto GetRandom2DQuadraticBezierInRange(TF maxSpan, TF range0, TF range1) -> QuadraticBezier<TF, 2>
	{
		auto directionAngle = GetUniformFloat<TF>() * Constants<TF>::C2Pi;
		auto initialPos = Vec2(GetUniformFloat<TF>(range0, range1), GetUniformFloat<TF>(range0, range1));
		auto midPointProp = GetUniformFloat<TF>(range0, range1);
		auto midPointOffset = GetUniformFloat<TF>(range0, range1);

		auto spanDirection = GetUniformFloat<TF>(TF(0), maxSpan);
		auto spanNormal = GetUniformFloat<TF>(TF(0), maxSpan);

		auto direction = Vec2(Cos(directionAngle), Sin(directionAngle));
		auto normal = Vec2(-direction[1], direction[0]);

		auto endPoint = initialPos + direction * spanDirection;
		auto midPoint = initialPos + midPointProp * endPoint + normal * midPointOffset * spanNormal;

		return QuadraticBezier<TF, 2>(initialPos, midPoint, endPoint);
	}


	template<typename TF>
	auto GetBezierPassingThrough(const Vector<TF, 2>& p0, const Vector<TF, 2>& p1, const Vector<TF, 2>& p2) -> QuadraticBezier<TF, 2>
	{
		auto cp = TF(2) * p1 - TF(0.5) * (p0 + p2);
		return QuadraticBezier<TF, 2>(p0, cp, p2);
	}
}
