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
	template <typename TF>
	struct Arc
	{
		using Scalar = TF;
		using Vec = Vector<TF, 2>;
		using BBox = BBox<TF, 2>;
		static constexpr U32 dimension = 2;

		Vec center;
		Scalar radius;
		Scalar arcStart;
		Scalar arcEnd;

		Arc(const Vec& center, Scalar radius, Scalar arcStart, Scalar arcEnd);
		auto GetSignedDistanceFrom(const Vec& p) const-> Scalar;
		auto GetDistanceFrom(const Vec& p) const-> Scalar;
		auto EvaluateAt(Scalar t) const -> Vec;
		auto GetBBox() const -> BBox;
	};

}


namespace PA
{
	template<typename TF>
	inline Arc<TF>::Arc(const Vec& center, Scalar radius, Scalar arcStart, Scalar arcEnd) :
		center(center), radius(radius), arcStart(arcStart), arcEnd(arcEnd)
	{
	}


	template<typename TF>
	inline auto Arc<TF>::GetSignedDistanceFrom(const Vec& p) const -> Scalar
	{
		auto centeredP = p - center;
		auto angle = ArcTan2(centeredP[1], centeredP[0]);

		auto normStart = Frac(arcStart / Constants<TF>::C2Pi) * Constants<TF>::C2Pi;
		auto normEnd = Frac(arcEnd / Constants<TF>::C2Pi) * Constants<TF>::C2Pi;

		auto minAngle = Min(normStart, normEnd);
		auto maxAngle = Max(normStart, normEnd);

		minAngle = (minAngle < 0)? minAngle + Constants<TF>::C2Pi : minAngle;
		maxAngle = (maxAngle < 0)? maxAngle + Constants<TF>::C2Pi : maxAngle;
		angle = (angle < 0)? angle + Constants<TF>::C2Pi : angle;

		if (angle >= minAngle && angle <= maxAngle)
		{
			return radius - centeredP.Length();
		}
		else
		{
			auto dist0 = Distance(p, EvaluateAt(0));
			auto dist1 = Distance(p, EvaluateAt(1));
			return Min(dist0, dist1);
		}
	}


	template<typename TF>
	inline auto Arc<TF>::GetDistanceFrom(const Vec& p) const  -> Scalar
	{
		return Abs(GetSignedDistanceFrom(p));
	}


	template<typename TF>
	inline auto Arc<TF>::EvaluateAt(Scalar t) const -> Vec
	{
		auto angle = arcStart * (Scalar(1) - t) + arcEnd * t;
		return center + Vec(Cos(angle), Sin(angle)) * radius;
	}


	template<typename TF>
	inline auto Arc<TF>::GetBBox() const -> BBox
	{
		return BBox(StaticArray<Vec, 3>{ EvaluateAt(0), EvaluateAt(0.5), EvaluateAt(1) });
	}
}
