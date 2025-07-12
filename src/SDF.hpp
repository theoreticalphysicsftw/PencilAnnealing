// Copyright 2025 Mihail Mladenov
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

#include "Vector.hpp"

namespace PA::SDF
{
	template <typename TF>
	auto Box2D(const Vector<TF, 2>& p, const Vector<TF, 2>& boxSize) -> TF
	{
		auto d = Abs(p) - boxSize;
		return Max(d, Vector<TF, 2>(TF(0))).Length() + Min(TF(0), Max(d[0], d[1]));
	}

	template <typename TF>
	auto Round(TF dist, TF radius) -> TF
	{
		return dist - radius;
	}
}