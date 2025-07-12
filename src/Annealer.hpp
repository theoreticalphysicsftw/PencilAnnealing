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

#include "Rendering.hpp"
#include "Color.hpp"
#include "Utilities.hpp"
#include "Random.hpp"
#include "Serialization.hpp"
#include "ThreadPool.hpp"
#include "Convolution.hpp"
#include "SDF.hpp"

namespace PA
{
	template <typename TF>
	class Annealer
	{
	public:
		using Scalar = TF;
		using Vec = Vector<Scalar, 2>;
		using QuadraticBezier = QuadraticBezier<Scalar, 2>;
		using Arc = Arc<Scalar>;
		using Line = Line<Scalar, 2>;

		struct Config
		{
			U32 maxStrokes = 0u;
			U32 maxSteps = 1u << 26;
			F32 maxWidth = 3.f;
			F32 edgeContribution = 0.3f;
			F32 screenCutoff = 0.2f;
			F32 screenCutoffRadius = 0.2f;
			U8 bgLightness = 255;
			B serializeToSVG = true;
			B serializeToVideo = true;
			B darkOnLight = true;
			B nonRandomStrokeSelection = false;
		};

		Annealer(const RawCPUImage* referance, const Config& cfg = Config());
		~Annealer();
		auto CopyCurrentApproximationToColor(ColorU32* data, U32 stride) -> V;
		auto AnnealBezier() -> B;
		auto ShutDownThreadPool() -> V;

	private:
        static constexpr U32 logAfterSteps = 1u << 16;
		static constexpr U32 updateScreenAfterSteps = 1024;
		static constexpr StrView CSaveFile = "save.pa"sv;

		auto GetEnergy(const RawCPUImage& img0) -> TF;

		auto GetLocalEnergy(const RawCPUImage& img0, const Array<Fragment>& f0, const Array<Fragment>& f1) -> TF;

		auto InitBezier() -> V;
		auto FindEdgeSupport() -> V;

		auto RemoveCurve(U32 curveIdx) -> V;
		auto AddCurve(QuadraticBezier&& newCurve, Array<Fragment>&& newFragments, Scalar width, Scalar pigment) -> V;
		auto PruneCurves() -> V;

		auto SaveProgress() -> V;
		auto LoadProgress() -> V;

		auto InsideInterestRegion(U32 i, U32 j) const -> B;
		auto InsideInterestRegion(U32 i) const -> B;

		RawCPUImage grayscaleReference;
		RawCPUImage grayscaleReferenceFiltered;
		RawCPUImage currentApproximation;
		RawCPUImage workingApproximation;
		RawCPUImage workingApproximationHDR;

		Array<QuadraticBezier> strokes;
		Array<Scalar> widths;
		Array<Scalar> pigments;
		Array<Array<Fragment>> fragmentsMap;

		Array<U32> edgeSupport;

		Config config;

		Scalar temperature;
		Scalar maxTemperature;
		Scalar optimalEnergy;

		U32 step = 0;

		using FragmentsMapDrawFunc = Void(*)(Array<Array<Fragment>>& fragments, RawCPUImage& surface);
		using FragmentsDrawFunc = Void(*)(Array<Fragment>& fragments, RawCPUImage& surface);
		FragmentsMapDrawFunc PutFragmentsMapOnHDRSurface = nullptr;
		FragmentsDrawFunc PutFragmentsOnHDRSurface = nullptr;
		FragmentsDrawFunc RemoveFragmentsFromHDRSurface = nullptr;

		Mutex currentApproximationLock;
		ThreadPool<> threadPool;
	};
}


namespace PA
{
	template<typename TF>
	inline Annealer<TF>::Annealer(const RawCPUImage* reference, const Config& cfg) :
		grayscaleReference(reference->width, reference->height, EFormat::A8, true),
		grayscaleReferenceFiltered(reference->width, reference->height, EFormat::A8, true),
		currentApproximation(reference->width, reference->height, EFormat::A8, true),
		workingApproximation(reference->width, reference->height, EFormat::A8, true),
		workingApproximationHDR(reference->width, reference->height, EFormat::A32Float, true)
	{
		if (cfg.darkOnLight)
		{
			PutFragmentsOnHDRSurface = SubtractFragmentsFromHDRSurface;
			RemoveFragmentsFromHDRSurface = AddFragmentsOnHDRSurface;
			PutFragmentsMapOnHDRSurface = SubtractFragmentsFromHDRSurface;
		}
		else
		{
			PutFragmentsOnHDRSurface = AddFragmentsOnHDRSurface;
			RemoveFragmentsFromHDRSurface = SubtractFragmentsFromHDRSurface;
			PutFragmentsMapOnHDRSurface = AddFragmentsOnHDRSurface;
		}

		grayscaleReference.Clear(Byte(cfg.bgLightness));
		currentApproximation.Clear(Byte(cfg.bgLightness));
		workingApproximation.Clear(Byte(cfg.bgLightness));
		workingApproximationHDR.Clear(F32(cfg.bgLightness / 255.f));

		for (auto i = 0u; i < reference->height; ++i)
		{
			for (auto j = 0u; j < reference->width; ++j)
			{
				auto idx = LebesgueCurve(j, i);
				auto inColor = ((ColorU32*)reference->data.data())[i * reference->width + j];
				auto grayscaleColor = RGBAToGrayscale(inColor);
				this->grayscaleReference.data[idx] = grayscaleColor;
			}
		}

		config = cfg;
		config.maxStrokes = cfg.maxStrokes ? cfg.maxStrokes : (reference->width * reference->height / 256);
		config.edgeContribution = Clamp(cfg.edgeContribution, 0.f, 1.f);

		auto grayscaleReferenceEdges = GradientMagnitude(threadPool, grayscaleReference);
		grayscaleReferenceFiltered = AdditiveBlendA8(grayscaleReference, grayscaleReferenceEdges, 1.f - config.edgeContribution);

		FindEdgeSupport();

		this->maxTemperature = 255 * 255;
		temperature = maxTemperature;

		if (FileExists(CSaveFile))
		{
			LoadProgress();
		}
		else
		{
			InitBezier();
		}

		fragmentsMap.resize(strokes.size());
		RasterizeToFragments
		(
			Span<const QuadraticBezier>(strokes),
			Span<const TF>(widths),
			Span<const TF>(pigments),
			fragmentsMap,
			grayscaleReference.width,
			grayscaleReference.height,
			threadPool
		);

		PutFragmentsMapOnHDRSurface(fragmentsMap, workingApproximationHDR);
		CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation);
		currentApproximation = workingApproximation;
		optimalEnergy = GetEnergy(currentApproximation);
	}

	template<typename TF>
	inline Annealer<TF>::~Annealer()
	{
		PruneCurves();
		SaveProgress();
		SerializeToWebP(workingApproximationHDR);
		// TODO: Fix SerializeToSVG for dark backgrounds.
		SerializeToSVG
		(
			Span<const QuadraticBezier>(strokes),
			Span<const TF>(widths),
			Span<const TF>(pigments),
			grayscaleReference.width,
			grayscaleReference.height
		);
		SerializeToVideo
		(
			Span<const QuadraticBezier>(strokes),
			Span<const TF>(widths),
			Span<const TF>(pigments),
			grayscaleReference.width,
			grayscaleReference.height,
			config.darkOnLight,
			config.bgLightness
		);
	}

	template<typename TF>
	inline auto Annealer<TF>::InitBezier() -> V
	{
		for (auto i = 0u; i < config.maxStrokes; ++i)
		{
			strokes.push_back(GetRandom2DQuadraticBezierInRange(TF(1)));
			widths.push_back(GetUniformFloat(TF(1), TF(config.maxWidth)));
			pigments.push_back(GetUniformFloat(TF(0), TF(1)));
		}
	}

	template <typename TF>
	inline auto Annealer<TF>::InsideInterestRegion(U32 i, U32 j) const -> B
	{
		auto p = Vec(i, j);
		auto pNorm = grayscaleReference.ToNormalizedCoordinates(p) - TF(0.5);
		return SDF::Round(SDF::Box2D(pNorm, Vec(TF(config.screenCutoff))), TF(config.screenCutoffRadius)) < TF(0);
	}

	template <typename TF>
	inline auto Annealer<TF>::InsideInterestRegion(U32 i) const -> B
	{
		auto [x, y] = LebesgueCurveInverse(i);
		return InsideInterestRegion(x, y);
	}

	template<typename TF>
	inline auto Annealer<TF>::FindEdgeSupport() -> V
	{
		for (auto i = 0u; i < grayscaleReferenceFiltered.height; ++i)
		{
			for (auto j = 0u; j < grayscaleReferenceFiltered.width; ++j)
			{
				if (i % 4 == 0 && j % 4 == 0 || !InsideInterestRegion(i, j))
				{
					continue;
				}

				auto idx = LebesgueCurve(j, i);
				if (
						(config.darkOnLight && grayscaleReferenceFiltered.data[idx] < config.bgLightness) ||
						(!config.darkOnLight && grayscaleReferenceFiltered.data[idx] > config.bgLightness)
				   )
				{
					edgeSupport.push_back(idx);
				}
			}
		}
	}

	template<typename TF>
	inline auto Annealer<TF>::RemoveCurve(U32 curveIdx) -> V
	{
		Swap(strokes[curveIdx], strokes.back());
		strokes.pop_back();
		Swap(widths[curveIdx], widths.back());
		widths.pop_back();
		Swap(pigments[curveIdx], pigments.back());
		pigments.pop_back();
		Swap(fragmentsMap[curveIdx], fragmentsMap.back());
		fragmentsMap.pop_back();
	}

	template<typename TF>
	inline auto Annealer<TF>::AddCurve(QuadraticBezier&& newCurve, Array<Fragment>&& newFragments, TF width, TF pigment) -> V
	{
		strokes.emplace_back(newCurve);
		widths.emplace_back(width);
		pigments.emplace_back(pigment);
		fragmentsMap.emplace_back(newFragments);
	}

	template<typename TF>
	inline auto Annealer<TF>::PruneCurves() -> V
	{
		for (auto i = 0; i < strokes.size(); ++i)
		{
			auto& oldCurve = strokes[i];
			auto& oldFragments = fragmentsMap[i];

			if (oldFragments.empty())
			{
				RemoveCurve(i);
				i--;
				continue;
			}

			auto localEnergy = GetLocalEnergy(workingApproximation, oldFragments, Array<Fragment>());

			RemoveFragmentsFromHDRSurface(oldFragments, workingApproximationHDR);
			CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, oldFragments);
			auto removeEnergy = GetLocalEnergy(workingApproximation, oldFragments, Array<Fragment>());

			if (removeEnergy <= localEnergy)
			{
				RemoveCurve(i);
				i--;
			}
			else
			{
				PutFragmentsOnHDRSurface(oldFragments, workingApproximationHDR);
				CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, oldFragments);
			}
		}
	}

	template<typename TF>
	inline auto Annealer<TF>::SaveProgress() -> V
	{
		Array<Byte> outBuffer;
		if (step < config.maxSteps)
		{
			Serialize(outBuffer, this->config.maxSteps);
			Serialize(outBuffer, this->config.maxStrokes);
			Serialize(outBuffer, this->config.maxWidth);
			Serialize(outBuffer, this->maxTemperature);
			Serialize(outBuffer, this->step);
			Serialize(outBuffer, this->temperature);
			Serialize(outBuffer, this->optimalEnergy);
			Serialize(outBuffer, this->strokes);
			Serialize(outBuffer, this->widths);
			Serialize(outBuffer, this->pigments);
			Serialize(outBuffer, this->fragmentsMap);
		}
		WriteWholeFile(CSaveFile, outBuffer);
	}

	template<typename TF>
	inline auto Annealer<TF>::LoadProgress() -> V
	{
		Array<Byte> inBuffer;
		if (ReadWholeFile(CSaveFile, inBuffer))
		{
			Span<const Byte> inData(inBuffer.data(), inBuffer.size());
			Deserialize(inData, this->config.maxSteps);
			Deserialize(inData, this->config.maxStrokes);
			Deserialize(inData, this->config.maxWidth);
			Deserialize(inData, this->maxTemperature);
			Deserialize(inData, this->step);
			Deserialize(inData, this->temperature);
			Deserialize(inData, this->optimalEnergy);
			Deserialize(inData, this->strokes);
			Deserialize(inData, this->widths);
			Deserialize(inData, this->pigments);
			Deserialize(inData, this->fragmentsMap);
		}
	}


	template<typename TF>
	inline auto Annealer<TF>::CopyCurrentApproximationToColor(ColorU32* data, U32 stride) -> V
	{
		currentApproximationLock.lock();
		for (auto i = 0u; i < this->currentApproximation.height; ++i)
		{
			for (auto j = 0u; j < this->currentApproximation.width; ++j)
			{
				auto idx = LebesgueCurve(j, i);
				auto inColor = this->currentApproximation.data[idx];
				data[i * stride / 4 + j] = ColorU32(inColor, inColor, inColor, 255);
			}
		}
		currentApproximationLock.unlock();
	}

	template<typename TF>
	inline auto Annealer<TF>::AnnealBezier() -> B
	{
		if (step >= config.maxSteps)
		{
            Log("Annealing done.");
			return false;
		}

		auto startTime = GetTimeStampUS();

		temperature = temperature * TF(0.999);

		U32 strokeIdx = 0;
		if (config.nonRandomStrokeSelection)
		{
			static U32 counter = 0;
			strokeIdx = counter;
			counter = (counter + 1) % strokes.size();
	 	}
		else
		{
			strokeIdx = GetUniformU32(0, strokes.size() - 1);
		}

		auto& oldCurve = strokes[strokeIdx];
		auto& oldFragments = fragmentsMap[strokeIdx];
		auto& oldWidth = widths[strokeIdx];
		auto& oldPigment = pigments[strokeIdx];

		Array<Fragment> newFragments;
		QuadraticBezier newCurve;

		B validCurve = false; 
		
		auto maxLength = grayscaleReference.width * TF(0.1);
		auto length = GetUniformFloat(TF(3), maxLength);

		while (!validCurve) {
			auto s0 = GetUniformU32(0, edgeSupport.size() - 1);
			auto p1U = LebesgueCurveInverse(edgeSupport[s0]);
			auto p1 = Vec(p1U.first, p1U.second);
			auto p1Norm = grayscaleReference.ToNormalizedCoordinates(p1) - TF(0.5);
			if (InsideInterestRegion(p1U.first, p1U.second)) {
				auto a0 = GetUniformFloat(TF(0), Constants<TF>::C2Pi);
				auto a2 = GetUniformFloat(TF(0), Constants<TF>::C2Pi);
				auto p0 = p1 + length * Vec(Cos(a0), Sin(a0));
				auto p2 = p1 + length * Vec(Cos(a2), Sin(a2));
				newCurve = GetBezierPassingThrough(p0, p1, p2);
				validCurve = true;
			}
		}


		grayscaleReference.ToNormalizedCoordinates(Span<Vec>(newCurve.points));
		auto newPigment = GetUniformFloat(TF(0.01), TF(1));
		auto newWidth = Min(config.maxWidth, GetExponentialFloat((TF(2) / config.maxWidth)) * temperature + 1);

		RasterizeToFragments
		(
			newCurve,
			newFragments,
			workingApproximationHDR.width,
			workingApproximationHDR.height,
			newPigment,
			newWidth
		);
		
		auto localEnergy = GetLocalEnergy(workingApproximation, oldFragments, newFragments);

		RemoveFragmentsFromHDRSurface(oldFragments, workingApproximationHDR);
		CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, oldFragments);
		auto removeEnergy = GetLocalEnergy(workingApproximation, oldFragments, newFragments);

		PutFragmentsOnHDRSurface(newFragments, workingApproximationHDR);
		CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, newFragments);
		auto updateEnergy = GetLocalEnergy(workingApproximation, oldFragments, newFragments);

		PutFragmentsOnHDRSurface(oldFragments, workingApproximationHDR);
		CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, oldFragments);
		auto addEnergy = GetLocalEnergy(workingApproximation, oldFragments, newFragments);

		enum class OpType { Remove, Update, Add } opType = OpType::Remove;

		auto currentEnergy = removeEnergy;

		if (updateEnergy < currentEnergy)
		{
			currentEnergy = updateEnergy;
			opType = OpType::Update;
		}

		if (addEnergy < currentEnergy && strokes.size() < config.maxStrokes)
		{
			currentEnergy = addEnergy;
			opType = OpType::Add;
		}

		auto energyImprovement = localEnergy - currentEnergy;
		auto transitionThreshold = Exp((energyImprovement) / temperature);
		auto minPixelImprovement = TF(5) / (workingApproximation.width * workingApproximation.height);

		// Never add new curves for no reason.
		if (energyImprovement < minPixelImprovement && opType == OpType::Add)
		{
			transitionThreshold = 0;
		}

		if (currentEnergy < localEnergy || transitionThreshold > GetUniformFloat<TF>())
		{
			optimalEnergy -= energyImprovement > 0 ? energyImprovement : 0.f;
			if (opType == OpType::Remove)
			{
				RemoveFragmentsFromHDRSurface(newFragments, workingApproximationHDR);
				CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, newFragments);
				RemoveFragmentsFromHDRSurface(oldFragments, workingApproximationHDR);
				CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, oldFragments);
				RemoveCurve(strokeIdx);
			}
			else if (opType == OpType::Add)
			{
				AddCurve(Move(newCurve), Move(newFragments), newWidth, newPigment);
			}
			else
			{
				RemoveFragmentsFromHDRSurface(oldFragments, workingApproximationHDR);
				CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, oldFragments);
				oldFragments = newFragments;
				oldCurve = newCurve;
				oldWidth = newWidth;
				oldPigment = newPigment;
			}
		}
		else
		{
			RemoveFragmentsFromHDRSurface(newFragments, workingApproximationHDR);
			CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, newFragments);
		}

		if (!(step % updateScreenAfterSteps) || step == config.maxSteps - 1)
		{
			currentApproximationLock.lock();
			currentApproximation = workingApproximation;
			currentApproximationLock.unlock();
		}

		step++;
		auto progress = TF(step) / config.maxSteps * TF(100);

		auto endTime = GetTimeStampUS();

		static auto avgTime = TF(0);

		avgTime += endTime - startTime;

		if (!(step % logAfterSteps))
		{
			avgTime /= TF(logAfterSteps);
			Log
			(
				"Energy = ",
				optimalEnergy,
				"\tTemperature = ",
				temperature,
				"\tStrokesCount = ",
				strokes.size(),
				"\tProgress = ",
				progress,
				"%",
				"\tAvgStepTime = ",
				avgTime,
				"us"
			);
			avgTime = 0;
		}

        return true;
	}



	template<typename TF>
	inline auto Annealer<TF>::ShutDownThreadPool() -> V
	{
		threadPool.ShutDown();
	}


	template<typename TF>
	inline auto Annealer<TF>::GetEnergy(const RawCPUImage& img) -> TF
	{
		auto extentSize = img.lebesgueStride * img.lebesgueStride;
		auto imgSize = img.width * img.height;
		auto taskCount = GetLogicalCPUCount();
		auto pixelsPerTask = extentSize / taskCount;

		auto task =
		[&](U32 start, U32 end) -> TF
		{
			TF energy = 0;
			for (auto i = start; i < end; ++i)
			{
				auto coords = LebesgueCurveInverse(i);
				if (coords.first >= img.width || coords.second >= img.height)
				{
					continue;
				}

				auto ref = TF(grayscaleReferenceFiltered.data[i]);
				auto diff = ref - TF(img.data[i]);
				energy += (diff * diff) / imgSize;
			}
			return energy;
		};

		Array<TaskResult<TF>> results;
		for (auto i = 0u; i < taskCount; ++i)
		{
			results.emplace_back(threadPool.AddTask(task, i * pixelsPerTask, (i + 1) * pixelsPerTask));
		}

		// Remainder
		task(pixelsPerTask * taskCount, extentSize);

		TF energy = 0;
		for (auto& result : results)
		{
			energy += result.Retrieve();
		}

		return energy;
	}


	template<typename TF>
	inline auto Annealer<TF>::GetLocalEnergy(const RawCPUImage& img, const Array<Fragment>& f0, const Array<Fragment>& f1) -> TF
	{
		static DynamicBitset visitedFragments;
		visitedFragments.Expand(img.lebesgueStride * img.lebesgueStride);

		TF energy = 0;
		auto imgSize = img.width * img.height;

		auto collectEnergy =
		[&](const Array<Fragment>& fragments) -> V
		{
			for (auto& frag : fragments)
			{
				auto i = frag.idx;
				if (visitedFragments.GetBitUnsafe(i))
				{
					continue;
				}

				auto ref = TF(grayscaleReferenceFiltered.data[i]);
				auto diff = ref - TF(img.data[i]);
				energy += (diff * diff) / imgSize;

				visitedFragments.SetBitUnsafe(i);
			}
		};

		auto clearVisited = 
		[&](const Array<Fragment>& fragments)
		{
			for (auto& frag : fragments)
			{
				visitedFragments.ClearBitUnsafe(frag.idx);
			}
		};

		collectEnergy(f0);
		collectEnergy(f1);

		clearVisited(f0);
		clearVisited(f1);

		return energy;
	}
}
