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

#include "Bezier.hpp"
#include "Arc.hpp"
#include "Line.hpp"
#include "QuadTree.hpp"
#include "Image.hpp"
#include "ThreadPool.hpp"

namespace PA
{
	template <typename TPrimitive>
	inline auto DrawToGSSurface(QuadTree<TPrimitive>& primitives, RawCPUImage& img, ThreadPool<>& threadPool) -> V;

	template <typename TF>
	inline auto RasterizeToGSSurface(const QuadraticBezier<TF, 2>& curve, RawCPUImage& img) -> V;
	template<typename TF>
	inline auto RasterizeToGSSurfaceUnsafe(Span<const QuadraticBezier<TF, 2>> curves, RawCPUImage& img, ThreadPool<>& threadPool) -> V;

	struct Fragment
	{
		U32 idx;
		F32 value;

		Fragment() {}
		Fragment(U32 i, F32 v) : idx(i), value(v) {}
	};

	template <typename TF>
	inline auto RasterizeToFragments
	(
		const QuadraticBezier<TF, 2>& curve,
		Array<Fragment>& fragments,
		U32 width,
		U32 height,
		TF color = TF(1),
		TF curveWidth = TF(1)
	) -> V;

	template<typename TF>
	inline auto RasterizeToFragments
	(
		Span<const QuadraticBezier<TF, 2>> curves,
		Span<const TF> widths,
		Span<const TF> pigments,
		Array<Array<Fragment>>& fragMap,
		U32 width,
		U32 height,
		ThreadPool<>& threadPool
	) -> V;

	inline auto AddFragmentsOnHDRSurface(Array<Fragment>& fragments, RawCPUImage& surface) -> V;
	inline auto SubtractFragmentsFromHDRSurface(Array<Fragment>& fragments, RawCPUImage& surface) -> V;
	inline auto AddFragmentsOnHDRSurface(Array<Array<Fragment>>& fragMap, RawCPUImage& surface) -> V;
	inline auto SubtractFragmentsFromHDRSurface(Array<Array<Fragment>>& fragMap, RawCPUImage& surface) -> V;

	inline auto CopyHDRSurfaceToGSSurface(RawCPUImage& hdr, RawCPUImage& sdr) -> V;
	inline auto CopyHDRSurfaceToGSSurface(RawCPUImage& hdr, RawCPUImage& sdr, Span<const Fragment> fragments) -> V;
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
				if (x >= img.width || y >= img.height)
				{
					continue;
				}

				auto i = LebesgueCurve(x, y);
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


	template<typename TF>
	auto RasterizeToFragments(const QuadraticBezier<TF, 2>& curve, Array<Fragment>& fragments, U32 width, U32 height, TF color, TF curveWidth) -> V
	{
		const auto halfCurveWidth = curveWidth / TF(2);
		TF splitCutoff = 4;
		static constexpr TF valThreshold = TF(0.0001);
		fragments.clear();
		auto screenCurve = curve;
		ToSurfaceCoordinates(Span<Vector<TF, 2>>(screenCurve.points), width, height);

		StaticArray<QuadraticBezier<TF, 2>, 64> stack;
		auto stackSize = 0u;
		stack[stackSize++] = screenCurve;

		Map<U32, F32> rasterizedFragments;

		while (stackSize)
		{
			stackSize--;
			auto current = stack[stackSize];

			auto roughApproxLength = (current[0] - current[1]).Length() + (current[2] - current[1]).Length();

			if (roughApproxLength <= splitCutoff)
			{
				auto bBox = current.GetBBox();
				auto xMin = Min(U32(Max(TF(0), Floor(bBox.lower[0] - halfCurveWidth))), width - 1);
				auto xMax = Min(U32(Max(TF(0), Ceil(bBox.upper[0] + halfCurveWidth))), width - 1);
				auto yMin = Min(U32(Max(TF(0), Floor(bBox.lower[1] - halfCurveWidth))), height - 1);
				auto yMax = Min(U32(Max(TF(0), Ceil(bBox.upper[1]) + halfCurveWidth)), height - 1);
				for (auto i = yMin; i <= yMax; ++i)
				{
					for (auto j = xMin; j <= xMax; ++j)
					{
						auto idx = LebesgueCurve(j, i);
						auto pixelCenter = Vector<TF, 2>(j, i) + TF(0.5);

						// Since the curve is very tiny we can approximate the distance by few samples
						// instead of solving the cubic equation.
						auto sample0 = current.EvaluateAt(TF(0.5));
						auto dist0 = Distance(pixelCenter, sample0);
						auto dist1 = Distance(pixelCenter, current.p0);
						auto dist2 = Distance(pixelCenter, current.p2);

						// auto dist = current.GetDistanceFrom(pixelCenter) - halfCurveWidth;
						auto dist = Min(dist0, Min(dist1, dist2)) - halfCurveWidth;
						auto val = color * Max(TF(0), TF(1) - SmoothStep(TF(0), TF(0.75), dist));
						if (val > valThreshold)
						{
							auto oldVal = rasterizedFragments.find(idx);
							if (oldVal != rasterizedFragments.end())
							{
								oldVal->second = Max(oldVal->second, val);
							}
							else
							{
								rasterizedFragments[idx] = val;
							}
						}
					}
				}
			}
			else
			{
				auto split = current.Split(TF(0.5));
				stack[stackSize++] = split.second;
				stack[stackSize++] = split.first;
			}
		}

		fragments.reserve(rasterizedFragments.size());
		for (auto& frag : rasterizedFragments)
		{
			fragments.emplace_back(frag.first, frag.second);
		}
	}


	template<typename TF>
	auto RasterizeToFragments
	(
		Span<const QuadraticBezier<TF, 2>> curves,
		Span<const TF> widths,
		Span<const TF> pigments,
		Array<Array<Fragment>>& fragMap,
		U32 width,
		U32 height,
		ThreadPool<>& threadPool
	) -> V
	{
		auto taskCount = GetLogicalCPUCount();
		auto curvesPerTask = curves.size() / taskCount;

		auto task =
			[&](U32 start, U32 end)
			{
				for (auto i = start; i < end; ++i)
				{
					RasterizeToFragments(curves[i], fragMap[i], width, height, pigments[i], widths[i]);
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


	inline auto AddFragmentsOnHDRSurface(Array<Fragment>& fragments, RawCPUImage& surface) -> V
	{
		auto sPtr = (F32*)surface.data.data();
		for (const auto& frag : fragments)
		{
			sPtr[frag.idx] += frag.value;
		}
	}


	inline auto SubtractFragmentsFromHDRSurface(Array<Fragment>& fragments, RawCPUImage& surface) -> V
	{
		auto sPtr = (F32*)surface.data.data();
		for (const auto& frag : fragments)
		{
			sPtr[frag.idx] -= frag.value;
		}
	}


	inline auto AddFragmentsOnHDRSurface(Array<Array<Fragment>>& fragMap, RawCPUImage& surface) -> V
	{
		for (auto& fragments : fragMap)
		{
			AddFragmentsOnHDRSurface(fragments, surface);
		}
	}


	inline auto SubtractFragmentsFromHDRSurface(Array<Array<Fragment>>& fragMap, RawCPUImage& surface) -> V
	{
		for (auto& fragments : fragMap)
		{
			SubtractFragmentsFromHDRSurface(fragments, surface);
		}
	}


	inline auto CopyHDRSurfaceToGSSurface(RawCPUImage& hdr, RawCPUImage& sdr) -> V
	{
		auto extentSize = sdr.data.size();
		PA_ASSERT(hdr.width == sdr.width && sdr.height == hdr.height);
		PA_ASSERT(hdr.format == EFormat::A32Float);
		PA_ASSERT(sdr.format == EFormat::A8);
		PA_ASSERT(hdr.lebesgueOrdered && sdr.lebesgueOrdered);

		auto hdrPtr = (F32*)hdr.data.data();
		auto sdrPtr = (U8*)sdr.data.data();

		for (auto i = 0u; i < extentSize; ++i)
		{
			sdrPtr[i] = ClampedU8(hdrPtr[i] * 255);
		}
	}


	inline auto CopyHDRSurfaceToGSSurface(RawCPUImage& hdr, RawCPUImage& sdr, Span<const Fragment> fragments) -> V
	{
		PA_ASSERT(hdr.format == EFormat::A32Float);
		PA_ASSERT(sdr.format == EFormat::A8);

		auto hdrPtr = (F32*)hdr.data.data();
		auto sdrPtr = (U8*)sdr.data.data();

		for (const auto& frag : fragments)
		{
			sdrPtr[frag.idx] = ClampedU8(hdrPtr[frag.idx] * 255);
		}
	}
}