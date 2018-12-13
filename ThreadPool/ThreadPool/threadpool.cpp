#include "threadpool.h"

#include <iostream>

StateData::StateData()
    : IsCompleted(false)
{
}

void StateData::Wait()
{
    std::unique_lock<std::mutex> l(Mutex);
    CompleteCondition.wait(l, [&] {return IsCompleted == true; });

    return;
}

void StateData::Complete()
{
    IsCompleted = true;
    CompleteCondition.notify_all();
}

ThreadPool::ThreadPool()
    : ThreadsCount(std::thread::hardware_concurrency())
    , Threads(ThreadsCount)
{
    InitThreads();
}

ThreadPool::ThreadPool(const int threadsCount)
    : ThreadsCount(threadsCount)
    , Threads(ThreadsCount)
{
    InitThreads();
}

ThreadPool::~ThreadPool()
{
    StopExecutionFlag = true;

    GotTasksCondition.notify_all();

    for (auto& t : Threads)
        t.join();
}

State ThreadPool::AddTask(const Task& task)
{
    State s{new StateData};
    StatedTask t;
    t.Execute = task;
    t.State = s;

    std::lock_guard<std::mutex> lg(TaskMutex);
    Tasks.push_back(t);

    GotTasksCondition.notify_one();

    return s;
}

void ThreadPool::ThreadContext()
{
    while (StopExecutionFlag != true)
    {
        std::unique_lock<std::mutex> lm(TaskMutex);
        GotTasksCondition.wait(lm, [&]{return Tasks.empty() == false || StopExecutionFlag; });

        if (StopExecutionFlag)
            return;

        StatedTask task = Tasks.back();
        Tasks.pop_back();
        lm.unlock();

        task.Execute();
        task.State->Complete();
    }
}