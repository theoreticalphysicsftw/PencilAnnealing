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

#include "Rendering.hpp"
#include "Color.hpp"
#include "Utilities.hpp"
#include "Random.hpp"
#include "Serialization.hpp"
#include "ThreadPool.hpp"
#include "Convolution.hpp"

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

		Annealer(const RawCPUImage* referance, U32 maxStrokes = 0, U32 maxSteps = 0);
		~Annealer();
		auto CopyCurrentApproximationToColor(ColorU32* data, U32 stride) -> V;
		auto AnnealBezier() -> B;
		auto ShutDownThreadPool() -> V;

	private:
        static constexpr U32 logAfterSteps = 1024;
		static constexpr U32 updateScreenAfterSteps = 4096;
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

		U32 maxStrokes;
		U32 maxSteps;
		Scalar maxWidth;

		Scalar temperature;
		Scalar maxTemperature;
		Scalar optimalEnergy;

		U32 step = 0;

		Mutex currentApproximationLock;
		ThreadPool<> threadPool;
	};
}


namespace PA
{
	template<typename TF>
	inline Annealer<TF>::Annealer(const RawCPUImage* reference, U32 maxStrokes, U32 maxSteps) :
		grayscaleReference(reference->width, reference->height, EFormat::A8, true),
		grayscaleReferenceFiltered(reference->width, reference->height, EFormat::A8, true),
		currentApproximation(reference->width, reference->height, EFormat::A8, true),
		workingApproximation(reference->width, reference->height, EFormat::A8, true),
		workingApproximationHDR(reference->width, reference->height, EFormat::A32Float, true)
	{
		grayscaleReference.Clear(Byte(255));
		currentApproximation.Clear(Byte(255));
		workingApproximation.Clear(Byte(255));
		workingApproximationHDR.Clear(F32(1.0));

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

		auto grayscaleReferenceEdges = GradientMagnitude(threadPool, grayscaleReference);
		grayscaleReferenceFiltered = AdditiveBlendA8(grayscaleReference, grayscaleReferenceEdges, 0.2f);

		FindEdgeSupport();

		this->maxWidth = 10;
		this->maxStrokes = maxStrokes ? maxStrokes : (reference->width * reference->height / 256);
		this->maxSteps = maxSteps ? maxSteps : (1u << 25);
		this->maxTemperature = 255 * 255;
		temperature = maxTemperature;

		if (FileExists(CSaveFile))
		{
			LoadProgress();
		}
		else
		{
			InitBezier();
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
		}

		PutFragmentsOnHDRSurface(fragmentsMap, workingApproximationHDR);
		CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation);
		currentApproximation = workingApproximation;
		optimalEnergy = GetEnergy(currentApproximation);
	}

	template<typename TF>
	inline Annealer<TF>::~Annealer()
	{
		PruneCurves();
		SaveProgress();
		SerializeToSVG(Span<const QuadraticBezier>(strokes), grayscaleReference.width, grayscaleReference.height);
	}

	template<typename TF>
	inline auto Annealer<TF>::InitBezier() -> V
	{
		for (auto i = 0u; i < maxStrokes; ++i)
		{
			strokes.push_back(GetRandom2DQuadraticBezierInRange(TF(1)));
			widths.push_back(GetUniformFloat(TF(1), TF(maxWidth)));
			pigments.push_back(GetUniformFloat(TF(0), TF(1)));
		}
	}

	template<typename TF>
	inline auto Annealer<TF>::FindEdgeSupport() -> V
	{
		for (auto i = 0u; i < grayscaleReferenceFiltered.height; ++i)
		{
			for (auto j = 0u; j < grayscaleReferenceFiltered.width; ++j)
			{
				auto idx = LebesgueCurve(j, i);
				if (grayscaleReferenceFiltered.data[idx] < 255)
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
		if (step < maxSteps)
		{
			Serialize(outBuffer, this->maxSteps);
			Serialize(outBuffer, this->maxStrokes);
			Serialize(outBuffer, this->maxSteps);
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
			Deserialize(inData, this->maxSteps);
			Deserialize(inData, this->maxStrokes);
			Deserialize(inData, this->maxSteps);
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
		if (step >= maxSteps)
		{
            Log("Annealing done.");
			return false;
		}

		auto startTime = GetTimeStampUS();

		temperature = temperature * TF(0.999);

		auto strokeIdx = GetUniformU32(0, strokes.size() - 1);

		auto& oldCurve = strokes[strokeIdx];
		auto& oldFragments = fragmentsMap[strokeIdx];
		auto& oldWidth = widths[strokeIdx];
		auto& oldPigment = pigments[strokeIdx];

		auto s0 = GetUniformU32(0, edgeSupport.size() - 1);

		auto maxLength = grayscaleReference.width * TF(0.2);
		auto length = GetUniformFloat(TF(3), maxLength);
		auto p1U = LebesgueCurveInverse(edgeSupport[s0]);
		auto p1 = Vec(p1U.first, p1U.second);
		auto a0 = GetUniformFloat(TF(0), Constants<TF>::C2Pi);
		auto a2 = GetUniformFloat(TF(0), Constants<TF>::C2Pi);
		auto p0 = p1 + length * Vec(Cos(a0), Sin(a0));
		auto p2 = p1 + length * Vec(Cos(a2), Sin(a2));
		auto newCurve = GetBezierPassingThrough(p0, p1, p2);

		grayscaleReference.ToNormalizedCoordinates(Span<Vec>(newCurve.points));
		auto newPigment = GetUniformFloat(TF(0), TF(1));
		auto newWidth = Min(maxWidth, GetExponentialFloat(TF(1) / maxWidth));

		Array<Fragment> newFragments;
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

		if (addEnergy < currentEnergy)
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
			optimalEnergy -= energyImprovement;
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
				AddCurve(Move(newCurve), Move(newFragments), newPigment, newWidth);
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

		if (!(step % updateScreenAfterSteps) || step == maxSteps - 1)
		{
			currentApproximationLock.lock();
			currentApproximation = workingApproximation;
			currentApproximationLock.unlock();
		}

		step++;
		auto progress = TF(step) / maxSteps * TF(100);

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

				auto diff = TF(grayscaleReferenceFiltered.data[i]) - TF(img.data[i]);
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
		Set<U32> visitedFragments;
		TF energy = 0;
		auto imgSize = img.width * img.height;

		auto collectEnergy =
		[&](const Array<Fragment>& fragments) -> V
		{
			for (auto& frag : fragments)
			{
				auto i = frag.idx;
				if (visitedFragments.contains(i))
				{
					continue;
				}

				auto diff = TF(grayscaleReferenceFiltered.data[i]) - TF(img.data[i]);
				energy += (diff * diff) / imgSize;

				visitedFragments.emplace(i);
			}
		};

		collectEnergy(f0);
		collectEnergy(f1);

		return energy;
	}
}
