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


#include "Types.hpp"
#include "Utilities.hpp"
#include "PresentSurface.hpp"
#include "EmbeddedTestImage.hpp"
#include "Webp.hpp"
#include "Annealer.hpp"

using namespace PA;

static constexpr StrView CDefaultInImagePath = "in.webp"sv;

I32 main(I32 argc, C** argv)
{
	Span<const Byte> rawImageData;
	Array<Byte> utilityBufer;
	if (argc > 1)
	{
		ReadWholeFile(argv[1], utilityBufer);
		rawImageData = Span<const Byte>(utilityBufer);
	}
	else if (FileExists(CDefaultInImagePath))
	{
		ReadWholeFile(CDefaultInImagePath, utilityBufer);
		rawImageData = Span<const Byte>(utilityBufer);
	}
	else
	{
		rawImageData = Span<const Byte>(GEmbeddedTestImageData, CEmbeddedTestImageSize);
	}

	auto decodedImage = DecodeWebP(rawImageData);

	if (decodedImage.data.empty())
	{
		Log("Cannot read input image.");
		return 1;
	}

	PA_ASSERT(PresentSurface::Init(decodedImage.width, decodedImage.height));
	Annealer<F32> annealer(&decodedImage);

	Thread annealingThread([&]() { while (!PresentSurface::IsClosed() && annealer.AnnealBezier()); annealer.ShutDownThreadPool(); });

	PresentSurface::AddRenderingCode
	(
		[&annealer]()
		{
			auto target = PresentSurface::LockScreenTarget();
			annealer.CopyCurrentApproximationToColor((ColorU32*)target.data, target.stride);
			PresentSurface::UnlockScreenTarget();
		}
	);

	PresentSurface::PresentLoop();
	return 0;
}