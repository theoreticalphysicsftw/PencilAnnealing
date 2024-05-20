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
#include "Utilities.hpp"
#include "Algebra.hpp"

namespace PA
{
	using Color2 = Vec2;
	using Color3 = Vec3;
	using Color4 = Vec4;

	union ColorU32
	{
		U32 packed;
		struct
		{
			U8 r;
			U8 g;
			U8 b;
			U8 a;
		};
		struct
		{
			U8 y;
			U8 cb;
			U8 cr;
			U8 a0;
		};
		struct
		{
			U8 y0;
			U8 u;
			U8 v;
			U8 a1;
		};

		ColorU32(U32 raw = 0);
		ColorU32(U8 r, U8 g, U8 b, U8 a);
		ColorU32(const Color4& c4);

		operator Color4() const;
	};

	inline auto YCbCrAToRGBA(ColorU32) -> ColorU32;
	inline auto RGBAToYCbCrA(ColorU32) -> ColorU32;
	inline auto RGBAToYCbCrABT601(ColorU32) -> ColorU32;
	inline auto YUVAToRGBA(ColorU32) -> ColorU32;
	inline auto RGBAToYUVA(ColorU32) -> ColorU32;
	inline auto RGBAToGrayscale(ColorU32) -> U8;
}


namespace PA
{
	inline ColorU32::ColorU32(U32 raw)
		: packed(raw)
	{
	}

	inline ColorU32::ColorU32(U8 r, U8 g, U8 b, U8 a)
		: r(r), g(g), b(b), a(a)
	{
	}

	inline ColorU32::ColorU32(const Color4& c4)
	{
		r = U8(c4[0] * 255);
		g = U8(c4[1] * 255);
		b = U8(c4[2] * 255);
		a = U8(c4[3] * 255);
	}

	inline ColorU32::operator Color4() const
	{
		return Color4(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
	}

	inline auto YCbCrAToRGBA(ColorU32 ycbcra)-> ColorU32
	{
		ColorU32 rgba;
		rgba.r = ClampedU8(ycbcra.y + 1.402f * (ycbcra.cr - 128.f));
		rgba.g = ClampedU8(ycbcra.y - 0.34414f * (ycbcra.cb - 128.f) - 0.71414f * (ycbcra.cr - 128.f));
		rgba.b = ClampedU8(ycbcra.y + 1.772f * (ycbcra.cb - 128.f));
		rgba.a = ycbcra.a;
		return rgba;
	}

	inline auto RGBAToYCbCrA(ColorU32 rgba) -> ColorU32
	{
		ColorU32 ycbcra;
		ycbcra.y = ClampedU8(0.299f * rgba.r + 0.587f * rgba.g + 0.114f * rgba.b);
		ycbcra.cr = ClampedU8(128.f - 0.1687f * rgba.r - 0.3313f * rgba.g + 0.5f * rgba.b);
		ycbcra.cb = ClampedU8(128.f + 0.5f * rgba.r - 0.4187f * rgba.g - 0.0813f * rgba.b);
		ycbcra.a = rgba.a;
		return ycbcra;
	}

	inline auto RGBAToYCbCrABT601(ColorU32 rgba) -> ColorU32
	{
		ColorU32 ycbcra;
		ycbcra.y = U8(Clamp(16.f + 0.2567f * rgba.r + 0.5041f * rgba.g + 0.0980f * rgba.b, 16.f, 235.f));
		ycbcra.cr = U8(Clamp(128.f - 0.1482f * rgba.r - 0.2909f * rgba.g + 0.4392f * rgba.b, 16.f, 240.f));
		ycbcra.cb = U8(Clamp(128.f + 0.4392f * rgba.r - 0.3677f * rgba.g - 0.0714f * rgba.b, 16.f, 240.f));
		ycbcra.a = rgba.a;
		return ycbcra;
	}

	auto YUVAToRGBA(ColorU32 yuva) -> ColorU32
	{
		ColorU32 rgba;
		rgba.r = ClampedU8(yuva.y + 1.1398f * yuva.v);
		rgba.g = ClampedU8(yuva.y - 0.3947f * yuva.u - 0.5806f * yuva.v);
		rgba.b = ClampedU8(yuva.y + 2.0321f * yuva.u);
		rgba.a = yuva.a;
		return rgba;
	}

	auto RGBAToYUVA(ColorU32 rgba) -> ColorU32
	{
		ColorU32 yuva;
		yuva.y = ClampedU8(0.299f * rgba.r + 0.587f * rgba.g + 0.114f * rgba.b);
		yuva.u = ClampedU8(-0.1471f * rgba.r - 0.2889f * rgba.g + 0.436f * rgba.b);
		yuva.v = ClampedU8(0.615f * rgba.r - 0.515f * rgba.g - 0.1f * rgba.b);
		yuva.a = rgba.a;
		return yuva;
	}


	auto RGBAToGrayscale(ColorU32 c) -> U8
	{
		return ClampedU8(0.299f * c.r + 0.587 * c.g + 0.114 * c.b) * (c.a / 255.f);
	}
}