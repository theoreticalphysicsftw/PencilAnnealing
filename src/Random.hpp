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

#include <random>

namespace PA
{
	inline static std::random_device GRandomSeed;
	inline static std::mt19937 GMerseneTwister(GRandomSeed());

	template <typename TF>
	auto GetUniformFloat(TF range0 = TF(0), TF range1 = TF(1)) -> TF;

	template <typename TF>
	inline auto GetExponentialFloat(TF lambda = TF(1)) -> TF;

	inline auto GetUniformU32(U32 range0, U32 range1) -> U32;
	inline auto GetUniformBernoulli() -> B;
}


namespace PA
{
	template<typename TF>
	auto GetUniformFloat(TF range0, TF range1) -> TF
	{
		std::uniform_real_distribution<TF> dist(range0, range1);
		return dist(GMerseneTwister);
	}

	template<typename TF>
	auto GetExponentialFloat(TF lambda) -> TF
	{
		std::exponential_distribution<TF> dist(lambda);

		return dist(GMerseneTwister);
	}

	inline auto GetUniformU32(U32 range0, U32 range1) -> U32
	{
		std::uniform_int_distribution<U32> dist(range0, range1);
		return dist(GMerseneTwister);
	}

	inline auto GetUniformBernoulli() -> B
	{
		std::bernoulli_distribution dist(0.5);

		return dist(GMerseneTwister);
	}
}