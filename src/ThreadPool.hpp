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