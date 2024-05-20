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

#include <chrono>

#include "Types.hpp"


namespace PA
{
	auto GetTimeStampUS() -> F64;
	auto UsToS(F32 us) -> F32;
}


namespace PA
{
	inline auto GetTimeStampUS() -> F64
	{
		static auto staticTimePoint = std::chrono::steady_clock::now();
		auto timePoint = std::chrono::steady_clock::now();
		return F64(std::chrono::duration_cast<std::chrono::nanoseconds>(timePoint - staticTimePoint).count() / 1000.f);
	}

	auto UsToS(F32 us) -> F32
	{
		return us / 1E6f;
	}
}