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
	inline auto Max(const T& v0, const T& v1) -> T;

	template <typename T>
	inline auto Min(const T& v0, const T& v1) -> T;

	inline auto InterleaveBits(U16 n) -> U32;
	inline auto DeinterleaveBits(U32 n) -> U16;
	inline auto LebesgueCurve(U16 x, U16 y) -> U32;
	inline auto LebesgueCurveInverse(U32 n) -> Pair<U16, U16>;
	
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
	inline auto Max(const T& v0, const T& v1) -> T
	{
		return std::max(v0, v1);
	}


	template <typename T>
	inline auto Min(const T& v0, const T& v1) -> T
	{
		return std::min(v0, v1);
	}


	auto InterleaveBits(U16 n) -> U32
	{
		U32 n32 = n;
		n32 = (n32 | (n32 << 8)) & 0b00000000111111110000000011111111u;
		n32 = (n32 | (n32 << 4)) & 0b00001111000011110000111100001111u;
		n32 = (n32 | (n32 << 2)) & 0b00110011001100110011001100110011u;
		n32 = (n32 | (n32 << 1)) & 0b01010101010101010101010101010101u;
		return n32;
	}

	auto DeinterleaveBits(U32 n) -> U16
	{
		n = (n | (n >> 1)) & 0b00110011001100110011001100110011u;
		n = (n | (n >> 2)) & 0b00001111000011110000111100001111u;
		n = (n | (n >> 4)) & 0b00000000111111110000000011111111u;
		n = (n | (n >> 8)) & 0b00000000000000001111111111111111u;
		return U16(n);
	}


	inline auto LebesgueCurve(U16 x, U16 y) -> U32
	{
		auto r0 = InterleaveBits(x);
		auto r1 = InterleaveBits(y);
		return r0 | (r1 << 1u);
	}


	inline auto LebesgueCurveInverse(U32 n) -> Pair<U16, U16>
	{
		Pair<U16, U16> result;
		auto x = n & 0b01010101010101010101010101010101u;
		auto y = (n - x) >> 1;
		result.first = DeinterleaveBits(x);
		result.second = DeinterleaveBits(y);
		return result;
	}
}