#define _CRT_SECURE_NO_WARNINGS

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <string.h>
#include <assert.h>
#include <ctime>

#define CMD_STOP     (int) -1
#define CMD_CONTINUE (int) -2
#define SIDE         (int)  256
#define SIZE         (int)  (SIDE * SIDE)
#define TEM_T        (float)200
#define TEM_B        (float)400
#define TEM_L        (float)100
#define TEM_R        (float)300

#define MAX_DIFFERENCE (float) 0.001f

struct SDL_Context
{
    SDL_Renderer* Renderer;
    SDL_Window* Window;
};

struct Task
{
    int xBegin;
    int xEnd;
    int xRange;
};

int Get2D(const int x, const int y)
{
    return y + x * SIDE;
}

Task GetTask(const int rank, const int size)
{
    const int part = SIDE / size;
    Task t;
    t.xBegin = rank == 0 ? 1 : part * rank;
    t.xEnd = rank == (size - 1) ? SIDE-1 : part * (rank + 1);
    t.xRange = part;

    return t;
}

float* InitializeMatrix()
{
    float* matrix = (float*)calloc(SIZE, sizeof(*matrix));

    for (int x = 0; x < SIDE; ++x)
    {
        matrix[Get2D(x, 0)] = TEM_T;
        matrix[Get2D(x, SIDE - 1)] = TEM_B;
    }

    for (int y = 0; y < SIDE; ++y)
        matrix[Get2D(0, y)] = TEM_L;

    for (int y = 0; y < SIDE; ++y)
        matrix[Get2D(SIDE-1, y)] = TEM_R;

    return matrix;
}

Task* GetSlaveTasks(const int slavesCount)
{
    Task* tasks = (Task*)calloc(slavesCount, sizeof(*tasks));
    for (int i = 0; i < slavesCount; ++i)
        tasks[i] = GetTask(i, slavesCount);

    return tasks;
}

float abs(float value)
{
    return value < 0 ? -1 * value : value;
}

float CalculateLocalDifference(const float* old_matrix, const float* new_matrix, const Task* task)
{
    float diff = 0;

    for (int x = task->xBegin; x < task->xEnd; ++x)
        for (int y = 1; y < SIDE-1; ++y)
        {
            
            diff += abs(new_matrix[Get2D(x, y)] - old_matrix[Get2D(x, y)]);
        }

    diff /= SIZE;

    return diff;
}

void PrintMatrix(float* matrix)
{
    for (int y = 0; y < SIDE; ++y)
    {
        for (int x = 0; x < SIDE; ++x)
        {
            printf("%f ", matrix[Get2D(x, y)]);
        }
        printf("\n");
    }
    printf("================================\n");
    fflush(stdout);
}

void CalculateHeatExchange(const Task* task, float* new_matrix, const float* old_matrix)
{
    for (int x = task->xBegin; x < task->xEnd; ++x)
        for (int y = 1; y < SIDE - 1; ++y)
        {
            new_matrix[Get2D(x, y)] = (old_matrix[Get2D(x - 1, y)] + old_matrix[Get2D(x + 1, y)] + old_matrix[Get2D(x, y - 1)] + old_matrix[Get2D(x, y + 1)]) / 4;
        }
}

void ProcessSlave(const int rank, const int size)
{
    const int nSlave = rank - 1;
    const int slavesCount = size - 1;
    
    Task t = GetTask(nSlave, slavesCount);

    float* old_matrix = (float*)calloc(SIZE, sizeof(*old_matrix));
    float* new_matrix = (float*)calloc(SIZE, sizeof(*new_matrix));

    int cmd = CMD_STOP;
    for (;;)
    {
        MPI_Recv(&cmd, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (cmd == CMD_STOP)
            break;
        else if (cmd != CMD_CONTINUE)
            assert(!"D:<");

        MPI_Recv(old_matrix, SIZE, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        for (int i = 0; i < SIZE; ++i)
            new_matrix[i] = old_matrix[i];

        CalculateHeatExchange(&t, new_matrix, old_matrix);

        const float difference = CalculateLocalDifference(old_matrix, new_matrix, &t);
        MPI_Send(&difference, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);

        const size_t offset = Get2D(t.xBegin, 0);
        MPI_Send(new_matrix + offset, t.xRange * SIDE, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
    }

    free(old_matrix);
    free(new_matrix);
}

void DumpMatrix(const float* matrix)
{
    FILE* ff = fopen("kiki", "a");
    
    for (int y = 0; y < SIDE; ++y)
    {
        for (int x = 0; x < SIDE; ++x)
        {
            fprintf(ff, "%.2f ", matrix[Get2D(x, y)]);
        }
        fprintf(ff, "\n");
    }
    fclose(ff);
}

void DrawNewFrame(SDL_Context* context, const float* matrix)
{
    SDL_RenderClear(context->Renderer);
    for(int x = 0; x < SIDE; ++x)
        for (int y = 0; y < SIDE; ++y)
        {
            float value = matrix[Get2D(x, y)];
            value = value > 255 ? 255 : value;
            SDL_SetRenderDrawColor(context->Renderer, value, 0, value, 255);
            SDL_RenderDrawPoint(context->Renderer, x, y);
        }

    SDL_RenderPresent(context->Renderer);
}

void ProcessMaster(const int rank, const int size, SDL_Context* context)
{
    double t1 = MPI_Wtime();

    const int slavesCount = size - 1;
    Task* slavesTasks = GetSlaveTasks(slavesCount);

    float* matrix = InitializeMatrix();
    float* differences = (float*)calloc(slavesCount, sizeof(*differences));

    float difference = 1;
    while (difference > MAX_DIFFERENCE)
    {
        DrawNewFrame(context, matrix);
        difference = 0;
        for (int i = 0; i < slavesCount; ++i)
        {
            const int cmd = CMD_CONTINUE;
            MPI_Send(&cmd, 1, MPI_INT, i+1, 0, MPI_COMM_WORLD);
            MPI_Send(matrix, SIZE, MPI_FLOAT, i+1, 0, MPI_COMM_WORLD);
        }
        
        for (int i = 0; i < slavesCount; ++i)
        {
            MPI_Recv(&differences[i], 1, MPI_FLOAT, i+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            const size_t offset = Get2D(slavesTasks[i].xBegin, 0);
            MPI_Recv(matrix + offset, slavesTasks[i].xRange * SIDE, MPI_FLOAT, i+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        for (int i = 0; i < slavesCount; ++i)
            difference += differences[i];
    }
    const int cmd = CMD_STOP;
    for (int i = 0; i < slavesCount; ++i)
        MPI_Send(&cmd, 1, MPI_INT, i+1, 0, MPI_COMM_WORLD);

    double dt = MPI_Wtime() - t1;
    printf("calculated for: %lf\n", dt);
}

int main(int argc, char **argv)
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank != 0)
        ProcessSlave(rank, size);
    else
    {
        SDL_Context context;
        SDL_CreateWindowAndRenderer(SIDE, SIDE, 0, &context.Window, &context.Renderer);

        ProcessMaster(rank, size, &context);
    }
    
    MPI_Finalize();

    return 0;
}

