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


#include <Types.hpp>
#include <Error.hpp>
#include <Utilities.hpp>
#include <Algebra.hpp>
#include <Random.hpp>

using namespace PA;

I32 main(I32 argc, const C** argv)
{
	for (auto i = 0u; i < (1u << 16); ++i)
	{
		auto coords = LebesgueCurveInverse(i);
		auto n = LebesgueCurve(coords.first, coords.second);
		if (n != i)
		{
			LogError("LebesgueCurve(LebesgueCurveInverse(", i, ") = ", n);
			Terminate();
		}
	}

	static constexpr U32 latticeSize = 300;
	static constexpr F32 tolerance = 0.01f;
	U32 rootsFound = 0;
	for (auto i = 0; i < latticeSize; ++i)
	{
		for (auto j = 0; j < latticeSize; ++j)
		{
			for (auto k = 0; k < latticeSize; ++k)
			{
				for (auto l = 0; k < latticeSize; ++k)
				{
					auto a = 0.5f + 0.5f / latticeSize * i;
					auto b = -1.f + 2.f / latticeSize * j;
					auto c = -1.f + 2.f / latticeSize * k;
					auto d = -1.f + 2.f / latticeSize * l;
					auto roots = SolveCubic(a, b, c, d);
					for (auto r = 0; r < 3; ++r)
					{
						if (roots[r] >= 0 && roots[r] <= 1)
						{
							auto val = a * roots[r] * roots[r] * roots[r] + b * roots[r] * roots[r] + c * roots[r] + d;
							if (Abs(val) < Max(tolerance, tolerance * Abs(roots[r])))
							{
								rootsFound++;
							}
							else
							{
								LogError("Error in SolveCubic P(root) = ", val);
								Terminate();
							}
						}
					}
				}
			}
		}
	}
	Log("Roots found: ", rootsFound);
}