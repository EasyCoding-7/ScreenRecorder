#include "MemoryPool.h"

MemoryPool::MemoryPool(uint64_t poolsize, uint64_t datasize)
{
    void *p = nullptr;
    m_lock.lock();
    for (int i = 0; i < poolsize; ++i)
    {
        p = new char[datasize];
        if (!p) { continue; }
        
        m_pool.push(p);
        m_poolsize++;
    }
    m_lock.unlock();
}

MemoryPool::~MemoryPool()
{
    m_lock.lock();
    while (!m_pool.empty())
    {
        void *p = m_pool.front();
        delete p;
        m_pool.pop();
    }
    m_lock.unlock();
}

void * MemoryPool::allocMemory()
{
    std::lock_guard<std::mutex> lck(m_lock);
    if (m_pool.empty())
    {
        return nullptr;
    }

    void *p = m_pool.front();   // �ڴ����ȡֵ
    m_pool.pop();               // ���ڴ�����        
    m_used[p] = true;           // ��¼ʹ��

    return p;
}

void MemoryPool::freeMemory(void * p)
{
    if (!p)
    {
        return;
    }
    m_lock.lock();
    auto iter = m_used.find(p);
    if (iter != m_used.end())
    {
        m_used.erase(iter);     // ����ʹ�ü�¼
        m_pool.push(p);         // �������
    }
    m_lock.unlock();
}

void MemoryPool::clean()
{
    m_lock.lock();
    for (auto iter = m_used.begin(); iter != m_used.end(); iter++)
    {
        m_pool.push(iter->first);
        //iter = m_used.erase(iter);
    }
    m_used.clear();
    m_lock.unlock();
}
