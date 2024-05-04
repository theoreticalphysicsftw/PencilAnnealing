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

		auto DrawLinesToSurface(const QuadTree<Line>& lines, RawCPUImage& img) -> Pair<U32, U32>;

		auto ShutDownThreadPool() -> V;

	private:
        static constexpr U32 logAfterSteps = 512;

		auto GetEnergy(const RawCPUImage& img0, Pair<U32, U32> extent) -> TF;
		auto GetEnergy(const RawCPUImage& img0) -> TF;

		auto InitBezier() -> V;

		RawCPUImage grayscaleReference;
		RawCPUImage grayscaleBlurredReference;
		RawCPUImage grayscaleReferenceEdges;
		RawCPUImage currentApproximation;
		RawCPUImage workingApproximation;
		RawCPUImage workingApproximationHDR;

		Array<QuadraticBezier> strokes;
		Array<Array<Fragment>> fragmentsMap;

		U32 maxStrokes;
		U32 maxSteps;

		Scalar temperature;
		Scalar maxTemperature;
		Scalar optimalEnergy;
		U32 step = 0;

		ThreadPool<> threadPool;
	};
}


namespace PA
{
	template<typename TF>
	inline Annealer<TF>::Annealer(const RawCPUImage* reference, U32 maxStrokes, U32 maxSteps) :
		grayscaleReference(reference->width, reference->height, EFormat::A8, true),
		currentApproximation(reference->width, reference->height, EFormat::A8, true),
		workingApproximation(reference->width, reference->height, EFormat::A8, true),
		workingApproximationHDR(reference->width, reference->height, EFormat::A32Float, true)
	{
		for (auto i = 0u; i < reference->width; ++i)
		{
			for (auto j = 0u; j < reference->height; ++j)
			{
				auto idx = LebesgueCurve(j, i);
				auto inColor = ((ColorU32*)reference->data.data())[i * reference->width + j];
				auto grayscaleColor = RGBAToGrayscale(inColor);
				this->grayscaleReference.data[idx] = grayscaleColor;
			}
		}

		for (auto i = 0u; i < currentApproximation.data.size(); ++i)
		{
			currentApproximation.data[i] = 255u;
			workingApproximation.data[i] = 255u;
		}

		currentApproximation.Clear(Byte(255));
		workingApproximation.Clear(Byte(255));
		workingApproximationHDR.Clear(F32(1.0));

		grayscaleReferenceEdges = GradientMagnitude(threadPool, grayscaleReference);

		this->maxStrokes = maxStrokes ? maxStrokes : (reference->width * reference->height / 512);
		this->maxSteps = maxSteps ? maxSteps : (1u << 24);
		this->maxTemperature = 255 * 255;
		temperature = maxTemperature;

		optimalEnergy = GetEnergy(currentApproximation);

		InitBezier();
	}

	template<typename TF>
	inline Annealer<TF>::~Annealer()
	{
		SerializeToSVG(Span<const QuadraticBezier>(strokes), grayscaleReference.width, grayscaleReference.height);
	}

	template<typename TF>
	inline auto Annealer<TF>::InitBezier() -> V
	{
		for (auto i = 0u; i < maxStrokes; ++i)
		{
			auto strokeLength = SmoothStep(TF(1) / grayscaleReference.width, TF(0.2), TF(0.2) * (1 - TF(i + 1) / maxStrokes));
			strokes.push_back(GetRandom2DQuadraticBezierInRange(strokeLength));
		}
		fragmentsMap.resize(strokes.size());
		RasterizeToFragments(Span<const QuadraticBezier>(strokes), fragmentsMap, grayscaleReference.width, grayscaleReference.height, threadPool);
		PutFragmentsOnHDRSurface(fragmentsMap, workingApproximationHDR);
		CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation);
	}


	template<typename TF>
	inline auto Annealer<TF>::CopyCurrentApproximationToColor(ColorU32* data, U32 stride) -> V
	{
		for (auto i = 0u; i < this->currentApproximation.width; ++i)
		{
			for (auto j = 0u; j < this->currentApproximation.height; ++j)
			{
				auto idx = LebesgueCurve(j, i);
				auto inColor = this->currentApproximation.data[idx];
				data[i * stride / 4 + j] = ColorU32(inColor, inColor, inColor, 255);
			}
		}
	}

	template<typename TF>
	inline auto Annealer<TF>::AnnealBezier() -> B
	{
		if (step >= maxSteps)
		{
            Log("Annealing done.");
			return false;
		}

		temperature = temperature * TF(0.999);

		auto strokeIdx = GetUniformU32(0, strokes.size() - 1);

		auto& oldCurve = strokes[strokeIdx];
		auto newCurve = GetRandom2DQuadraticBezierInRange(TF(1));
		auto& oldFragments = fragmentsMap[strokeIdx];
		Array<Fragment> newFragments;
		RasterizeToFragments(newCurve, newFragments, workingApproximationHDR.width, workingApproximationHDR.height);
		
		RemoveFragmentsFromHDRSurface(oldFragments, workingApproximationHDR);
		PutFragmentsOnHDRSurface(newFragments, workingApproximationHDR);
		// Update the surface on both the old and new fragments;
		CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, oldFragments);
		CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, newFragments);
		auto currentEnergy = GetEnergy(workingApproximation);

		auto transitionThreshold = Exp((optimalEnergy - currentEnergy) / temperature);

		if (currentEnergy < optimalEnergy || transitionThreshold >= GetUniformFloat<TF>())
		{
			optimalEnergy = currentEnergy;
			oldFragments = newFragments;
			oldCurve = newCurve;
			if (!(step % logAfterSteps) || step == maxSteps - 1)
			{
				currentApproximation = workingApproximation;
			}
		}
		else
		{
			RemoveFragmentsFromHDRSurface(newFragments, workingApproximationHDR);
			PutFragmentsOnHDRSurface(oldFragments, workingApproximationHDR);
			CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, oldFragments);
			CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, newFragments);
		}

		if (!(step % logAfterSteps))
		{
			Log("Energy = ", optimalEnergy, " Temperature = ", temperature);
		}
        step++;
        return true;
	}



	template<typename TF>
	inline auto Annealer<TF>::ShutDownThreadPool() -> V
	{
		threadPool.ShutDown();
	}


	template<typename TF>
	inline auto Annealer<TF>::GetEnergy(const RawCPUImage& img, Pair<U32, U32> extent) -> TF
	{
		auto extentSize = extent.second - extent.first;
		auto taskCount = GetLogicalCPUCount();
		auto pixelsPerTask = extentSize / taskCount;

		auto task =
		[&](U32 start, U32 end) -> TF
		{
			TF energy = 0;
			for (auto i = start; i < end; ++i)
			{
				auto diff = TF(grayscaleReference.data[i]) - TF(img.data[i]);
				energy += 0.2 * (diff * diff) / extentSize;
			}

			for (auto i = start; i < end; ++i)
			{
				auto diff = TF(grayscaleReferenceEdges.data[i]) - TF(img.data[i]);
				energy += 0.8 * (diff * diff) / extentSize;
			}
			return energy;
		};

		Array<TaskResult<TF>> results;
		for (auto i = 0u; i < taskCount; ++i)
		{
			results.emplace_back(threadPool.AddTask(task, extent.first + i * pixelsPerTask, extent.first + (i + 1) * pixelsPerTask));
		}

		// Remainder
		task(extent.first + pixelsPerTask * taskCount, extent.second);

		TF energy = 0;
		for (auto& result : results)
		{
			energy += result.Retrieve();
		}

		return energy;
	}


	template<typename TF>
	inline auto Annealer<TF>::GetEnergy(const RawCPUImage& img) -> TF
	{
		auto imgSize = img.width * img.height;
		return GetEnergy(img, Pair<U32, U32>(0u, imgSize));
	}
}
