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
		return GetBBox().Intersects(bBox);
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