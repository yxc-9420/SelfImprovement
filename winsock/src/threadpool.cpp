#include "threadpool.h"
#include <bits/stdc++.h>
#include"synchapi.h"

const int NUMBER = 2;

ThreadPool *threadPoolCreate(int min, int max, int queueSize)
{
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    do
    {
        if(NULL == pool)
        {
            printf("malloc threadpool failed...\n");
            break;
        }
        pool->threadIds = (pthread_t*)malloc(sizeof(pthread_t) * max);
        if(NULL == pool->threadIds)
        {
            printf("malloc threadIds failed...\n");
            break;
        }
        memset(pool->threadIds,0,sizeof(pthread_t) * max);
        pool->minNum = min;
        pool->maxNum = max;
        pool->busyNum = 0;
        pool->liveNum = min;
        pool->exitNum = 0;

        if(pthread_mutex_init(&pool->mutexPool,NULL) != 0 ||
        pthread_mutex_init(&pool->mutexBusy,NULL) != 0||
        pthread_cond_init(&pool->notEmpty,NULL) != 0||
        pthread_cond_init(&pool->notFull,NULL)!= 0)
        {
            printf("mutex or condition init failed...\n");
            break;
        }

        //任务队列
        pool->taskQ = (Task*)malloc(sizeof(Task) * queueSize);
        pool->queueCapacity = queueSize;
        pool->queueSize = 0;
        pool->queueFront = 0;
        pool->queueRear = 0;

        pool->shutDown = 0;

        //创建线程
        pthread_create(&pool->managerId, NULL, manager, pool);
        for (size_t i = 0; i < min; i++)
        {
            pthread_create(&pool->threadIds[i], NULL, worker, pool);
        }
        return pool;
    } while (0);
    

    //释放资源
    if(pool && pool->threadIds) free(pool->threadIds);
    if(pool && pool->taskQ) free(pool->taskQ);
    if(pool) free(pool);
    return NULL;
}

int threadPoolDestroy(ThreadPool *pool)
{
    if(NULL == pool)
    {
        return -1;
    }
    //关闭线程池
    pool->shutDown = 1;
    //阻塞回收管理者线程
    pthread_join(pool->managerId,NULL);
    //唤醒阻塞的消费者
    for (size_t i = 0; i < pool->liveNum; ++i);
    {
        pthread_cond_signal(&pool->notEmpty);
    }
    //释放堆内存
    if(pool->taskQ)
    {
        free(pool->taskQ);
    }
    if(pool->threadIds)
    {
        free(pool->threadIds);
    }
    
    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);

    free(pool);
    pool = NULL;
    return 0;
}

void threadPoolAdd(ThreadPool *pool, void (*func)(void *), void * args)
{
    pthread_mutex_lock(&pool->mutexPool);
    while (pool->queueSize == pool->queueCapacity && pool->shutDown)
    {
        //阻塞生产者线程
        pthread_cond_wait(&pool->notFull, & pool->mutexPool);
    }
    if(pool->shutDown)
    {
        pthread_mutex_unlock(&pool->mutexPool);
        return;
    }
    //添加任务
    pool->taskQ[pool->queueRear].function = func;
    pool->taskQ[pool->queueRear].args = args;

    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    pool->queueSize++;

    //唤醒工作线程
    pthread_cond_signal(&pool->notEmpty);

    pthread_mutex_unlock(&pool->mutexPool);
}

int threadPoolBusyNum(ThreadPool *pool)
{
    pthread_mutex_lock(&pool->mutexBusy);
    int busyNum = pool->busyNum;
    pthread_mutex_unlock(&pool->mutexBusy);
    return busyNum;
}

int threadPoolLiveNum(ThreadPool *pool)
{
     pthread_mutex_lock(&pool->mutexPool);
    int liveNum = pool->liveNum;
    pthread_mutex_unlock(&pool->mutexPool);
    return liveNum;
}

void *manager(void *args)
{
    ThreadPool* pool = (ThreadPool*)args;
    while(!pool->shutDown)
    {
        //每隔3秒检测一次
        Sleep(3000);

        //取出线程池中任务数量和当前线程数量
        pthread_mutex_lock(&pool->mutexPool);
        int queueSize = pool->queueSize;
        int liveNum = pool->liveNum;
        pthread_mutex_unlock(&pool->mutexPool);

        pthread_mutex_lock(&pool->mutexBusy);
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutexBusy);
        
        //添加线程
        // 任务的个数 > 存活的线程个数  && 存活的线程数 < 最大线程数
        if(queueSize > liveNum && liveNum < pool->maxNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            int counter = 0;
            for(int i = 0; i < pool->maxNum && counter < NUMBER
                && pool->liveNum < pool->maxNum; ++i)
            {
                if(0 == pool->threadIds[i])
                {
                    pthread_create(&pool->threadIds[i], NULL, worker, pool);
                    counter++;
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }

        //销毁线程
        //忙的线程 * 2 < 存活的线程数 && 存活的线程 > 最小线程数
        if(busyNum * 2 < liveNum && liveNum > pool->minNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum = NUMBER;
            pthread_mutex_unlock(&pool->mutexPool);
            //让工作的线程自杀
            for (size_t i = 0; i < NUMBER; ++i)
            {
                pthread_cond_signal(&pool->notEmpty);
            }
        }
    }
    return NULL;
}

void* worker(void *args)
{
    ThreadPool* pool = (ThreadPool*)args;
    while (1)
    {
        pthread_mutex_lock(&pool->mutexPool);
        //如果当前没有任务
        while (pool->queueSize == 0 && !pool->shutDown)
        {
            //等待，进入等待后，自动释放pool->mutexPool
            pthread_cond_wait(&pool->notEmpty,&pool->mutexPool);
            //当函数返回时，自动获得pool->mutexPool

            //判断是不是要销毁线程
            if(pool->exitNum > 0)
            {
                pool->exitNum--;
                if(pool->liveNum > pool->minNum)
                {
                    pool->liveNum--;
                    pthread_mutex_unlock(&pool->mutexPool);
                    threadExit(pool);
                }
            }
        }
        
        //判断线程池是否被关闭了
        if(pool->shutDown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            threadExit(pool);
        }

        //从任务队列中取出一个任务
        Task task;
        task.function = pool->taskQ[pool->queueFront].function;
        task.args = pool->taskQ[pool->queueFront].args;
        //移动头节点
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->queueSize--;
        //唤醒生产进程
        pthread_cond_signal(&pool->notFull);
        //解锁
        pthread_mutex_unlock(&pool->mutexPool);

        printf("Thread %ld start working...\n",pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);

        //执行任务
        (*task.function)(task.args);
        free(task.args);
        task.args = NULL;

        printf("Thread %ld end working...\n",pthread_self());

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }
    return NULL;
}

void threadExit(ThreadPool *pool)
{
    pthread_t tid = pthread_self();
    for (size_t i = 0; i < pool->maxNum; ++i)
    {
        if(pool->threadIds[i] == tid)
        {
            pool->threadIds[i] = 0;
            printf("Thread %ld quit...\n",pthread_self());
            break;
        }
    }
    pthread_exit(NULL);
}
