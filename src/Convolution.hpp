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

#include "Types.hpp"
#include "Image.hpp"
#include "Error.hpp"
#include "ThreadPool.hpp"
#include "Vector.hpp"
#include "Utilities.hpp"


namespace PA
{
	template <typename TF, U32 Channels, U32 Dim0, U32 Dim1, B UniformPerChannel = true>
	struct Kernel
	{
		using Scalar = TF;
		static constexpr U32 dimension0 = Dim0;
		static constexpr U32 dimension1 = Dim1;
		static constexpr U32 channels = Channels;

		template <typename... TArgs>
		constexpr Kernel(TArgs... args);

		Conditional<UniformPerChannel, StaticArray<TF, Dim0 * Dim1>, StaticArray<TF, Dim0 * Dim1 * Channels>> data;
	};

	template <typename TKernel>
	inline auto Convolute(ThreadPool<>& threadPool, const TKernel& kernel, const RawCPUImage& input) -> RawCPUImage;

	template <typename TF, U32 Channels>
	inline static constexpr auto SobelX = Kernel<TF, Channels, 3, 3, true>(TF(-1), TF(0), TF(1), TF(-2), TF(0), TF(2), TF(-1), TF(0), TF(-1));

	template <typename TF, U32 Channels>
	inline static constexpr auto SobelY = Kernel<TF, Channels, 3, 3, true>(TF(1), TF(2), TF(1), TF(0), TF(0), TF(0), TF(-1), TF(-2), TF(-1));

	inline auto SobelEdgeDetect(ThreadPool<>& threadPool, const RawCPUImage& input, F32 threshold = 200) -> RawCPUImage;
	inline auto GradientMagnitude(ThreadPool<>& threadPool, const RawCPUImage& input, F32 threshold = 150) -> RawCPUImage;
}


namespace PA
{
	template<typename TF, U32 Channels, U32 Dim0, U32 Dim1, B UniformPerChannel>
	template<typename... TArgs>
	inline constexpr Kernel<TF, Channels, Dim0, Dim1, UniformPerChannel>::Kernel(TArgs... args) :
		data({ args... })
	{
	}


	template<typename TKernel>
	auto Convolute(ThreadPool<>& threadPool, const TKernel& kernel, const RawCPUImage& input) -> RawCPUImage
	{
		// TODO: Handle more output formats
		RawCPUImage result(input.width, input.height, EFormat::A32Float, true);
		PA_ASSERT(input.lebesgueOrdered);

		auto extentSize = input.lebesgueStride * input.lebesgueStride;
		auto tasksCount = threadPool.GetMaxTasks();
		auto pixelsPerTask = extentSize / tasksCount;

		auto task =
		[&] (U32 start, U32 end)
		{
			for (auto i = start; i < end; ++i)
			{
				auto coords = LebesgueCurveInverse(i);
				I32 offsetX = (kernel.dimension0 - 1) / 2;
				I32 offsetY = (kernel.dimension1 - 1) / 2;
				typename TKernel::Scalar accumulator(0);
				for (auto j = 0u; j < kernel.dimension1; ++j)
				{
					for (auto k = 0u; k < kernel.dimension0; ++k)
					{
						auto x = -offsetX + k + coords.first;
						auto y = -offsetY + j + coords.second;

						typename TKernel::Scalar value(0);
						if (x >= 0 && y >= 0 && x < input.width && y < input.height)
						{
							auto idx = LebesgueCurve(x, y);
							// TODO: Handle multiple formats.
							PA_ASSERT(input.format == EFormat::A32Float || input.format == EFormat::A8);
							if (input.format == EFormat::A8)
							{
								value = input.data[idx];
							}
							else if (input.format == EFormat::A32Float)
							{
								value = ((F32*)input.data.data())[idx];
							}
						}
						accumulator += value * kernel.data[j * kernel.dimension0 + k];
					}
				}
				((F32*)result.data.data())[i] = accumulator;
			}
		};

		Array<TaskResult<V>> results;
		for (auto i = 0u; i < tasksCount; ++i)
		{
			results.emplace_back(threadPool.AddTask(task, i * pixelsPerTask, (i + 1) * pixelsPerTask));
		}
		task(pixelsPerTask * tasksCount, extentSize);

		for (auto& res : results)
		{
			res.Retrieve();
		}

		return result;
	}

	inline auto SobelEdgeDetect(ThreadPool<>& threadPool, const RawCPUImage& input, F32 threshold) -> RawCPUImage
	{
		PA_ASSERT(input.lebesgueOrdered);
		RawCPUImage result(input.width, input.height, input.format, true);

		auto gX = Convolute(threadPool, SobelX<F32, 1>, input);
		auto resultF = Convolute(threadPool, SobelY<F32, 1>, gX);

		auto extentSize = input.lebesgueStride * input.lebesgueStride;
		auto tasksCount = threadPool.GetMaxTasks();
		auto pixelsPerTask = extentSize / tasksCount;

		auto task =
		[&](U32 start, U32 end)
		{
			for (auto i = start; i < end; ++i)
			{
				auto coords = LebesgueCurveInverse(i);
				if (coords.first >= input.width || coords.second >= input.height)
				{
					continue;
				}
				// TODO: Handle more formats.
				result.data[i] = (((F32*) resultF.data.data())[i] < threshold)? 255 : 0;
			}
		};

		Array<TaskResult<V>> results;
		for (auto i = 0u; i < tasksCount; ++i)
		{
			results.emplace_back(threadPool.AddTask(task, i * pixelsPerTask, (i + 1) * pixelsPerTask));
		}
		task(pixelsPerTask * tasksCount, extentSize);

		for (auto& res : results)
		{
			res.Retrieve();
		}

		return result;
	}

	inline auto GradientMagnitude(ThreadPool<>& threadPool, const RawCPUImage& input, F32 threshold) -> RawCPUImage
	{
		PA_ASSERT(input.lebesgueOrdered);
		RawCPUImage result(input.width, input.height, input.format, true);

		auto gX = Convolute(threadPool, SobelX<F32, 1>, input);
		auto gY = Convolute(threadPool, SobelY<F32, 1>, input);

		auto extentSize = input.lebesgueStride * input.lebesgueStride;
		auto tasksCount = threadPool.GetMaxTasks();
		auto pixelsPerTask = extentSize / tasksCount;

		auto task =
		[&] (U32 start, U32 end)
		{
			for (auto i = start; i < end; ++i)
			{
				auto coords = LebesgueCurveInverse(i);
				if (coords.first >= input.width || coords.second >= input.height)
				{
					continue;
				}
				// TODO: Handle more formats.
				auto gXV = ((F32*)gX.data.data())[i];
				auto gYV = ((F32*)gY.data.data())[i];
				//result.data[i] = ClampedU8(Sqrt(gXV * gXV + gYV * gYV));
				auto gradMag = Sqrt(gXV * gXV + gYV * gYV);
				result.data[i] = (gradMag < threshold) ? gradMag : 255;
			}
		};

		Array<TaskResult<V>> results;
		for (auto i = 0u; i < tasksCount; ++i)
		{
			results.emplace_back(threadPool.AddTask(task, i * pixelsPerTask, (i + 1) * pixelsPerTask));
		}
		task(pixelsPerTask * tasksCount, extentSize);

		for (auto& res : results)
		{
			res.Retrieve();
		}

		return result;		
	}
}