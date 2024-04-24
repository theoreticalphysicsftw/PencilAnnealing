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
#include "QuadTree.hpp"
#include "Color.hpp"
#include "Utilities.hpp"
#include "Random.hpp"
#include "File.hpp"
#include "ThreadPool.hpp"
#include "Convolution.hpp"

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

		Annealer(const RawCPUImage* referance, U32 maxStrokes = 0, U32 maxSteps = 0);
		~Annealer();
		auto CopyCurrentApproximationToColor(ColorU32* data, U32 stride) -> V;
		auto AnnealBezier() -> B;
		auto AnnealArc() -> V;
		auto AnnealLine() -> B;

		auto ClearSurface(RawCPUImage& img) -> V;
		auto DrawLinesToSurface(const QuadTree<Line>& lines, RawCPUImage& img) -> Pair<U32, U32>;

		auto ShutDownThreadPool() -> V;

	private:
        static constexpr U32 logAfterSteps = 128;

		auto ToScreenSpaceCoordinates(Vec in) -> Vec;
		auto ToWorldCoordinates(Vec inScreen) -> Vec;
		auto ToScreenSpaceCoordinates(Span<Vec> in) -> V;

		auto DrawBezierToSurface(const QuadraticBezier& b, RawCPUImage& img) -> V;
		auto DrawBezierToSurface(const Array<QuadraticBezier>& b, RawCPUImage& img) -> V;
		auto DrawArcToSurface(const Arc& a, RawCPUImage& img) -> Pair<U32, U32>;
		auto DrawArcsToSurface(const Array<Arc>& a, RawCPUImage& img) -> Pair<U32, U32>;
		auto RestoreSurfaceFrom(RawCPUImage& toRestore, const RawCPUImage& reference, Pair<U32, U32> extent);

		auto GetEnergy(const RawCPUImage& img0, Pair<U32, U32> extent) -> TF;
		auto GetEnergy(const RawCPUImage& img0) -> TF;

		auto InitBezier() -> V;
		auto InitArc() -> V;
		auto InitLine() -> V;

		RawCPUImage grayscaleReference;
		RawCPUImage grayscaleBlurredReference;
		RawCPUImage grayscaleReferenceEdges;
		RawCPUImage currentApproximation;
		RawCPUImage workingApproximation;

		Array<QuadraticBezier> strokes;
		Array<Arc> arcStrokes;
		QuadTree<Line> lineStrokes;

		U32 maxStrokes;
		U32 maxSteps;

		Scalar temperature;
		Scalar maxTemperature;
		Scalar optimalEnergy;
		U32 step = 0;

		ThreadPool<> threadPool;
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

		grayscaleReferenceEdges = GradientMagnitude(threadPool, grayscaleReference);

		this->maxStrokes = maxStrokes ? maxStrokes : (reference->width * reference->height / 64);
		this->maxSteps = maxSteps ? maxSteps : (1u << 24);
		this->maxTemperature = 255 * 255;
		temperature = maxTemperature;

		optimalEnergy = GetEnergy(currentApproximation);

		InitBezier();
		//InitArc();
		InitLine();
	}

	template<typename TF>
	inline Annealer<TF>::~Annealer()
	{
		auto primitives = lineStrokes.GetSerializedPrimitives();
		Str svg = "<svg xmlns = \"http://www.w3.org/2000/svg\" width =\"";
		svg += ToString(currentApproximation.width);
		svg += "\" height=\"" + ToString(currentApproximation.height) + "\"";
		svg += " viewBox=\"0 0 " + ToString(currentApproximation.width) + " ";
		svg += ToString(currentApproximation.height) + "\">\n";
		svg += "<path stroke=\"#010101\" stroke-width=\"0.5\" d=\"";
		for (auto& primitive : primitives)
		{
			auto p0 = ToScreenSpaceCoordinates(primitive.p0);
			auto p1 = ToScreenSpaceCoordinates(primitive.p1);
			svg += "M " + ToString(p0[0]) + " " + ToString(p0[1]) + " ";
			svg += "L " + ToString(p1[0]) + " " + ToString(p1[1]) + " ";
		}
		svg += "\"/>\n";
		svg += "</svg>";
		WriteWholeFile("out.svg", { (const Byte*)svg.data(), svg.size() });
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
		Array<Line> randomLines;
		for (auto i = 0u; i < maxStrokes; ++i)
		{
			auto strokeLength = SmoothStep(TF(4) / grayscaleReference.width, TF(0.2), TF(0.2) * (1 - TF(i + 1) / maxStrokes));
			auto p0 = Vec2(GetUniformFloat01<TF>(), GetUniformFloat01<TF>());
			auto angle = GetUniformFloat01<TF>() * Constants<TF>::C2Pi;
			auto direction = Vec2(Cos(angle), Sin(angle)) * strokeLength;
			PA_ASSERT(direction.Length() <= 1);
			auto p1 = p0 + direction;
			randomLines.emplace_back(p0, p1);
		}
		lineStrokes.Build({randomLines.begin(), randomLines.end()});
		Log("QuadTree built.");
	}


	template<typename TF>
	inline auto Annealer<TF>::ClearSurface(RawCPUImage& img) -> V
	{
		auto imgSize = img.height * img.width;
		auto taskCount = GetLogicalCPUCount();
		auto pixelsPerTask = imgSize / taskCount;

		auto task =
		[&](U32 start, U32 end)
		{
			for (auto i = start; i < end; ++i)
			{
				img.data[i] = 255u;
			}
		};

		Array<TaskResult<V>> results;
		for (auto i = 0u; i < taskCount; ++i)
		{
			results.emplace_back(threadPool.AddTask(task, i * pixelsPerTask, (i + 1) * pixelsPerTask));
		}

		// Remainder
		task(pixelsPerTask * taskCount, imgSize);

		for (auto& result : results)
		{
			result.Retrieve();
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
	inline auto Annealer<TF>::AnnealBezier() -> B
	{
		if (step >= maxSteps)
		{
            Log("Annealing done.");
			return false;
		}

		temperature = temperature * TF(0.9999);

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

		if (!(step % logAfterSteps))
		{
			Log("Energy = ", optimalEnergy, " Temperature = ", temperature);
		}
        step++;
        return true;
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
	inline auto Annealer<TF>::AnnealLine() -> B
	{
		if (step >= maxSteps)
		{
			Log("Annealing done.");
			return false;
		}


		temperature = temperature * TF(0.999);

		auto line = lineStrokes.GetRandomPrimitive();
		auto oldLine = line;
		auto oldLength = Distance(oldLine.p0, oldLine.p1);
		B appropriatePreturbation = false;
		BBox field(Vec(0, 0), Vec(1, 1));
		while (!appropriatePreturbation)
		{
			auto newAngle = GetUniformFloat01<TF>() * Constants<TF>::C2Pi;
			auto newDirection = Vec(Sin(newAngle), Cos(newAngle)) * oldLength;
			auto newStart = Vec(GetUniformFloat01<TF>(), GetUniformFloat01<TF>());
			line.p0 = newStart;
			line.p1 = newStart + newDirection;

			if (field.Contains(line))
			{
				appropriatePreturbation = true;
			}
			else
			{
				line = oldLine;
			}
		}


		lineStrokes.Remove(oldLine);
		lineStrokes.Add(line);
		ClearSurface(workingApproximation);
		DrawLinesToSurface(lineStrokes, workingApproximation);
		auto currentEnergy = GetEnergy(workingApproximation);

		auto transitionThreshold = Exp((optimalEnergy - currentEnergy) / temperature);

		if (currentEnergy < optimalEnergy || transitionThreshold >= GetUniformFloat01<TF>())
		{
			optimalEnergy = currentEnergy;
			if (!(step % logAfterSteps) || step == maxSteps - 1)
			{
				currentApproximation = workingApproximation;
			}
		}
		else
		{
			lineStrokes.Remove(line);
			lineStrokes.Add(oldLine);
		}
		step++;


		if (!(step % logAfterSteps))
		{
			Log("Energy = ", optimalEnergy, " Temperature = ", temperature);
		}

		return true;
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
	inline auto Annealer<TF>::ToWorldCoordinates(Vec inScreen) -> Vec
	{
		Vec result;
		result[0] = inScreen[0] / (grayscaleReference.width - 1);
		result[1] = TF(1) - inScreen[1] / (grayscaleReference.height - 1);
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
	inline auto Annealer<TF>::DrawBezierToSurface(const QuadraticBezier& curve, RawCPUImage& img) -> V
	{
		auto bBox = curve.GetBBox();
		auto lowerLeft = ToScreenSpaceCoordinates(bBox.lower);
		auto upperRight = ToScreenSpaceCoordinates(bBox.upper);
		auto lowerRight = Vec(upperRight[0], lowerLeft[1]);
		auto upperLeft = Vec(lowerLeft[0], upperRight[1]);
		auto imgSize = img.width * img.height;

		auto screenCurve = curve;
		ToScreenSpaceCoordinates(screenCurve.points);

        Array<QuadraticBezier> stack;
        stack.push_back(screenCurve);

        while (!stack.empty())
        {
            auto current = stack.back();
            stack.pop_back();

            auto roughApproxLength = (current[0] - current[1]).Length() + (current[2] - current[1]).Length();

            if (roughApproxLength <= Scalar(1))
            {
                auto centroid = current.GetCentroid();
                auto x = U16(centroid[0]);
                auto y = U16(centroid[1]);
                auto i = LebesgueCurve(x, y);
                if (i >= img.data.size())
                {
                    continue;
                }

                auto pixelCenter = Vec(x, y) + Scalar(0.5);
                auto dist = current.GetDistanceFrom(pixelCenter);
                auto val = SmoothStep(TF(0), TF(1), dist);
                img.data[i] = ClampedU8(img.data[i] - (TF(255) - TF(255) * val));
            }
            else
            {
                auto split = current.Split(Scalar(0.5));
                stack.push_back(split.first);
                stack.push_back(split.second);
            }
        }
	}


	template<typename TF>
	inline auto Annealer<TF>::DrawBezierToSurface(const Array<QuadraticBezier>& curves, RawCPUImage& img) -> V
	{

		auto taskCount = GetLogicalCPUCount();
		auto curvesPerTask = curves.size() / taskCount;

		auto task =
		[&](U32 start, U32 end)
		{
			for (auto i = start; i < end; ++i)
			{
                DrawBezierToSurface(curves[i], img);
			}
		};

		Array<TaskResult<V>> results;
		for (auto i = 0u; i < taskCount; ++i)
		{
			results.emplace_back(threadPool.AddTask(task, i * curvesPerTask, (i + 1) * curvesPerTask));
		}

		// Remainder
		task(taskCount * curvesPerTask, curves.size());

		for (auto& result : results)
		{
			result.Retrieve();
		}
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
	inline auto PA::Annealer<TF>::DrawLinesToSurface(const QuadTree<Line>& lines, RawCPUImage& img) -> Pair<U32, U32>
	{
		auto imgSize = img.width * img.height;

		auto taskCount = GetLogicalCPUCount();
		auto pixelsPerTask = imgSize / taskCount;

		auto task =
		[&](U32 start, U32 end)
		{
			for (auto i = start; i < end; ++i)
			{
				auto coords = LebesgueCurveInverse(i);
				auto floatCoords = Vec(coords.first + 0.5, coords.second + 0.5);
				auto worldCoords = ToWorldCoordinates(floatCoords);
				auto nearLines = lines.GetPrimitivesAround(worldCoords);

				for (auto& l : nearLines)
				{
					auto screenLine = l;
					ToScreenSpaceCoordinates(screenLine.points);

					auto dist = screenLine.GetDistanceFrom(floatCoords);
					auto val = SmoothStep(TF(0), TF(1), dist);
					img.data[i] = ClampedU8(img.data[i] - (TF(255) - TF(255) * val));

					if (img.data[i] == 0)
					{
						break;
					}
				}
			}
		};

		Array<TaskResult<V>> results;
		for (auto i = 0u; i < taskCount; ++i)
		{
			results.emplace_back(threadPool.AddTask(task, i * pixelsPerTask, (i + 1) * pixelsPerTask));
		}

		// Remainder
		task(taskCount * pixelsPerTask, imgSize);

		for (auto& result : results)
		{
			result.Retrieve();
		}

		return Pair<U32, U32>(0, imgSize);
	}

	template<typename TF>
	inline auto Annealer<TF>::ShutDownThreadPool() -> V
	{
		threadPool.ShutDown();
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
		auto extentSize = extent.second - extent.first;
		auto taskCount = GetLogicalCPUCount();
		auto pixelsPerTask = extentSize / taskCount;

		auto task =
		[&](U32 start, U32 end) -> TF
		{
			TF energy = 0;
			for (auto i = start; i < end; ++i)
			{
				auto diff = TF(grayscaleReference.data[i]) - TF(img.data[i]);
				energy += 0.2 * (diff * diff) / extentSize;
			}

			for (auto i = start; i < end; ++i)
			{
				auto diff = TF(grayscaleReferenceEdges.data[i]) - TF(img.data[i]);
				energy += 0.8 * (diff * diff) / extentSize;
			}
			return energy;
		};

		Array<TaskResult<TF>> results;
		for (auto i = 0u; i < taskCount; ++i)
		{
			results.emplace_back(threadPool.AddTask(task, extent.first + i * pixelsPerTask, extent.first + (i + 1) * pixelsPerTask));
		}

		// Remainder
		task(extent.first + pixelsPerTask * taskCount, extent.second);

		TF energy = 0;
		for (auto& result : results)
		{
			energy += result.Retrieve();
		}

		return energy;
	}


	template<typename TF>
	inline auto Annealer<TF>::GetEnergy(const RawCPUImage& img) -> TF
	{
		auto imgSize = img.width * img.height;
		return GetEnergy(img, Pair<U32, U32>(0u, imgSize));
	}
}
