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

#include "Types.hpp"

namespace PA
{
	template <typename T>
	struct TaskResult
	{
		auto Retrieve() -> T;
		Future<T> future;
	};

	template <typename TTask = Function<V()>>
	class ThreadPool
	{
	public:
		using Task = TTask;

		ThreadPool(U32 numberOfThreads = GetLogicalCPUCount());

		template <typename TFunc, typename... TArgs>
		auto AddTask(TFunc f, TArgs... args) -> TaskResult<InvokeResult<TFunc, TArgs...>>;

		auto GetMaxTasks() -> U32;
		auto ShutDown() -> V;

	private:
		Atomic<B> keepRunning = true;
		Mutex queueMutex;
		Deque<Task> queue;
		Array<Thread> threads;
		Semaphore availableTasks;
	};

}


namespace PA
{
	template<typename T>
	inline auto TaskResult<T>::Retrieve() -> T
	{
		return future.get();
	}


	template<typename TTask>
	inline ThreadPool<TTask>::ThreadPool(U32 numberOfThreads) :
		availableTasks(0)
	{
		for (auto i = 0u; i < numberOfThreads; ++i)
		{
			threads.emplace_back
			(
				[&]()
				{
					while (keepRunning)
					{
						availableTasks.acquire();
						if (!keepRunning)
						{
							break;
						}
						queueMutex.lock();
						auto front = queue.front();
						queue.pop_front();
						queueMutex.unlock();

						Invoke<Task>(Move(front));
					}
				}
			);
		}
	}


	template<typename TTask>
	inline auto ThreadPool<TTask>::GetMaxTasks() -> U32
	{
		return threads.size();
	}

	template<typename TTask>
	inline auto ThreadPool<TTask>::ShutDown() -> V
	{
		keepRunning = false;
		availableTasks.release(threads.size());
	}


	template<typename TTask>
	template<typename TFunc, typename ...TArgs>
	inline auto ThreadPool<TTask>::AddTask(TFunc f, TArgs... args) -> TaskResult<InvokeResult<TFunc, TArgs...>>
	{
		using Result = InvokeResult<TFunc, TArgs...>;
		
		TaskResult<Result> result;

		queueMutex.lock();
		auto promise = MakeShared<Promise<Result>>();
		result.future = promise->get_future();
		queue.emplace_back
		(
			[promiseL = promise, fL = Move(f), ... argsL = Move(args)]()
			{
				if constexpr (IsSameType<Result, V>)
				{
					fL(argsL...);
					promiseL->set_value();
				}
				else
				{
					promiseL->set_value(fL(argsL...));
				}
			}
		);
		queueMutex.unlock();
		availableTasks.release();
		return result;
	}
}