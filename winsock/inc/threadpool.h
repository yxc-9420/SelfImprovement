#ifndef THREADPOOL
#define THREADPOOL

#include <pthread.h>

//任务结构体
struct Task
{
    void (*function)(void* args);
    void* args;
};

//线程池结构体
struct ThreadPool
{
    //任务队列
    Task* taskQ;
    int queueCapacity;  //容量
    int queueSize;      //当前任务个数
    int queueFront;     //队头 ->取数据
    int queueRear;      //队尾 ->放数据

    pthread_t managerId;    //管理者线程ID
    pthread_t* threadIds;   //工作的线程ID
    int minNum;             //最小线程数量
    int maxNum;             //最大线程数量
    int busyNum;            //忙的线程个数
    int liveNum;            //存活的线程个数
    int exitNum;            //要销毁的线程个数

    pthread_mutex_t mutexPool;  //锁整个线程池
    pthread_mutex_t mutexBusy;  //锁busyNum变量
    pthread_cond_t notFull;     //任务队列是否满了
    pthread_cond_t notEmpty;    //任务队列是否空了

    int shutDown;               //是否销毁线程池，销毁为1，不销毁为0
};


//创建线程池并初始化
ThreadPool* threadPoolCreate(int min, int max, int queueSize);

//销毁线程池
int threadPoolDestroy(ThreadPool* pool);

//给线程池添加任务
void threadPoolAdd(ThreadPool* pool, void (*func)(void*), void*);

//获取工作线程个数
int threadPoolBusyNum(ThreadPool* pool);
//获取存活线程个数
int threadPoolLiveNum(ThreadPool* pool);

void* manager(void* args);
void* worker(void* args);
void threadExit(ThreadPool* pool);


#endif