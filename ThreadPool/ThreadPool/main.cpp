#include "threadpool.h"
#include "MergeSortTask.h"
#include "MergeTask.h"
#include "common.h"

#include <chrono>

#include <iostream>
#include <cstring>
#include <ctime>
#include <cmath>
#include <fstream>
#include <string>
#include <intrin.h>


void PrintArray(const int* buf, const size_t size, const int delimiterPos=-1)
{
    for (int i = 0; i < size; ++i)
    {
        std::printf("%d ", buf[i]);
        if (i == delimiterPos && i >= 0)
            std::printf("#");
    }
    std::printf("\n\n");
}

unsigned long long MergeSortBench(const int N)
{

    int* buf = new int[N];

    for (int i = 0; i < N; ++i)
        buf[i] = std::rand();

    
    unsigned long long t1 = __rdtsc();
    MergeSort(buf, 0, N);
    unsigned long long dt = __rdtsc() - t1;


    return dt;
}

unsigned long long ParallelMergeSortBench(ThreadPool& tp, const int nThreads, const int N)
{
    int* buf = new int[N];

    for (int i = 0; i < N; ++i)
        buf[i] = std::rand();

    State* states = new State[nThreads];

    unsigned long long t1 = __rdtsc();
    const int part = N / nThreads;
    for (int i = 0; i < nThreads; ++i)
    {
        const int first = i * part;
        const int last = (i == nThreads - 1) ? N : (i + 1) * part;

        MergeSortTask mst{ buf, first, last };

        states[i] = tp.AddTask(mst);
    }

    for (int i = 0; i < nThreads; ++i)
        states[i]->Wait();
    delete[] states;

    const int h = static_cast<int>(std::log2(nThreads));
    int rl = 0;
    for (int i = 0; i < h; ++i)
    {
        const int mpart = part * static_cast<int>(std::pow(2, i));
        const int mergesCount = nThreads / (2 * (i + 1));

        states = new State[mergesCount];

        for (int k = 0; k < mergesCount; ++k)
        {
            const int lb = 2 * mpart * k;
            const int ll = lb + mpart;
            const int rb = ll;
            rl = (k == mergesCount - 1) && (nThreads % 2 == 0) ? N : rb + mpart;

            MergeTask mt{ buf, lb, ll, rb, rl };
            states[k] = tp.AddTask(mt);
        }

        for (int k = 0; k < mergesCount; ++k)
            states[k]->Wait();

        delete[] states;
    }

    if (nThreads % 2 != 0)
        Merge(buf, 0, rl, rl, N);

    unsigned long long dt = __rdtsc() - t1;

    delete[] buf;

    return dt;
}

const char* Help = "usage: ./{exe} {threads number:int}";

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::printf("%s\n", Help);
        return -1;
    }
    
    const int nThreads = std::atoi(argv[1]);

    ThreadPool tp(nThreads);
    
    std::ofstream f((std::to_string(nThreads) + ".csv").c_str());

    for (int i = 7; i < 27; ++i)
    {
        const int dataSize = 1 << i;
        unsigned long long dt = 0;
        if (nThreads == 1)
            dt = MergeSortBench(dataSize);
        else
            dt = ParallelMergeSortBench(tp, nThreads, dataSize);
        
        std::printf("data:%d %d %llu\n",dataSize, nThreads, dt);
        //f <<dataSize<< " " << dt << "\n";
        f << dt << "\n";
    }
}