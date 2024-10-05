#pragma once
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <assert.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/sem.h>
#include <string>
#include <sys/types.h>

#define BUFSZ 128

enum SHMRingPattern
{
    Product,
    Consumer
};

// struct sembuf
// {
//     unsigned short sem_num; // 信号在信号集中的索引，0代表第一个信号，1代表第二个信号
//     short sem_op;           // 操作类型
//     short sem_flg;          // 操作标志
// };

template <typename T>
class RingBufferShm
{
public:
    typedef struct Header
    {
        T *playLoad;
        size_t front;
        size_t rear;
    } Header;

private:
    // 用于获取ftok的文件
    std::string filepath;
    key_t ftk;
    // 头尾信号量
    key_t ftk_sm_front;
    int sem_front;
    key_t ftk_sm_rear;
    int sem_rear;
    size_t ring_size;
    // 头信息
    Header *header;
    // 共享内存id
    int shm_id;
    // 创建模式
    SHMRingPattern pattern;
    // 判断队列是否为空
    bool isEmpty();
    // 判断队列十分满
    bool isFull();
    // 初始化信号量
    void semaphoreInit();
    // 信号量P操作
    void semaphoreP(int sem_id);
    // 信号量V操作
    void semaphoreV(int sem_id);
    // 删除信号量
    void clearSemaphore(int sem_id);

public:
    RingBufferShm(const std::string, size_t, SHMRingPattern);
    ~RingBufferShm();
    // 向队列插入元素
    void enQueue(T &item);
    // 向队列获取元素
    T getQueue();
    // 显示队列所有元素
    void show();
};

template <typename T>
inline bool RingBufferShm<T>::isEmpty()
{
    if (header->front == header->rear)
        return true;
    return false;
}

template <typename T>
inline bool RingBufferShm<T>::isFull()
{
    if ((header->rear + 1) % ring_size == header->front)
    {
        std::cout << "队列已满" << std::endl;
        return true;
    }
    return false;
}

template <typename T>
inline void RingBufferShm<T>::semaphoreInit()
{
    sem_front = semget(ftk_sm_front, 1, IPC_CREAT | 0644);
    if (sem_front == -1)
    {
        std::cout << "头信号量创建失败" << std::endl;
    }
    sem_rear = semget(ftk_sm_rear, 1, IPC_CREAT | 0644);
    if (sem_rear == -1)
    {
        std::cout << "尾巴信号量创建失败" << std::endl;
    }
    if (pattern == Product)
    {
        // 给信号量赋予初值
        int flag1 = semctl(sem_front, 0, SETVAL, 1);
        int flag2 = semctl(sem_rear, 0, SETVAL, 1);
        std::cout << "创建信号量成功" << std::endl;
    }
}

template <typename T>
inline void RingBufferShm<T>::semaphoreP(int sem_id)
{
    std::cout << "进入P操作 sem_id: " << sem_id << std::endl;
    sembuf sbf{};
    sbf.sem_num = 0;
    sbf.sem_op = -1;
    sbf.sem_flg = SEM_UNDO;
    int flag = semop(sem_id, &sbf, 1);
    if (-1 != flag)
    {
        std::cout << "信号P操作成功, id: " << sem_id << std::endl;
        return;
    }
    perror("信号P操失败");
    std::cout << "信号P操失败, id: " << sem_id << std::endl;
}

template <typename T>
inline void RingBufferShm<T>::semaphoreV(int sem_id)
{
    std::cout << "进入V操作 sem_id: " << sem_id << std::endl;
    sembuf sbf{};
    sbf.sem_num = 0;
    sbf.sem_op = 1;
    sbf.sem_flg = SEM_UNDO;
    int flag = semop(sem_id, &sbf, 1);
    if (-1 != flag)
    {
        std::cout << "信号V操作成功, id: " << sem_id << std::endl;
    }
}

template <typename T>
inline void RingBufferShm<T>::clearSemaphore(int sem_id)
{
    int flag = semctl(sem_id, 0, IPC_RMID);
    if (-1 == flag)
    {
        std::cout << "信号删除失败, id:  " << sem_id << std::endl;
        return;
    }
    std::cout << "信号删除成功, id:  " << sem_id << std::endl;
}

template <typename T>
RingBufferShm<T>::RingBufferShm(const std::string filepath, size_t ring_size, SHMRingPattern Pattern)
    : filepath(filepath), ring_size(ring_size), pattern(Pattern)
{
    // 获取ftok
    ftk = ftok(filepath.c_str(), 0);
    // 创建头信号量tk
    ftk_sm_front = ftk + 1;
    ftk_sm_rear = ftk + 2;
    int shm_fd_id = shmget(ftk, sizeof(Header) + sizeof(T) * (ring_size + 1), 0644 | IPC_CREAT);
    std::cout << sizeof(Header) << std::endl;
    if (-1 != shm_fd_id)
    {
        shm_id = shm_fd_id;
        std::cout << "创建共享内存成功 shmid:" << shm_fd_id << std::endl;
        // 挂载内存
        header = (Header *)shmat(shm_fd_id, NULL, 0);
        if (header == (Header *)-1)
        {
            perror("共享内存挂载失败");
            shmctl(shm_fd_id, IPC_RMID, NULL);
            exit(-1);
        }
        std::cout << "内存挂载成功 iD:  " << shm_fd_id << std::endl;
        // 判断初始化模式
        if (Pattern == Product)
        {
            // 初始化Header信息
            header->front = 0;
            header->rear = 0;
            std::cout << "Product 初始化队列头尾" << std::endl;
        }
        // header
        header->playLoad = (T *)(header + 1);
        // 初始化信号量
        semaphoreInit();
    }
    else
    {
        perror("创建失败");
        exit(-1);
    }
}

template <typename T>
RingBufferShm<T>::~RingBufferShm()
{
    // shmdt(shm_id);
    if (pattern == Product)
    {
        int m = shmctl(shm_id, IPC_RMID, NULL);
        if (-1 == m)
        {
            std::cout << "卸载共享内存失败， shm_id: " << shm_id << std::endl;
        }
        else
        {
            std::cout << "卸载共享内存成功， shm_id: " << shm_id << std::endl;
        }
        clearSemaphore(sem_front);
        clearSemaphore(sem_rear);
    }
}

template <typename T>
inline void RingBufferShm<T>::enQueue(T &item)
{
    // 判断队列是否已满
    if (isFull())
    {
        std::cout << "插入失败" << std::endl;
        return;
    }
    semaphoreP(sem_rear);
    memcpy(header->playLoad + header->rear, &item, sizeof(item));
    header->rear = (header->rear + 1) % ring_size;
    semaphoreV(sem_rear);
}

template <typename T>
inline T RingBufferShm<T>::getQueue()
{
    // 判断队列是否为空
    if (isEmpty())
    {
        std::cout << "获取失败" << std::endl;
        return T();
    }
    semaphoreP(sem_front);
    T &t = *(header->playLoad + header->front);
    header->front = (header->front + 1) % ring_size;
    semaphoreV(sem_front);
    return t;
}

template <typename T>
inline void RingBufferShm<T>::show()
{
    if (isEmpty())
    {
        std::cout << "队列为空" << std::endl;
        return;
    }
    size_t i = 0;
    while ((header->front + i) % ring_size != header->rear)
    {
        //
        std::cout << (header->playLoad + (header->front + i) % ring_size) << std::endl;
        i++;
    }
}
