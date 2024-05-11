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

#include "Types.hpp"
#include "Algebra.hpp"
#include "ThreadPool.hpp"


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
		PA_ASSERT(img0.width == img0.height && img1.width == img1.height);
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
		auto maxDim = Max(height, width);
		auto maxExtent = lebesgueOrdered? maxDim * maxDim : width * height;
		auto tPtr = (T*)data.data();

		for (auto i = 0u; i < maxExtent; ++i)
		{
			tPtr[i] = clearValue;
		}
	}

	template<typename T>
	inline auto RawCPUImage::Clear(T clearValue, ThreadPool<>& threadPool) -> V
	{
		auto maxDim = Max(height, width);
		auto maxExtent = lebesgueOrdered ? maxDim * maxDim : width * height;
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