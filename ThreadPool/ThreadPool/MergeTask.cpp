#include "MergeTask.h"
#include "common.h"

#include <thread>

MergeTask::MergeTask(int* buf, const int lb, const int ll, const int rb, const int rl)
    : Buf(buf)
    , Lb(lb)
    , Ll(ll)
    , Rb(rb)
    , Rl(rl)
{
}

void MergeTask::operator()()
{
    Merge(Buf, Lb, Ll, Rb, Rl);
}