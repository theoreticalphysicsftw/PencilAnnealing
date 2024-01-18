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

#include "Bezier.hpp"
#include "Arc.hpp"
#include "Line.hpp"
#include "Color.hpp"
#include "Utilities.hpp"
#include "Random.hpp"

namespace PA
{
	template <typename TF>
	class Annealer
	{
	public:
		using Scalar = TF;
		using Vec = Vector<Scalar, 2>;
		using QuadraticBezier = QuadraticBezier<Scalar, 2>;
		using Arc = Arc<Scalar>;
		using Line = Line<Scalar, 2>;

		RawCPUImage grayscaleReference;
		RawCPUImage currentApproximation;
		RawCPUImage workingApproximation;

		Annealer(const RawCPUImage* referance, U32 maxStrokes = 0, U32 maxSteps = 0);
		auto CopyCurrentApproximationToColor(ColorU32* data, U32 stride) -> V;
		auto AnnealBezier() -> V;
		auto AnnealArc() -> V;
		auto AnnealLine() -> V;

	private:
		auto ToScreenSpaceCoordinates(Vec in) -> Vec;
		auto ToScreenSpaceCoordinates(Span<Vec> in) -> V;

		auto DrawBezierToSurface(const QuadraticBezier& b, RawCPUImage& img) -> Pair<U32, U32>;
		auto DrawBezierToSurface(const Array<QuadraticBezier>& b, RawCPUImage& img) -> Pair<U32, U32>;
		auto DrawArcToSurface(const Arc& a, RawCPUImage& img) -> Pair<U32, U32>;
		auto DrawArcsToSurface(const Array<Arc>& a, RawCPUImage& img) -> Pair<U32, U32>;
		auto DrawLineToSurface(const Line& a, RawCPUImage& img) -> Pair<U32, U32>;
		auto DrawLinesToSurface(const Array<Line>& a, RawCPUImage& img) -> Pair<U32, U32>;
		auto ClearSurface(RawCPUImage& img);
		auto RestoreSurfaceFrom(RawCPUImage& toRestore, const RawCPUImage& reference, Pair<U32, U32> extent);

		auto GetEnergy(const RawCPUImage& img0, Pair<U32, U32> extent) -> TF;
		auto GetEnergy(const RawCPUImage& img0) -> TF;

		auto InitBezier() -> V;
		auto InitArc() -> V;
		auto InitLine() -> V;

		Array<QuadraticBezier> strokes;
		Array<Arc> arcStrokes;
		Array<Line> lineStrokes;
		
		U32 maxStrokes;
		U32 maxSteps;

		Scalar temperature = TF(1);
		Scalar optimalEnergy;
		U32 step = 0;
	};
}


namespace PA
{
	template<typename TF>
	inline Annealer<TF>::Annealer(const RawCPUImage* reference, U32 maxStrokes, U32 maxSteps) :
		grayscaleReference(reference->width, reference->height, EFormat::A8, true),
		currentApproximation(reference->width, reference->height, EFormat::A8, true),
		workingApproximation(reference->width, reference->height, EFormat::A8, true)
	{
		for (auto i = 0u; i < reference->width; ++i)
		{
			for (auto j = 0u; j < reference->height; ++j)
			{
				auto idx = LebesgueCurve(j, i);
				auto inColor = ((ColorU32*)reference->data.data())[i * reference->width + j];
				auto grayscaleColor = RGBAToGrayscale(inColor);
				this->grayscaleReference.data[idx] = grayscaleColor;
			}
		}

		for (auto i = 0u; i < currentApproximation.data.size(); ++i)
		{
			currentApproximation.data[i] = 255u;
			workingApproximation.data[i] = 255u;
		}

		this->maxStrokes = maxStrokes ? maxStrokes : 1 << 5;
		this->maxSteps = maxSteps ? maxSteps : 1u << 10;

		optimalEnergy = GetEnergy(currentApproximation);

		//InitBezier();
		//InitArc();
		InitLine();
	}


	template<typename TF>
	inline auto Annealer<TF>::InitBezier() -> V
	{
		for (auto i = 0u; i < maxStrokes; ++i)
		{
			auto strokeLength = SmoothStep(TF(1) / grayscaleReference.width, TF(0.5), TF(0.5) * (1 - TF(i + 1) / maxStrokes));
			auto directionAngle = GetUniformFloat01<TF>() * Constants<TF>::C2Pi;
			auto initialPos = Vec2(GetUniformFloat01<TF>(), GetUniformFloat01<TF>());
			auto midPointProp = GetUniformFloat01<TF>();
			auto midPointOffset = GetUniformFloat01<TF>();

			auto direction = Vec2(Cos(directionAngle), Sin(directionAngle));
			auto normal = Vec2(-direction[1], direction[0]);

			auto endPoint = initialPos + direction * strokeLength;
			auto midPoint = initialPos + midPointProp * endPoint + normal * midPointOffset * strokeLength;

			strokes.emplace_back(initialPos, midPoint, endPoint);
		}
	}

	template<typename TF>
	inline auto Annealer<TF>::InitArc() -> V
	{
		for (auto i = 0u; i < maxStrokes; ++i)
		{
			auto strokeLength = SmoothStep(TF(1) / grayscaleReference.width, TF(0.5), TF(0.5) * (1 - TF(i + 1) / maxStrokes));
			auto arcStart = GetUniformFloat01<TF>() * Constants<TF>::C2Pi;
			auto arcEnd = GetUniformFloat01<TF>() * Constants<TF>::C2Pi;
			auto center = Vec2(GetUniformFloat01<TF>(), GetUniformFloat01<TF>());
			auto radius = GetUniformFloat01<TF>() * strokeLength;
			arcStrokes.emplace_back(center, radius, arcStart, arcEnd);
		}
	}


	template<typename TF>
	inline auto Annealer<TF>::InitLine() -> V
	{
		for (auto i = 0u; i < maxStrokes; ++i)
		{
			auto strokeLength = SmoothStep(TF(1) / grayscaleReference.width, TF(0.1), TF(0.1) * (1 - TF(i + 1) / maxStrokes));
			auto p0 = Vec2(GetUniformFloat01<TF>(), GetUniformFloat01<TF>());
			auto angle = GetUniformFloat01<TF>() * Constants<TF>::C2Pi;
			auto direction = Vec2(Cos(angle), Sin(angle)) * strokeLength;
			PA_ASSERT(direction.Length() <= 1);
			auto p1 = p0 + direction;
			lineStrokes.emplace_back(p0, p1);
		}
	}


	template<typename TF>
	inline auto Annealer<TF>::ClearSurface(RawCPUImage& img)
	{
		for (auto i = 0u; i < img.data.size(); ++i)
		{
			img.data[i] = 255u;
		}
	}


	template<typename TF>
	inline auto Annealer<TF>::CopyCurrentApproximationToColor(ColorU32* data, U32 stride) -> V
	{
		for (auto i = 0u; i < this->currentApproximation.width; ++i)
		{
			for (auto j = 0u; j < this->currentApproximation.height; ++j)
			{
				auto idx = LebesgueCurve(j, i);
				auto inColor = this->currentApproximation.data[idx];
				data[i * stride / 4 + j] = ColorU32(inColor, inColor, inColor, 255);
			}
		}
	}

	template<typename TF>
	inline auto Annealer<TF>::AnnealBezier() -> V
	{
		if (step >= maxSteps)
		{
			return;
		}

		temperature = TF(1) - TF(step) / maxSteps;

		auto pointPreturbation = Vec(GetUniformFloat01<TF>(), GetUniformFloat01<TF>());
		auto pointIdx = GetUniformU32(0, 2);
		auto strokeIdx = GetUniformU32(0, strokes.size() - 1);

		auto& targetPointRef = strokes[strokeIdx].points[pointIdx];
		targetPointRef = targetPointRef + pointPreturbation;

		ClearSurface(workingApproximation);
		DrawBezierToSurface(strokes, workingApproximation);
		auto currentEnergy = GetEnergy(workingApproximation);

		if (currentEnergy < optimalEnergy || Exp((optimalEnergy - currentEnergy) / temperature) >= GetUniformFloat01<TF>())
		{
			optimalEnergy = currentEnergy;
			currentApproximation = workingApproximation;
		}
		else
		{
			targetPointRef = targetPointRef - pointPreturbation;
		}
	}


	template<typename TF>
	inline auto Annealer<TF>::AnnealArc() -> V
	{
		if (step >= maxSteps)
		{
			return;
		}

		temperature = TF(1) - TF(step) / maxSteps;

		
		auto strokeIdx = GetUniformU32(0, arcStrokes.size() - 1);
		auto propertyIdx = GetUniformU32(0, 4);
		auto preturbation = GetUniformFloat01<TF>();

		auto propertyPtr = (TF*) &arcStrokes[strokeIdx];
		propertyPtr[propertyIdx] += preturbation;

		ClearSurface(workingApproximation);
		DrawArcsToSurface(arcStrokes, workingApproximation);
		auto currentEnergy = GetEnergy(workingApproximation);

		if (currentEnergy < optimalEnergy || Exp((optimalEnergy - currentEnergy) / temperature) >= GetUniformFloat01<TF>())
		{
			optimalEnergy = currentEnergy;
			currentApproximation = workingApproximation;
		}
		else
		{
			propertyPtr[propertyIdx] -= preturbation;
		}
	}

	template<typename TF>
	inline auto Annealer<TF>::AnnealLine() -> V
	{
		if (step >= maxSteps)
		{
			return;
		}

		temperature = TF(1) - TF(step) / maxSteps;

		auto strokeIdx = GetUniformU32(0, lineStrokes.size() - 1);
		auto preturbation = GetUniformFloat01<TF>() * temperature;
		auto preturbationAngle = GetUniformFloat01<TF>() * Constants<TF>::C2Pi;

		auto& line = lineStrokes[strokeIdx];
		auto centroid = line.GetCentroid();
		line.p0 = line.p0 - centroid;
		line.p1 = line.p1 - centroid;
		auto rotation = CreateRotation<TF>(preturbationAngle);
		auto invRotation = CreateRotation<TF>(-preturbationAngle);
		line.p0 = rotation * line.p0;
		line.p1 = rotation * line.p1;
		line.p0 = line.p0 + centroid + preturbation;
		line.p1 = line.p1 + centroid + preturbation;

		ClearSurface(workingApproximation);
		DrawLinesToSurface(lineStrokes, workingApproximation);
		auto currentEnergy = GetEnergy(workingApproximation);

		if (currentEnergy < optimalEnergy || Exp((optimalEnergy - currentEnergy) / temperature) >= GetUniformFloat01<TF>())
		{
			optimalEnergy = currentEnergy;
			currentApproximation = workingApproximation;
		}
		else
		{
			line.p0 = line.p0 - centroid - preturbation;
			line.p1 = line.p1 - centroid - preturbation;
			line.p0 = invRotation * line.p0;
			line.p1 = invRotation * line.p1;
			line.p0 = line.p0 + centroid;
			line.p1 = line.p1 + centroid;
		}
	}


	template<typename TF>
	inline auto Annealer<TF>::ToScreenSpaceCoordinates(Vec in) -> Vec
	{
		Vec result;
		result[0] = in[0] * (grayscaleReference.width - 1);
		result[1] = (TF(1) - in[1]) * (grayscaleReference.height - 1);
		return result;
	}


	template<typename TF>
	inline auto Annealer<TF>::ToScreenSpaceCoordinates(Span<Vec> in) -> V
	{
		for (auto& vec : in)
		{
			vec = ToScreenSpaceCoordinates(vec);
		}
	}


	template<typename TF>
	inline auto Annealer<TF>::DrawBezierToSurface(const QuadraticBezier& curve, RawCPUImage& img) -> Pair<U32, U32>
	{
		auto bBox = curve.GetBBox();
		auto lowerLeft = ToScreenSpaceCoordinates(bBox.lower);
		auto upperRight = ToScreenSpaceCoordinates(bBox.upper);
		auto lowerRight = Vec(upperRight[0], lowerLeft[1]);
		auto upperLeft = Vec(lowerLeft[0], upperRight[1]);
		auto imgSize = img.width * img.height;
		auto minIdx = Min(LebesgueCurve(upperLeft[0], upperLeft[1]), imgSize - 1);
		auto maxIdx = Min(LebesgueCurve(lowerRight[0], lowerRight[1]), imgSize - 1);

		auto screenCurve = curve;
		ToScreenSpaceCoordinates(screenCurve.points);

		for (auto i = minIdx; i <= maxIdx; ++i)
		{
			auto coords = LebesgueCurveInverse(i);
			auto dist = screenCurve.GetDistanceFrom(Vec(coords.first, coords.second));
			auto val = SmoothStep(TF(0), TF(1), dist / 1.f);
			img.data[i] = ClampedU8(img.data[i] - (TF(255) - TF(255) * val));
		}

		return Pair<U32, U32>(minIdx, maxIdx);
	}


	template<typename TF>
	inline auto Annealer<TF>::DrawBezierToSurface(const Array<QuadraticBezier>& curves, RawCPUImage& img) -> Pair<U32, U32>
	{
		auto maxExtent = Pair<U32, U32>(0, 0);
		for (auto& curve : curves)
		{
			auto extent = DrawBezierToSurface(curve, img);
			maxExtent.first = Max(maxExtent.first, extent.first);
			maxExtent.second = Max(maxExtent.second, extent.second);
		}
		return maxExtent;
	}


	template<typename TF>
	inline auto Annealer<TF>::DrawArcToSurface(const Arc& a, RawCPUImage& img) -> Pair<U32, U32>
	{
		auto bBox = a.GetBBox();
		auto lowerLeft = ToScreenSpaceCoordinates(bBox.lower);
		auto upperRight = ToScreenSpaceCoordinates(bBox.upper);
		auto lowerRight = Vec(upperRight[0], lowerLeft[1]);
		auto upperLeft = Vec(lowerLeft[0], upperRight[1]);
		auto imgSize = img.width * img.height;
		auto minIdx = Min(LebesgueCurve(upperLeft[0], upperLeft[1]), imgSize - 1);
		auto maxIdx = Min(LebesgueCurve(lowerRight[0], lowerRight[1]), imgSize - 1);

		auto screenArc = a;
		screenArc.center = ToScreenSpaceCoordinates(screenArc.center);
		screenArc.radius *= img.width;

		for (auto i = minIdx; i <= maxIdx; ++i)
		{
			auto coords = LebesgueCurveInverse(i);
			auto dist = screenArc.GetDistanceFrom(Vec(coords.first, coords.second));
			auto val = SmoothStep(TF(0), TF(1), dist / 1.f);
			img.data[i] = ClampedU8(img.data[i] - (TF(255) - TF(255) * val));
		}

		return Pair<U32, U32>(minIdx, maxIdx);
	}


	template<typename TF>
	inline auto Annealer<TF>::DrawArcsToSurface(const Array<Arc>& arcs, RawCPUImage& img) -> Pair<U32, U32>
	{
		auto maxExtent = Pair<U32, U32>(0, 0);
		for (auto& arc : arcs)
		{
			auto extent = DrawArcToSurface(arc, img);
			maxExtent.first = Max(maxExtent.first, extent.first);
			maxExtent.second = Max(maxExtent.second, extent.second);
		}
		return maxExtent;
	}


	template<typename TF>
	inline auto Annealer<TF>::DrawLineToSurface(const Line& l, RawCPUImage& img) -> Pair<U32, U32>
	{
		auto bBox = l.GetBBox();
		auto lowerLeft = ToScreenSpaceCoordinates(bBox.lower);
		auto upperRight = ToScreenSpaceCoordinates(bBox.upper);
		auto lowerRight = Vec(upperRight[0], lowerLeft[1]);
		auto upperLeft = Vec(lowerLeft[0], upperRight[1]);
		auto imgSize = img.width * img.height;
		auto minIdx = Min(LebesgueCurve(upperLeft[0], upperLeft[1]), imgSize - 1);
		auto maxIdx = Min(LebesgueCurve(lowerRight[0], lowerRight[1]), imgSize - 1);

		auto screenLine = l;
		ToScreenSpaceCoordinates(screenLine.points);

		for (auto i = minIdx; i <= maxIdx; ++i)
		{
			auto coords = LebesgueCurveInverse(i);
			auto dist = screenLine.GetDistanceFrom(Vec(coords.first, coords.second));
			auto val = SmoothStep(TF(0), TF(1), dist / 1.f);
			img.data[i] = ClampedU8(img.data[i] - (TF(255) - TF(255) * val));
		}

		return Pair<U32, U32>(minIdx, maxIdx);
	}
	

	template<typename TF>
	inline auto PA::Annealer<TF>::DrawLinesToSurface(const Array<Line>& lines, RawCPUImage& img) -> Pair<U32, U32>
	{
		/*
		auto maxExtent = Pair<U32, U32>(0, 0);
		for (auto& line : lines)
		{
			auto extent = DrawLineToSurface(line, img);
			maxExtent.first = Max(maxExtent.first, extent.first);
			maxExtent.second = Max(maxExtent.second, extent.second);
		}
		return maxExtent;
		*/

		auto imgSize = img.width * img.height;
		for (auto i = 0; i < imgSize; ++i)
		{
			for (auto& l : lines)
			{
				auto screenLine = l;
				ToScreenSpaceCoordinates(screenLine.points);
				auto coords = LebesgueCurveInverse(i);
				auto dist = screenLine.GetDistanceFrom(Vec(coords.first, coords.second));
				auto val = SmoothStep(TF(0), TF(1), dist / 1.f);
				img.data[i] = ClampedU8(img.data[i] - (TF(255) - TF(255) * val));
			}
		}

		return Pair<U32, U32>(0, imgSize);
	}


	template<typename TF>
	inline auto Annealer<TF>::RestoreSurfaceFrom(RawCPUImage& toRestore, const RawCPUImage& reference, Pair<U32, U32> extent)
	{
		for (auto i = extent.first; i < extent.second; ++i)
		{
			toRestore.data[i] = reference.data[i];
		}
	}


	template<typename TF>
	inline auto Annealer<TF>::GetEnergy(const RawCPUImage& img, Pair<U32, U32> extent) -> TF
	{
		TF energy = 0;
		for (auto i = extent.first; i < extent.second; ++i)
		{
			auto diff = grayscaleReference.data[i] - img.data[i];
			energy += diff * diff;
		}
		return energy;
	}


	template<typename TF>
	inline auto Annealer<TF>::GetEnergy(const RawCPUImage& img) -> TF
	{
		TF energy = 0;
		auto imgSize = img.height * img.width;
		for (auto i = 0u; i < imgSize; ++i)
		{
			auto diff = grayscaleReference.data[i] - img.data[i];
			energy += diff * diff;
		}
		return energy;
	}
}