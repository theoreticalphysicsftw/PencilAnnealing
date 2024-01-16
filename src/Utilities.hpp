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

#include <cstring>
#include <cstdlib>

#include <algorithm>

#include "Types.hpp"
#include "Concepts.hpp"

namespace PA
{
	template <typename T>
	inline auto SizeInBytes(const Array<T>& array) -> U64;
	inline auto StringLength(const C* cstr) -> U32;
	template <typename T>
	inline auto ToString(const T& v) -> Str;
	inline auto Terminate() -> V;

	template <class TContainer>
	inline auto Fill(TContainer& c, const typename TContainer::value_type& value) -> V;
	
	template <typename T>
		requires CIsArithmetic<T>
	inline auto ClampedU8(T v) -> U8;

	template <typename T>
	inline auto Max(const T& v0, const T& v1);
}


namespace PA
{
	template <typename T>
	inline auto SizeInBytes(const Array<T>& array) -> U64
	{
		return array.size() * sizeof(T);
	}


	inline auto StringLength(const C* cstr) -> U32
	{
		return U32(strlen(cstr));
	}

	template<typename T>
	inline auto ToString(const T& v) -> Str
	{
		return std::to_string(v);
	}

	inline auto Terminate() -> V
	{
		std::abort();
	}

	template <typename TContainer>
	inline auto Fill(TContainer& c, const typename TContainer::value_type& value) -> V
	{
		std::fill(c.begin(), c.end(), value);
	}

	template<typename T>
		requires CIsArithmetic<T>
	inline auto ClampedU8(T v) -> U8
	{
		return U8(std::max(T(0), std::min(v, T(0xFF))));
	}

	template <typename T>
	inline auto Max(const T& v0, const T& v1)
	{
		return std::max(v0, v1);
	}

	template <typename T>
	inline auto Min(const T& v0, const T& v1)
	{
		return std::min(v0, v1);
	}
}