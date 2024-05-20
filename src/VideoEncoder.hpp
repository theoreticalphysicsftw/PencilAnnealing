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

#include "Color.hpp"
#include "Image.hpp"
#include "Error.hpp"
#include "Logging.hpp"
#include "File.hpp"
#include "Random.hpp"

#include <ogg/ogg.h>
#include <theora/theoraenc.h>

namespace PA
{
	class VideoEncoder
	{
	public:
		struct Config
		{
			U32 width;
			U32 height;
			U32 fps = 30;
			U32 crf = 56;

			U64 cacheBufferMaxSize = U64(1) << 18;
			Str outFileName = "out.ogv";

			B encodeToFile = true;
			B logErrors = true;
		};

		VideoEncoder(const Config& cfg = Config());
		~VideoEncoder();
		auto EncodeA32Float(const RawCPUImage& img, B lastFrame = false) -> V;
		auto FlushCacheToDisk() -> V;

	private:
		auto WriteToCache(Span<const Byte> inData) -> V;

		Config cfg;
		ogg_stream_state oggStream;
		th_enc_ctx* theoraEncoder;
		th_comment theoraComment;
		th_info theoraInfo;

		Array<Byte> cache;
	};
}


namespace PA
{
	inline VideoEncoder::VideoEncoder(const Config& config):
		cfg(config)
	{
		ogg_stream_init(&oggStream, (I32)GetUniformU32(0u, ~0u));
		th_info_init(&theoraInfo);
		// The output frames need to be divisible by 16;
		auto outWidth = (cfg.width + 15) & 0xFFFFFF0u;
		auto outHeight = (cfg.height + 15) & 0xFFFFFF0u;

		theoraInfo.frame_width = outWidth;
		theoraInfo.frame_height = outHeight;
		theoraInfo.pic_width = cfg.width;
		theoraInfo.pic_height = cfg.height;
		theoraInfo.pic_x = 0;
		theoraInfo.pic_y = 0;
		theoraInfo.fps_numerator = cfg.fps;
		theoraInfo.fps_denominator = 1;
		theoraInfo.pixel_fmt = TH_PF_420;
		theoraInfo.aspect_numerator = 1;
		theoraInfo.aspect_denominator = 1;
		theoraInfo.colorspace = TH_CS_UNSPECIFIED;
		theoraInfo.quality = cfg.crf;
		theoraInfo.target_bitrate = 0;
		theoraInfo.keyframe_granule_shift = 6;

		theoraEncoder = th_encode_alloc(&theoraInfo);

		if (cfg.logErrors && theoraEncoder == nullptr)
		{
			LogError("Failed to create theora encoder instance.");
		}
		 

		th_comment_init(&theoraComment);
		ogg_packet oggPacket;
		ogg_page oggPage;
		
		if (th_encode_flushheader(theoraEncoder, &theoraComment, &oggPacket) <= 0 && cfg.logErrors)
		{
			LogError("Internal libtheora error.");
		}

		ogg_stream_packetin(&oggStream, &oggPacket);

		if (ogg_stream_pageout(&oggStream, &oggPage) != 1 && cfg.logErrors) 
		{
			LogError("Internal libogg error.");
		}

		WriteToCache(Span<const Byte>(oggPage.header, oggPage.header_len));
		WriteToCache(Span<const Byte>(oggPage.body, oggPage.body_len));

		while(true)
		{
			auto status = th_encode_flushheader(theoraEncoder, &theoraComment, &oggPacket);
			if (status < 0)
			{
				LogError("Internal libtheora library error.");
			}
			if (!status)
			{
				break;
			}
			ogg_stream_packetin(&oggStream, &oggPacket);
		}

		if (ogg_stream_pageout(&oggStream, &oggPage) > 0)
		{
			WriteToCache(Span<const Byte>(oggPage.header, oggPage.header_len));
			WriteToCache(Span<const Byte>(oggPage.body, oggPage.body_len));
		}
	}


	inline VideoEncoder::~VideoEncoder()
	{
		FlushCacheToDisk();
		th_info_clear(&theoraInfo);
		th_encode_free(theoraEncoder);
		ogg_stream_clear(&oggStream);
	}


	inline auto VideoEncoder::EncodeA32Float(const RawCPUImage& img, B lastFrame) -> V
	{
		PA_ASSERT(img.lebesgueOrdered);
		PA_ASSERT(img.format == EFormat::A32Float);

		auto paddedWidth = theoraInfo.frame_width;
		auto paddedHeight = theoraInfo.frame_height;
		auto paddedSize = paddedWidth * paddedHeight;
		Array<Byte> yData(paddedSize);
		Array<Byte> cbData(paddedSize / 4);
		Array<Byte> crData(paddedSize / 4);

		auto inPtr = (const F32*) img.data.data();
		for (auto y = 0; y < img.height; ++y)
		{
			for (auto x = 0; x < img.width; ++x)
			{
				auto i = LebesgueCurve(x, y);
				auto inGray = ClampedU8(inPtr[i] * 255);
				auto inColor = RGBAToYCbCrABT601(ColorU32(inGray, inGray, inGray, 255u));
				yData[y * paddedWidth + x] = inColor.y;
				cbData[y / 2 * paddedWidth / 2 + x / 2] = inColor.cb;
				crData[y / 2 * paddedWidth / 2 + x / 2] = inColor.cr;
			}
		}

		th_ycbcr_buffer inBuffer;
		inBuffer[0].width = paddedWidth;
		inBuffer[0].height = paddedHeight;
		inBuffer[0].stride = paddedWidth;
		inBuffer[0].data = yData.data();
		inBuffer[1].width = paddedWidth / 2;
		inBuffer[1].height = paddedHeight / 2;
		inBuffer[1].stride = paddedWidth / 2;
		inBuffer[1].data = cbData.data();
		inBuffer[2].width = paddedWidth / 2;
		inBuffer[2].height = paddedHeight / 2;
		inBuffer[2].stride = paddedWidth /2;
		inBuffer[2].data = crData.data();

		ogg_packet oggPacket;
		auto encodeStatus = th_encode_ycbcr_in(theoraEncoder, inBuffer);
		if (encodeStatus && cfg.logErrors)
		{
			LogError("Failed to encode frame (status = ", encodeStatus, ").");
		}
		auto status = th_encode_packetout(theoraEncoder, lastFrame, &oggPacket);

		if (status > 0)
		{
			ogg_page oggPage;
			ogg_stream_packetin(&oggStream, &oggPacket);
			if (ogg_stream_pageout(&oggStream, &oggPage) > 0)
			{
				WriteToCache(Span<const Byte>(oggPage.header, oggPage.header_len));
				WriteToCache(Span<const Byte>(oggPage.body, oggPage.body_len));
			}
		}
	}


	inline auto VideoEncoder::FlushCacheToDisk() -> V
	{
		WriteWholeFile(cfg.outFileName, Span<const Byte>(cache.data(), cache.size()), true);
		cache.clear();
	}


	inline auto VideoEncoder::WriteToCache(Span<const Byte> inData) -> V
	{
		if (inData.size() + cache.size() > cfg.cacheBufferMaxSize)
		{
			FlushCacheToDisk();
		}
		
		auto oldSize = cache.size();
		cache.resize(cache.size() + inData.size());
		MemCopy(inData, cache.data() + oldSize);
	}
}