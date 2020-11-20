#pragma once

#include<list>
#include<cstdio>
#include<exception>
#include<pthread.h>
#include"locker.h"

template<typename T>
class Threadpool
{
public:
    Threadpool(int thread_number=8,int max_requests=10000);
    ~Threadpool();

    //往请求队列中追加任务
    bool append(T* request);
private:
    //工作线程运行的函数,它从工作队列中取出任务并执行
    static void* worker(void* arg);
    void run();
private:
    int m_thread_number; //线程池中的线程数
    int m_max_requests; //请求队列中允许的最大请求数
    pthread_t* m_threads;//描述线程池的数组,大小为m_thread_number
    std::list<T*> m_workqueue;//请求队列
    Mutex m_queuelocker;//保护请求队列的互斥锁
    Sem m_queuestat;//是否有任务需要处理
    bool m_stop;//是否结束线程
};

template<typename T>
Threadpool<T>::Threadpool(int thread_number,int max_requests)
    :m_thread_number(thread_number),m_max_requests(max_requests),m_stop(false),m_threads(NULL)
{
    if((thread_number<=0) || (max_requests<=0)){
        throw std::exception();
    }

    m_threads=new pthread_t[m_thread_number];
    if(!m_threads){
        throw std::exception();
    }

    //创建m_thread_number个线程，并将它们都设置为脱离线程
    for(int i=0;i<m_thread_number;++i){
        printf("create the %dth thread\n",i);
        if(pthread_create(m_threads+i,NULL,worker,this)!=0){
            delete [] m_threads;
            throw std::exception();
        }
        if(pthread_detach(m_threads[i])){
            delete [] m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
Threadpool<T>::~Threadpool()
{
    delete [] m_threads;
    m_stop=true;
}

template<typename T>
bool Threadpool<T>::append(T* request)
{
    m_queuelocker.lock();
    if(m_workqueue.size()>m_max_requests){
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}
