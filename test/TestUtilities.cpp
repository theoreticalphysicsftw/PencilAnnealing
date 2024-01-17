#include <Types.hpp>
#include <Error.hpp>
#include <Utilities.hpp>
#include <Algebra.hpp>

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

	static constexpr U32 latticeSize = 400;
	static constexpr F32 tolerance = 0.001;
	U32 rootsFound = 0;
	for (auto i = 0; i < latticeSize; ++i)
	{
		for (auto j = 0; j < latticeSize; ++j)
		{
			for (auto k = 0; k < latticeSize; ++k)
			{
				for (auto l = 0; k < latticeSize; ++k)
				{
					auto a = -1.f + 2.f / latticeSize * i;
					auto b = -1.f + 2.f / latticeSize * j;
					auto c = -1.f + 2.f / latticeSize * k;
					auto d = -1.f + 2.f / latticeSize * l;
					auto roots = SolveCubic(a, b, c, d);
					for (auto r = 0; r < 3; ++r)
					{
						if (!IsNan(roots[r]))
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