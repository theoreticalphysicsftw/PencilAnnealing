#include <Error.hpp>
#include <Bezier.hpp>

using namespace PA;

auto Test0() -> V
{
	static constexpr U32 latticeSize = 10000;
	static constexpr F32 tolerance = 0.0001;

	QuadraticBezier<F32, 2> curve(Vec2(-1, -1), Vec2(0, 1), Vec2(1, -1));
	for (auto i = 0; i < latticeSize; ++i)
	{
		for (auto j = 0; j < latticeSize; ++j)
		{
			auto t = 1.f / latticeSize * j;
			auto angle = Constants<F32>::C2Pi / latticeSize * i;
			auto rotation = CreateRotation<F32>(angle);
			auto newCurve = curve;
			newCurve.p0 = rotation * newCurve.p0;
			newCurve.p1 = rotation * newCurve.p1;
			newCurve.p2 = rotation * newCurve.p2;
			auto coefficients = newCurve.GetPolynomialCoefficients();

			auto val0 = coefficients[0] * t * t + coefficients[1] * t + coefficients[2];
			auto val1 = newCurve.EvaluateAt(t);
			auto diff = Abs(val1 - val0);

			if (diff[0] > tolerance || diff[1] > tolerance)
			{
				LogError("difference = (", diff[0], ", ", diff[1], ")");
				Terminate();
			}
		}
	}
}

auto Test1() -> V
{
	static constexpr U32 latticeSize = 10000;
	static constexpr F32 tolerance = 5;

	QuadraticBezier<F32, 2> curve(Vec2(-1000, -1000), Vec2(0, 1000), Vec2(1000, -100));
	for (auto i = 0; i < latticeSize; ++i)
	{
		for (auto j = 0; j < latticeSize; ++j)
		{
			auto t = 1.f / latticeSize * j;
			auto angle = Constants<F32>::C2Pi / latticeSize * i;
			auto rotation = CreateRotation<F32>(angle);
			auto newCurve = curve;
			newCurve.p0 = rotation * newCurve.p0;
			newCurve.p1 = rotation * newCurve.p1;
			newCurve.p2 = rotation * newCurve.p2;

			auto p = newCurve.EvaluateAt(t);
			auto dist = newCurve.GetDistanceFrom(p);

			if (dist > tolerance)
			{
				LogError("distance = ", dist);
				Terminate();
			}
		}
	}
}

I32 main(I32 argc, const C** argv)
{
	Test1();
}