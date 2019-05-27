#pragma once
#include <QThread>
#include <mutex>
#include <queue>

class MemoryPool;
class FrameData;

class Capture : protected QThread
{
public:
    virtual ~Capture();

    virtual bool startCapture();
    virtual bool stopCapture();
    FrameData *getData();
    void freeData(FrameData *p);
    void clean();
protected:
    Capture();
    void pushData(FrameData *p);
    virtual void run();
    virtual bool captureData(char *p) = 0;
    virtual int syncTimeMs(int use_time) = 0;
    
    // ���̰߳�ȫ
    std::mutex  m_data_lck;              // ������
    std::mutex  m_oper_lck;              // ������
    bool        m_init = false;
    bool        m_start = false;
    MemoryPool  *m_mempool = nullptr;    // �ڴ�ؾ��
    std::queue<FrameData*> m_frames;     // �ڴ����
    int m_cacheSize = 5;
};