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
		return us / 1E6;
	}
}