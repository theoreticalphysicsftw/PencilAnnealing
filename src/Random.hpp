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