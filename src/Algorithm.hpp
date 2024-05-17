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

#include <algorithm>

namespace PA
{
	template <typename TInputIt, typename T>
	inline auto Find(TInputIt first, TInputIt last, const T& v) -> TInputIt;

	template <class TContainer>
	inline auto Fill(TContainer& c, const typename TContainer::value_type& value) -> V;

	template <typename T>
	inline auto GenerateSequence(T first, T last) -> Array<T>;

	template <typename TContainer, typename TComp>
	inline auto Sort(TContainer& c, TComp comp) -> V;
}


namespace PA
{
	template<typename TInputIt, typename T>
	inline auto Find(TInputIt first, TInputIt last, const T& v) -> TInputIt
	{
		return std::find(first, last, v);
	}


	template <typename TContainer>
	inline auto Fill(TContainer& c, const typename TContainer::value_type& value) -> V
	{
		std::fill(c.begin(), c.end(), value);
	}


	template<typename T>
	auto GenerateSequence(T first, T last) -> Array<T>
	{
		Array<T> result;

		for (auto i = first; i < last; ++i)
		{
			result.emplace_back(i);
		}

		return result;
	}


	template<typename TContainer, typename TComp>
	auto Sort(TContainer& c, TComp comp) -> V
	{
		std::sort(c.begin(), c.end(), comp);
	}

}