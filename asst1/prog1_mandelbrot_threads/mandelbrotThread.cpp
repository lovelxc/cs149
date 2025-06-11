#include <algorithm>
#include <stdio.h>
#include <thread>

#include "CycleTimer.h"

typedef struct {
    float x0, x1;
    float y0, y1;
    unsigned int width;
    unsigned int height;
    int maxIterations;
    int *output;
    int threadId;
    int numThreads;
} WorkerArgs;

extern void mandelbrotSerial(float x0, float y0, float x1, float y1, int width,
                             int height, int startRow, int numRows,
                             int maxIterations, int output[]);

//
// workerThreadStart --
//
// Thread entrypoint.
void workerThreadStart(WorkerArgs *const args) {

    // TODO FOR CS149 STUDENTS: Implement the body of the worker
    // thread here. Each thread should make a call to mandelbrotSerial()
    // to compute a part of the output image.  For example, in a
    // program that uses two threads, thread 0 could compute the top
    // half of the image and thread 1 could compute the bottom half.
    double t_startTime = CycleTimer::currentSeconds();
    printf("Hello world from thread %d\n", args->threadId);
    // 一开始写的，观察mandel函数可以发现，每行的计算量其实是不一样的，起始行越大，计算时间越长
    // 因此要将任务均分给三个线程，也就是切成条，而不是直接切成块
    /*
    int max_rows = args->height / args->numThreads;
    int nums_rows = max_rows;
    if (args->threadId == args->numThreads - 1) {
        // Last thread takes the remaining rows
        nums_rows = args->height -
                    (args->numThreads - 1) * (args->height / args->numThreads);
    }
    mandelbrotSerial(args->x0, args->y0, args->x1, args->y1, args->width,
                     args->height, args->threadId * max_rows, nums_rows,
                     args->maxIterations, args->output);
    */
    const unsigned int CHUNK_SIZE = 16; // 或 8、32 等，需要测试最优值
    // 每次处理多行，减少函数调用开销
    for (unsigned int cur_row = args->threadId * CHUNK_SIZE;
         cur_row < args->height; cur_row += args->numThreads * CHUNK_SIZE) {
        // 确保不会越界
        int numRows = std::min(CHUNK_SIZE, args->height - cur_row);
        mandelbrotSerial(args->x0, args->y0, args->x1, args->y1, args->width,
                         args->height, cur_row, numRows, args->maxIterations,
                         args->output);
    }

    double t_endTime = CycleTimer::currentSeconds();
    printf("Thread %d: [%.3f] ms\n", args->threadId,
           (t_endTime - t_startTime) * 1000);
}

//
// MandelbrotThread --
//
// Multi-threaded implementation of mandelbrot set image generation.
// Threads of execution are created by spawning std::threads.
void mandelbrotThread(int numThreads, float x0, float y0, float x1, float y1,
                      int width, int height, int maxIterations, int output[]) {
    static constexpr int MAX_THREADS = 32;

    if (numThreads > MAX_THREADS) {
        fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
        exit(1);
    }

    // Creates thread objects that do not yet represent a thread.
    std::thread workers[MAX_THREADS];
    WorkerArgs args[MAX_THREADS];

    for (int i = 0; i < numThreads; i++) {

        // TODO FOR CS149 STUDENTS: You may or may not wish to modify
        // the per-thread arguments here.  The code below copies the
        // same arguments for each thread
        args[i].x0 = x0;
        args[i].y0 = y0;
        args[i].x1 = x1;
        args[i].y1 = y1;
        args[i].width = width;
        args[i].height = height;
        args[i].maxIterations = maxIterations;
        args[i].numThreads = numThreads;
        args[i].output = output;

        args[i].threadId = i;
    }

    // Spawn the worker threads.  Note that only numThreads-1 std::threads
    // are created and the main application thread is used as a worker
    // as well.
    for (int i = 1; i < numThreads; i++) {
        workers[i] = std::thread(workerThreadStart, &args[i]);
    }

    workerThreadStart(&args[0]);

    // join worker threads
    for (int i = 1; i < numThreads; i++) {
        workers[i].join();
    }
}
