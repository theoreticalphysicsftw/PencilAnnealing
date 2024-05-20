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