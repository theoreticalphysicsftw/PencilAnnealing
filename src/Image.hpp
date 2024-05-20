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

#include "Types.hpp"
#include "Algebra.hpp"
#include "ThreadPool.hpp"
#include "Color.hpp"


namespace PA
{
	enum class EFormat
	{
		A8 = 0,
		A32Float,
		RGBA8,
		RGBA32,
		RGBA32Float,
		Invalid
	};

	inline auto GetSize(EFormat format);

	struct Extent
	{
		U32 x;
		U32 y;
		U32 z;
		U32 w;
		U32 h;
		U32 d;
	};

	struct RawCPUImage
	{
		B lebesgueOrdered;
		EFormat format;
		U32 height;
		U32 width;
		Array<Byte> data;
		U32 lebesgueStride;

		RawCPUImage(U32 width = 0, U32 height = 0, EFormat format = EFormat::Invalid, B lebesgueOrdered = false);

		template <typename T>
		auto ToSurfaceCoordinates(const Vector<T, 2>& in) const -> Vector<T, 2>;
		template <typename T>
		auto ToSurfaceCoordinates(Span<Vector<T, 2>> in) const -> V;
		template <typename T>
		auto ToNormalizedCoordinates(const Vector<T, 2>& inSurface) const->Vector<T, 2>;
		template <typename T>
		auto ToNormalizedCoordinates(Span<Vector<T, 2>> in) const->V;

		template <typename T>
		auto Clear(T clearValue = T(0)) -> V;

		template <typename T>
		auto Clear(T clearValue, ThreadPool<>& threadPool) -> V;
	};

	template <typename TF>
	inline auto ToSurfaceCoordinates(const Vector<TF, 2>& in, U32 width, U32 height) -> Vector<TF, 2>;
	template <typename TF>
	inline auto ToSurfaceCoordinates(Span<Vector<TF, 2>> in, U32 width, U32 height) -> V;

	inline auto AdditiveBlendA8(const RawCPUImage& img0, const RawCPUImage& img1, F32 img0Contribution) -> RawCPUImage;

	inline auto A32FloatToRGBA8Linear(const RawCPUImage & img)->RawCPUImage;
}


namespace PA
{
	template<typename TF>
	auto ToSurfaceCoordinates(const Vector<TF, 2>& in, U32 width, U32 height) -> Vector<TF, 2>
	{
		Vector<TF, 2> result;
		result[0] = in[0] * (width - 1);
		result[1] = (TF(1) - in[1]) * (height - 1);
		return result;
	}

	template<typename TF>
	auto ToSurfaceCoordinates(Span<Vector<TF, 2>> in, U32 width, U32 height) -> V
	{
		for (auto& vec : in)
		{
			vec = ToSurfaceCoordinates(vec, width, height);
		}
	}

	inline auto GetSize(EFormat format)
	{
		static constexpr U32 sizeTable[] =
		{
			1,
			4,
			4,
			16,
			16,
			0
		};

		return sizeTable[U32(format)];
	}

	inline auto AdditiveBlendA8(const RawCPUImage& img0, const RawCPUImage& img1, F32 img0Contribution) -> RawCPUImage
	{
		PA_ASSERT(img0.width == img0.width && img1.height == img1.height);
		PA_ASSERT(img0.lebesgueOrdered && img1.lebesgueOrdered);
		PA_ASSERT(img0.format == EFormat::A8 && img1.format == EFormat::A8);

		RawCPUImage result(img0.width, img0.height, img0.format, img0.lebesgueOrdered);

		auto maxExtent = img0.lebesgueStride * img0.lebesgueStride;

		for (auto i = 0; i < maxExtent; ++i)
		{
			result.data[i] = ClampedU8(img0Contribution * img0.data[i] + (1.f - img0Contribution) * img1.data[i]);
		}

		return result;
	}


	inline auto A32FloatToRGBA8Linear(const RawCPUImage& img) -> RawCPUImage
	{
		PA_ASSERT(img.format == EFormat::A32Float);
		PA_ASSERT(img.lebesgueOrdered == true);
		RawCPUImage result(img.width, img.height, EFormat::RGBA8, false);

		auto inPtr = (const F32*)img.data.data();
		auto outPtr = (ColorU32*)result.data.data();

		for (auto i = 0u; i < img.height; ++i)
		{
			for (auto j = 0; j < img.width; ++j)
			{
				auto lebesgueIdx = LebesgueCurve(j, i);
				auto color = ClampedU8(255.f * (inPtr[lebesgueIdx]));
				outPtr[i * img.width + j] = ColorU32(color, color, color, 255u);
			}
		}
		return result;
	}


	inline RawCPUImage::RawCPUImage(U32 width, U32 height, EFormat format, B lebesgueOrdered) :
		width(width), height(height), format(format), lebesgueOrdered(lebesgueOrdered)
	{
		if (lebesgueOrdered)
		{
			auto maxDim = Max(width, height);
			lebesgueStride = RoundToPowerOfTwo(maxDim);
			data.resize(lebesgueStride * lebesgueStride * GetSize(format));
		}
		else
		{
			data.resize(height * width * GetSize(format));
		}

	}


	template<typename TF>
	inline auto RawCPUImage::ToSurfaceCoordinates(const Vector<TF, 2>& in) const -> Vector<TF, 2>
	{
		return ::ToSurfaceCoordinates(in, width, height);
	}


	template<typename TF>
	inline auto RawCPUImage::ToNormalizedCoordinates(const Vector<TF, 2>& inSurface) const -> Vector<TF, 2>
	{
		Vector<TF, 2> result;
		result[0] = inSurface[0] / (width - 1);
		result[1] = TF(1) - inSurface[1] / (height - 1);
		return result;
	}

	template<typename T>
	inline auto RawCPUImage::ToNormalizedCoordinates(Span<Vector<T, 2>> in) const -> V
	{
		for (auto& vec : in)
		{
			vec = ToNormalizedCoordinates(vec);
		}
	}


	template<typename T>
	inline auto RawCPUImage::ToSurfaceCoordinates(Span<Vector<T, 2>> in) const -> V
	{
		::ToSurfaceCoordinates(in, width, height);
	}


	template<typename T>
	inline auto RawCPUImage::Clear(T clearValue) -> V
	{
		auto maxExtent = lebesgueOrdered? lebesgueStride * lebesgueStride : width * height;
		auto tPtr = (T*)data.data();

		for (auto i = 0u; i < maxExtent; ++i)
		{
			tPtr[i] = clearValue;
		}
	}

	template<typename T>
	inline auto RawCPUImage::Clear(T clearValue, ThreadPool<>& threadPool) -> V
	{
		auto maxExtent = lebesgueOrdered ? lebesgueStride * lebesgueStride : width * height;
		auto tPtr = (T*)data.data();
		auto taskCount = GetLogicalCPUCount();
		auto pixelsPerTask = maxExtent / taskCount;

		auto task =
			[&](U32 start, U32 end)
			{
				for (auto i = start; i < end; ++i)
				{
					tPtr[i] = clearValue;
				}
			};

		Array<TaskResult<V>> results;
		for (auto i = 0u; i < taskCount; ++i)
		{
			results.emplace_back(threadPool.AddTask(task, i * pixelsPerTask, (i + 1) * pixelsPerTask));
		}

		// Remainder
		task(pixelsPerTask * taskCount, maxExtent);

		for (auto& result : results)
		{
			result.Retrieve();
		}
	}

}