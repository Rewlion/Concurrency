#include "MergeSortTask.h"

#include "common.h"

#include <iostream>

MergeSortTask::MergeSortTask(int* buf, const int first, const int last)
    :Buf(buf)
    , First(first)
    , Last(last)
{
}

void MergeSortTask::operator()()
{
    MergeSort(Buf, First, Last);
}
