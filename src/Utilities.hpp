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
#include <bit>

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

	template <typename TFrom, typename TTo>
	inline auto MemCopy(Span<const TFrom> from, TTo* to) -> V;
	
	template <typename T>
		requires CIsArithmetic<T>
	inline auto ClampedU8(T v) -> U8;

	template <typename T>
	inline auto Max(const T& v0, const T& v1) -> T;

	template <typename T>
	inline auto Min(const T& v0, const T& v1) -> T;

	template <typename T>
	inline auto Swap(T& v0, T& v1) -> V;

	inline auto RoundToPowerOfTwo(U32 x) -> U32;
	inline auto InterleaveBits(U16 n) -> U32;
	inline auto DeinterleaveBits(U32 n) -> U16;
	inline auto LebesgueCurve(U16 x, U16 y) -> U32;
	inline auto LebesgueCurveInverse(U32 n) -> Pair<U16, U16>;

	template <typename T>
	inline auto FromLE(T x) -> T;
	template <typename T>
	inline auto FromBE(T x) -> T;

	template <typename TWord = U64>
	class DynamicBitsetBase
	{
		Array<TWord> data;

	public:
		DynamicBitsetBase(U32 size = 0);
		auto Expand(U32 size) -> V;
		auto SetBitUnsafe(U32 idx) -> V;
		auto ClearBitUnsafe(U32 idx) -> V;
		auto GetBitUnsafe(U32 idx) -> B;
	};

	using DynamicBitset = DynamicBitsetBase<>;
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

	template<typename TFrom, typename TTo>
	auto MemCopy(Span<const TFrom> from, TTo* to) -> V
	{
		std::memcpy((V*)to, (const V*)from.data(), from.size() * sizeof(TFrom));
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


	template<typename T>
	auto Swap(T& v0, T& v1) -> V
	{
		std::swap(v0, v1);
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


	inline auto RoundToPowerOfTwo(U32 x) -> U32
	{
		x--;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		x++;
		return x;
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

	template <typename T>
	auto ByteSwap(T x) -> T
	{
		T result;
		auto bytes = (Byte*)&x;

		for (auto i = 0u; i < sizeof(T); ++i)
		{
			result[i] = bytes[sizeof(T) - 1 - i];
		}

		return result;
	}

	template <typename T>
		requires (sizeof(T) == 2)
	auto ByteSwap(T x) -> T
	{
		T result;
		auto uXPtr = (U16*)&x;
		*((U16*)&result) = (*uXPtr << 8) | ((*uXPtr) >> 8);
		return result;
	}

	template <typename T>
		requires (sizeof(T) == 4)
	auto ByteSwap(T x) -> T
	{
		T result;
		auto uXPtr = (U32*)&x;
		*((U32*)&result) =
			((*uXPtr & 0xFF000000u) >> 24) |
			((*uXPtr & 0x00FF0000u) >> 8) |
			((*uXPtr & 0x0000FF00u) << 8) |
			((*uXPtr & 0x000000FFu) << 24);
		return result;
	}

	template <typename T>
		requires (sizeof(T) == 8)
	auto ByteSwap(T x) -> T
	{
		T result;
		auto uXPtr = (U64*)&x;
		*((U64*)&result) =
			((*uXPtr & 0xFF00000000000000ull) >> 56) |
			((*uXPtr & 0x00FF000000000000ull) >> 40) |
			((*uXPtr & 0x0000FF0000000000ull) >> 24) |
			((*uXPtr & 0x000000FF00000000ull) >> 8) |
			((*uXPtr & 0x00000000FF000000ull) << 8) |
			((*uXPtr & 0x0000000000FF0000ull) << 24) |
			((*uXPtr & 0x000000000000FF00ull) << 40) |
			((*uXPtr & 0x00000000000000FFull) << 56);
		return result;
	}

	template <typename T>
	auto FromBE(T x) -> T
	{
		if constexpr (std::endian::native == std::endian::little)
		{
			return ByteSwap(x);
		}
		return x;
	}

	template <typename T>
	auto FromLE(T x) -> T
	{
		if constexpr (std::endian::native == std::endian::big)
		{
			return ByteSwap(x);
		}
		return x;
	}


	template<typename TWord>
	inline DynamicBitsetBase<TWord>::DynamicBitsetBase(U32 size)
	{
		Expand(size);
	}


	template<typename TWord>
	inline auto DynamicBitsetBase<TWord>::Expand(U32 size) -> V
	{
		auto minSize = (size + sizeof(TWord) - 1) / sizeof(TWord);

		if (data.size() < minSize)
		{
			data.resize(minSize, 0u);
		}
	}

	template<typename TWord>
	inline auto DynamicBitsetBase<TWord>::SetBitUnsafe(U32 idx) -> V
	{
		auto wordIdx = idx / sizeof(TWord);
		auto bitIdx = idx % sizeof(TWord);

		data[wordIdx] |= TWord(1) << bitIdx;
	}


	template<typename TWord>
	inline auto DynamicBitsetBase<TWord>::ClearBitUnsafe(U32 idx) -> V
	{
		auto wordIdx = idx / sizeof(TWord);
		auto bitIdx = idx % sizeof(TWord);

		data[wordIdx] &= ~(TWord(1) << bitIdx);
	}


	template<typename TWord>
	inline auto DynamicBitsetBase<TWord>::GetBitUnsafe(U32 idx) -> B
	{
		auto wordIdx = idx / sizeof(TWord);
		auto bitIdx = idx % sizeof(TWord);

		return B(data[wordIdx] & (~(TWord(1) << bitIdx)));
	}
}