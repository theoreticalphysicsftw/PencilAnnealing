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

#include "Bezier.hpp"
#include "Color.hpp"
#include "Utilities.hpp"

namespace PA
{
	template <typename TF>
	class Annealer
	{
	public:
		using Scalar = TF;
		using Vec = Vector<Scalar, 2>;
		using QuadraticBezier = QuadraticBezier<Scalar, 2>;

		RawCPUImage grayscaleReference;
		RawCPUImage currentApproximation;
		RawCPUImage workingApproximation;

		Annealer(const RawCPUImage* referance);
		auto CopyCurrentApproximationToColor(ColorU32* data, U32 stride) -> V;

	private:
		auto ToScreenSpaceCoordinates(Vec in) -> Vec;
		auto ToScreenSpaceCoordinates(Span<Vec> in) -> V;
		auto DrawBezierToSurface(const QuadraticBezier& b, RawCPUImage& img) -> Pair<U32, U32>;
	};
}


namespace PA
{
	template<typename TF>
	inline Annealer<TF>::Annealer(const RawCPUImage* reference) :
		grayscaleReference(reference->width, reference->height, EFormat::A8, true),
		currentApproximation(reference->width, reference->height, EFormat::A8, true),
		workingApproximation(reference->width, reference->height, EFormat::A8, true)
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

		DrawBezierToSurface(QuadraticBezier(Vec(0.4, 0.4), Vec(0.6, 0.4), Vec(0.6, 0.6)), currentApproximation);
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
	inline auto Annealer<TF>::ToScreenSpaceCoordinates(Vec in) -> Vec
	{
		Vec result;
		result[0] = in[0] * (grayscaleReference.width - 1);
		result[1] = (TF(1) - in[1]) * (grayscaleReference.height - 1);
		return result;
	}


	template<typename TF>
	inline auto Annealer<TF>::ToScreenSpaceCoordinates(Span<Vec> in) -> V
	{
		for (auto& vec : in)
		{
			vec = ToScreenSpaceCoordinates(vec);
		}
	}


	template<typename TF>
	inline auto Annealer<TF>::DrawBezierToSurface(const QuadraticBezier& curve, RawCPUImage& img) -> Pair<U32, U32>
	{
		auto bBox = curve.GetBBox();
		auto lowerLeft = ToScreenSpaceCoordinates(bBox.lower);
		auto upperRight = ToScreenSpaceCoordinates(bBox.upper);
		auto lowerRight = Vec(upperRight[0], lowerLeft[1]);
		auto upperLeft = Vec(lowerLeft[0], upperRight[1]);
		auto minIdx = LebesgueCurve(upperLeft[0], upperLeft[1]);
		auto maxIdx = LebesgueCurve(lowerRight[0], lowerRight[1]);

		auto screenCurve = curve;
		ToScreenSpaceCoordinates(screenCurve.points);

		for (auto i = minIdx; i <= maxIdx; ++i)
		{
			auto coords = LebesgueCurveInverse(i);
			auto dist = screenCurve.GetDistanceFrom(Vec(coords.first, coords.second));
			auto val = SmoothStep(TF(0), TF(1), dist / 2.f);
			img.data[i] = U8(TF(255) * val);
		}

		return Pair<U32, U32>(minIdx, maxIdx);
	}
}