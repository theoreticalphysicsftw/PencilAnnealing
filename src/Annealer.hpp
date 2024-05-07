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

		auto GetEnergy(const RawCPUImage& img0) -> TF;

		auto InitBezier() -> V;
		auto FindEdgeSupport() -> V;

		RawCPUImage grayscaleReference;
		RawCPUImage grayscaleBlurredReference;
		RawCPUImage grayscaleReferenceEdges;
		RawCPUImage currentApproximation;
		RawCPUImage workingApproximation;
		RawCPUImage workingApproximationHDR;

		Array<QuadraticBezier> strokes;
		Array<Array<Fragment>> fragmentsMap;

		Array<U32> edgeSupport;

		U32 maxStrokes;
		U32 maxSteps;

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

		grayscaleReferenceEdges = GradientMagnitude(threadPool, grayscaleReference);

		FindEdgeSupport();

		this->maxStrokes = maxStrokes ? maxStrokes : (reference->width * reference->height / 256);
		this->maxSteps = maxSteps ? maxSteps : (1u << 25);
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
			strokes.push_back(GetRandom2DQuadraticBezierInRange(TF(1)));
		}
		fragmentsMap.resize(strokes.size());
		RasterizeToFragments(Span<const QuadraticBezier>(strokes), fragmentsMap, grayscaleReference.width, grayscaleReference.height, threadPool);
		PutFragmentsOnHDRSurface(fragmentsMap, workingApproximationHDR);
		CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation);
	}

	template<typename TF>
	inline auto Annealer<TF>::FindEdgeSupport() -> V
	{
		for (auto i = 0; i < grayscaleReferenceEdges.height; ++i)
		{
			for (auto j = 0; j < grayscaleReferenceEdges.width; ++j)
			{
				auto idx = LebesgueCurve(j, i);
				if (grayscaleReferenceEdges.data[idx] < 255)
				{
					edgeSupport.push_back(idx);
				}
			}
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
				auto inColor = this->grayscaleReferenceEdges.data[idx];
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

		temperature = temperature * TF(0.999);

		auto strokeIdx = GetUniformU32(0, strokes.size() - 1);

		auto& oldCurve = strokes[strokeIdx];
		auto& oldFragments = fragmentsMap[strokeIdx];

		auto s0 = GetUniformU32(0, edgeSupport.size() - 1);
		//auto s1 = GetUniformU32(0, edgeSupport.size() - 1);
		//auto s2 = GetUniformU32(0, edgeSupport.size() - 1);
		//auto p0 = LebesgueCurveInverse(edgeSupport[s0]);
		//auto p1 = LebesgueCurveInverse(edgeSupport[s1]);
		//auto p2 = LebesgueCurveInverse(edgeSupport[s2]);

		auto maxLength = grayscaleReference.width * TF(0.1);
		auto length = GetUniformFloat(TF(3), maxLength);
		auto p1U = LebesgueCurveInverse(edgeSupport[s0]);
		auto p1 = Vec(p1U.first, p1U.second);
		auto a0 = GetUniformFloat(TF(0), Constants<TF>::C2Pi);
		auto a2 = GetUniformFloat(TF(0), Constants<TF>::C2Pi);
		auto p0 = p1 + length * Vec(Cos(a0), Sin(a0));
		auto p2 = p1 + length * Vec(Cos(a2), Sin(a2));
		auto newCurve = GetBezierPassingThrough(p0, p1, p2);

		//auto newCurve = GetBezierPassingThrough(Vec(p0.first, p0.second), Vec(p1.first, p1.second), Vec(p2.first, p2.second));
		grayscaleReference.ToNormalizedCoordinates(Span<Vec>(newCurve.points));
		auto newColor = GetUniformFloat(TF(0), TF(1));
		//auto newCurve = GetRandom2DQuadraticBezierInRange(TF(1));

		Array<Fragment> newFragments;
		RasterizeToFragments(newCurve, newFragments, workingApproximationHDR.width, workingApproximationHDR.height, newColor);
		
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
		}
		else
		{
			RemoveFragmentsFromHDRSurface(newFragments, workingApproximationHDR);
			PutFragmentsOnHDRSurface(oldFragments, workingApproximationHDR);
			CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, oldFragments);
			CopyHDRSurfaceToGSSurface(workingApproximationHDR, workingApproximation, newFragments);
		}

		if (!(step % updateScreenAfterSteps) || step == maxSteps - 1)
		{
			currentApproximationLock.lock();
			currentApproximation = workingApproximation;
			currentApproximationLock.unlock();
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

				auto diff0 = TF(grayscaleReference.data[i]) - TF(img.data[i]);
				energy += 0.1 * (diff0 * diff0) / imgSize;

				auto diff1 = TF(grayscaleReferenceEdges.data[i]) - TF(img.data[i]);
				energy += 0.9 * (diff1 * diff1) / imgSize;
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
}
