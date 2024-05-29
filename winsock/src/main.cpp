#include<bits/stdc++.h>
#include"threadPool.h"
#include"synchapi.h"

void taskFunc(void* args)
{
    int* num = (int*)args;
    printf("thread %ld is working, number = %d\n", pthread_self(), *num);
    Sleep(1000);
}

int main()
{
    ThreadPool* pool = threadPoolCreate(3,10,100);
    for (size_t i = 0; i < 100; i++)
    {
        int* num = (int*)malloc(sizeof(int));
        *num = i + 100;
        threadPoolAdd(pool, taskFunc, num);
    }
    Sleep(20000);
    threadPoolDestroy(pool);
    return 0;
}
