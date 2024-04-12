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

#include <cstdint>

#include <vector>
#include <span>
#include <array>
#include <deque>
#include <unordered_map>
#include <unordered_set>

#include <string>

#include <functional>

#include <variant>
#include <utility>
#include <type_traits>

#include <thread>
#include <atomic>
#include <mutex>
#include <future>
#include <semaphore>

#include <memory>

namespace PA
{
    using U8 = uint8_t;
    using U16 = uint16_t;
    using U32 = uint32_t;
    using U64 = uint64_t;

    using I8 = int8_t;
    using I16 = int16_t;
    using I32 = int32_t;
    using I64 = int64_t;

    using F32 = float;
    using F64 = double;

    using C = char;
    using Byte = uint8_t;
    using B = bool;
    using V = void;
    using Void = V;

    template <typename T>
    using Array = std::vector<T>;

    template <typename T, U64 size>
    using StaticArray = std::array<T, size>;

    template <typename T>
    using ChunkArray = std::deque<T>;

    template <typename T>
    using Span = std::span<T>;

    template <typename T>
    using Deque = std::deque<T>;

    template <typename K, typename V>
    using Map = std::unordered_map<K, V>;

    template <typename K>
    using Set = std::unordered_set<K>;

    template<typename F, typename S>
    using Pair = std::pair<F, S>;

    using Str = std::string;

    template <typename T>
    using Function = std::function<T>;

    template<typename TF, typename... TArgs >
    using InvokeResult = std::invoke_result_t<TF, TArgs...>;

    template <typename TF, typename... TArgs>
    constexpr auto& Invoke = std::invoke<TF, TArgs...>;

    template <typename... Ts>
    using Variant = std::variant<Ts...>;

    template <U32 size>
    struct StorageType
    {
        StaticArray<Byte, size> data;
    };

    template <typename T>
    using RemoveReference = std::remove_reference_t<T>;

    template <typename T, typename U>
    constexpr auto IsSameType = std::is_same<T, U>::value;

    template <B Condition, typename T, typename F>
    using Conditional = std::conditional_t<Condition, T, F>;

    template <typename T>
    [[nodiscard]]
    constexpr T&& Forward(RemoveReference<T>&& t) noexcept
    {
        return std::forward<T>(t);
    }

    template <typename T>
    [[nodiscard]]
    constexpr T&& Forward(RemoveReference<T>& t) noexcept
    {
        return std::forward<T>(t);
    }

    template <typename T>
    typename RemoveReference<T> Move(T&& t)
    {
        return std::move(t);
    }

    using Thread = std::jthread;
    using Mutex = std::mutex;

    template <typename T>
    using Atomic = std::atomic<T>;

    template <typename T>
    using ScopedLock = std::scoped_lock<T>;

    template <typename T>
    using Future = std::future<T>;
    
    template <typename T>
    using Promise = std::promise<T>;

    using Semaphore = std::counting_semaphore<>;

    constexpr auto& GetLogicalCPUCount = std::thread::hardware_concurrency;

    template <typename T>
    using SharedPtr = std::shared_ptr<T>;

    template <typename T>
    constexpr auto MakeShared = std::make_shared<T>;
}
