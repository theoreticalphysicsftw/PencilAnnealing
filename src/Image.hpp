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

		RawCPUImage(U32 width = 0, U32 height = 0, EFormat format = EFormat::Invalid, B lebesgueOrdered = false);

		template <typename T>
		auto ToSurfaceCoordinates(const Vector<T, 2>& in) const -> Vector<T, 2>;
		template <typename T>
		auto ToNormalizedCoordinates(const Vector<T, 2>& inSurface) const -> Vector<T, 2>;
		template <typename T>
		auto ToSurfaceCoordinates(Span<Vector<T, 2>> in) const -> V;
	};

	template <typename TF>
	inline auto ToSurfaceCoordinates(const Vector<TF, 2>& in, U32 width, U32 height) -> Vector<TF, 2>;
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


	inline RawCPUImage::RawCPUImage(U32 width, U32 height, EFormat format, B lebesgueOrdered) :
		width(width), height(height), format(format), lebesgueOrdered(lebesgueOrdered)
	{
		data.resize(height * width * GetSize(format));
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
	inline auto RawCPUImage::ToSurfaceCoordinates(Span<Vector<T, 2>> in) const -> V
	{
		for (auto& vec : in)
		{
			vec = RawCPUImage::ToSurfaceCoordinates(vec);
		}
	}

}