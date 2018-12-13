#include "common.h"
#include <cstring>

void MergeSort(int* buf, const int first, const int last)
{
    if ((last - first) > 1)
    {
        const int middle = (last - first) / 2;

        const int lb = first;
        const int ll = first + middle;

        const int rb = ll;
        const int rl = last;

        MergeSort(buf, lb, ll);
        MergeSort(buf, rb, rl);

        Merge(buf, lb, ll, rb, rl);
    }
}

void Merge(int* buf, const int lb, const int ll, const int rb, const int rl)
{
    const int LSize = ll - lb;
    const int RSize = rl - rb;

    int* L = new int[LSize];
    int* R = new int[RSize];

    std::memcpy(L, buf + lb, LSize * sizeof(*buf));
    std::memcpy(R, buf + rb, RSize * sizeof(*buf));

    int li = 0;
    int ri = 0;
    int bi = lb;

    while (li != LSize && ri != RSize)
    {
        if (L[li] < R[ri])
            buf[bi++] = L[li++];
        else
            buf[bi++] = R[ri++];
    }

    if (li != LSize)
    {
        while (li != LSize)
            buf[bi++] = L[li++];
    }

    if (ri != RSize)
    {
        while (ri != RSize)
            buf[bi++] = R[ri++];
    }

    delete[] L;
    delete[] R;
}