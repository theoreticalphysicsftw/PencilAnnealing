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

#include <cstdint>

#include <vector>
#include <span>
#include <array>
#include <deque>
#include <unordered_map>
#include <unordered_set>

#include <string>
#include <string_view>

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

    template <typename T, typename U>
    constexpr auto MakePair(const T& f, const U& s) -> Pair<T, U>
    {
        return std::make_pair(f, s);
    }

    using Str = std::string;
    using StrView = std::string_view;

    using namespace std::literals::string_view_literals;

    template <typename T>
    using Function = std::function<T>;

    template<typename TF, typename... TArgs >
    using InvokeResult = std::invoke_result_t<TF, TArgs...>;

    template <typename TF, typename... TArgs>
    constexpr auto& Invoke = std::invoke<TF, TArgs...>;

    template <typename... Ts>
    using Variant = std::variant<Ts...>;

    template<typename TR, class TVisitor, class... TVariants >
    inline constexpr TR Visit(TVisitor&& vis, TVariants&&... vars)
    {
        return std::visit(vis, vars...);
    }

    template <typename T, typename... Ts>
    inline constexpr auto HoldsAlternative(const Variant<Ts...>& v) -> B
    {
        return std::holds_alternative<T>(v);
    }

    template <typename T, typename... Ts>
    constexpr auto Get(Variant<Ts...>& v) -> T&
    {
        return std::get<T>(v);
    }

    template <typename T, typename... Ts>
    constexpr auto Get(const Variant<Ts...>& v) -> const T&
    {
        return std::get<T>(v);
    }

    template <U32 size>
    struct StorageType
    {
        StaticArray<Byte, size> data;
    };

    template <typename T>
    using RemoveReference = std::remove_reference_t<T>;

    template <typename T>
    using RemovePointer = std::remove_pointer_t<T>;

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
    RemoveReference<T> Move(T&& t)
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

    inline constexpr auto& GetLogicalCPUCount = std::thread::hardware_concurrency;

    template <typename T>
    using SharedPtr = std::shared_ptr<T>;

    template <typename T>
    constexpr auto MakeShared = std::make_shared<T>;
}
