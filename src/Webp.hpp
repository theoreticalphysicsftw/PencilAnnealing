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

#include <libwebp/src/webp/decode.h>
#include <libwebp/src/webp/encode.h>

#include "Types.hpp"
#include "Image.hpp"
#include "Error.hpp"

namespace PA
{
	inline auto DecodeWebP(Span<const Byte> data) -> RawCPUImage;
	inline auto EncodeWebP(const RawCPUImage& img, F32 qf = 50.f) -> Array<Byte>;
}

namespace PA
{
	inline auto DecodeWebP(Span<const Byte> data) -> RawCPUImage
	{
		RawCPUImage result;
		result.format = EFormat::RGBA8;
		I32 width, height;

		WebPGetInfo(data.data(), data.size(), &width, &height);
		result.width = width;
		result.height = height;
		result.data.resize(result.height * result.width * sizeof(U32));

		auto decodeResult =
			WebPDecodeRGBAInto(data.data(), data.size(), result.data.data(), result.data.size(), result.width * sizeof(U32));

		if (decodeResult == nullptr)
		{
			result.data.clear();
			result.width = 0;
			result.height = 0;
		}

		return result;
	}

	inline auto EncodeWebP(const RawCPUImage& img, F32 qf) -> Array<Byte>
	{
		PA_ASSERT(img.format == EFormat::RGBA8);
		U8* out;
		auto size = WebPEncodeRGBA(img.data.data(), img.width, img.height, img.width * 4, qf, &out);
		// Unnecessary copy but whatever it's not performance critical function.
		Array<Byte> result(out, out + size);
		WebPFree(out);
		return result;
	}
}