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
#include "ThreadPool.hpp"

namespace PA
{
	template <typename TPrimitive>
	inline auto DrawToGSSurface(QuadTree<TPrimitive>& primitives, RawCPUImage& img, ThreadPool<>& threadPool) -> V;

	template <typename TF>
	inline auto RasterizeToGSSurface(const QuadraticBezier<TF, 2>& curve, RawCPUImage& img) -> V;
	template<typename TF>
	inline auto RasterizeToGSSurfaceUnsafe(Span<const QuadraticBezier<TF, 2>> curves, RawCPUImage& img, ThreadPool<>& threadPool) -> V;
}

namespace PA
{
	template<typename TPrimitive>
	inline auto DrawToGSSurface(QuadTree<TPrimitive>& primitives, RawCPUImage& img, ThreadPool<>& threadPool) -> V
	{
		using Scalar = TPrimitive::Scalar;
		using Vec = typename TPrimitive::Vec;
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
				auto worldCoords = img.ToNormalizedCoordinates(floatCoords);
				auto nearPrimitives = primitives.GetPrimitivesAround(worldCoords);

				for (auto& p : nearPrimitives)
				{
					auto screenPrimitive = p;
					img.ToSurfaceCoordinates(screenPrimitive.points);

					auto dist = screenPrimitive.GetDistanceFrom(floatCoords);
					auto val = SmoothStep(Scalar(0), Scalar(1), dist);
					img.data[i] = ClampedU8(img.data[i] - (Scalar(255) - Scalar(255) * val));

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
	}

	template<typename TF>
	inline auto RasterizeToGSSurface(const QuadraticBezier<TF, 2>& curve, RawCPUImage& img) -> V
	{
		auto bBox = curve.GetBBox();
		auto lowerLeft = img.ToSurfaceCoordinates(bBox.lower);
		auto upperRight = img.ToSurfaceCoordinates(bBox.upper);
		auto lowerRight = Vector<TF, 2>(upperRight[0], lowerLeft[1]);
		auto upperLeft = Vector<TF, 2>(lowerLeft[0], upperRight[1]);
		auto imgSize = img.width * img.height;

		auto screenCurve = curve;
		img.ToSurfaceCoordinates(Span<Vector<TF, 2>>(screenCurve.points));

		Array<QuadraticBezier<TF, 2>> stack;
		stack.push_back(screenCurve);

		while (!stack.empty())
		{
			auto current = stack.back();
			stack.pop_back();

			auto roughApproxLength = (current[0] - current[1]).Length() + (current[2] - current[1]).Length();

			if (roughApproxLength <= TF(1))
			{
				auto centroid = current.GetCentroid();
				auto x = U16(centroid[0]);
				auto y = U16(centroid[1]);
				auto i = LebesgueCurve(x, y);
				if (i >= img.data.size())
				{
					continue;
				}

				auto pixelCenter = Vector<TF, 2>(x, y) + TF(0.5);
				auto dist = current.GetDistanceFrom(pixelCenter);
				auto val = SmoothStep(TF(0), TF(1), dist);
				img.data[i] = ClampedU8(img.data[i] - (TF(255) - TF(255) * val));
			}
			else
			{
				auto split = current.Split(TF(0.5));
				stack.push_back(split.first);
				stack.push_back(split.second);
			}
		}
	}

	template<typename TF>
	inline auto RasterizeToGSSurfaceUnsafe(Span<const QuadraticBezier<TF, 2>> curves, RawCPUImage& img, ThreadPool<>& threadPool) -> V
	{

		auto taskCount = GetLogicalCPUCount();
		auto curvesPerTask = curves.size() / taskCount;

		auto task =
			[&](U32 start, U32 end)
			{
				for (auto i = start; i < end; ++i)
				{
					RasterizeToGSSurface(curves[i], img);
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
}