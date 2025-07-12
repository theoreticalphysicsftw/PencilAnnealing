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
#include "CLI.hpp"

using namespace PA;

I32 main(I32 argc, const C** argv)
{
	Annealer<F32>::Config cfg;
	Str inImagePath = "in.webp";
	B recordOptimization = false;

	CLI::Parser cliParser;
	cliParser.Add("--in", inImagePath);
	cliParser.Add("--recordOptimization", recordOptimization);
	cliParser.Add("--maxStrokes", cfg.maxStrokes);
	cliParser.Add("--maxSteps", cfg.maxSteps);
	cliParser.Add("--maxWidth", cfg.maxWidth);
	cliParser.Add("--serializeToSVG", cfg.serializeToSVG);
	cliParser.Add("--serializeToVideo", cfg.serializeToVideo);
	cliParser.Add("--screenCutoff", cfg.screenCutoff);
	cliParser.Add("--screenCutoffRadius", cfg.screenCutoffRadius);
	cliParser.Add("--darkOnLight", cfg.darkOnLight);
	cliParser.Add("--bgLightness", cfg.bgLightness);
	cliParser.Add("--edgeContribution", cfg.edgeContribution);
	cliParser.Add("--nonRandomStrokeSelection", cfg.nonRandomStrokeSelection);
	cliParser.Parse(argc, argv);

	Span<const Byte> rawImageData;
	Array<Byte> utilityBufer;
	
	if (FileExists(inImagePath))
	{
		ReadWholeFile(inImagePath, utilityBufer);
		rawImageData = Span<const Byte>(utilityBufer);
	}
	else
	{
		LogError("File \"", inImagePath, "\" not found!");
		rawImageData = Span<const Byte>(GEmbeddedTestImageData, CEmbeddedTestImageSize);
	}

	auto decodedImage = DecodeWebP(rawImageData);

	if (decodedImage.data.empty())
	{
		LogError("Cannot read input image!");
		return 1;
	}

	auto [displayW, displayH] = PresentSurface::GetDisplayRes();

	auto windowWidth = decodedImage.width;
	auto windowHeight = decodedImage.height;
	F32 scale = 1.f;

	if (displayW < windowWidth || displayH < windowHeight)
	{
		scale = Min(displayW / F32(windowWidth), displayH / F32(windowHeight));
		scale *= 0.9;
	}

	PA_ASSERT
	(
		PresentSurface::Init
		(
			U32(windowWidth * scale),
			U32(windowHeight * scale),
			decodedImage.width,
			decodedImage.height
		)
	);

	Annealer<F32> annealer(&decodedImage, cfg);

	Thread annealingThread([&]() { while (!PresentSurface::IsClosed() && annealer.AnnealBezier()); annealer.ShutDownThreadPool(); });

	VideoEncoder* encoder = nullptr;
	if (recordOptimization)
	{
		VideoEncoder::Config cfg;
		cfg.width = decodedImage.width;
		cfg.height = decodedImage.height;
		cfg.fps = 30;
		cfg.crf = 63;
		cfg.outFileName = "optimization.ogv";
		RemoveFile(cfg.outFileName);
		encoder = new VideoEncoder(cfg);
	}

	PresentSurface::AddRenderingCode
	(
		[&annealer, encoder, recordOptimization]()
		{
			auto target = PresentSurface::LockScreenTarget();
			annealer.CopyCurrentApproximationToColor((ColorU32*)target.data, target.stride);
			if (recordOptimization)
			{
				encoder->EncodeRGBA8Linear(target, false);
			}
			PresentSurface::UnlockScreenTarget();
		}
	);

	PresentSurface::PresentLoop();
	return 0;
}