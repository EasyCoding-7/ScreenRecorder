#include "VideoCapture.h"
#include "glog/logging.h"

//#include <QApplication>
//#include <QGuiApplication> 
//#include <QDesktopWidget> 
//#include <QScreen>
#include <QTime>

#include <iostream>

#pragma comment(lib, "d3d9.lib")

const char *VideoCapture::VidCapModeStr(VidCapMode mode)
{
    static char* str[] =
    {
        "none",
        "DirectX",
        "QT"
    };
    if (mode > VID_CAP_MODE_NONE && mode < VID_CAP_MODE_CNT) {
        return str[mode];
    } else {
        return str[VID_CAP_MODE_NONE];
    }
}

VideoCapture::VideoCapture(VidCapMode mode, int fps) : m_mode(mode), m_fps(fps)
{
}

VideoCapture::~VideoCapture()
{
}

bool VideoCapture::init()
{
    if (m_init)
    {
        LOG(WARNING) << "VideoCapture is already inited";
        return true;
    }

    if (VID_CAP_MODE_DIRECTX == m_mode)
    {
        // 1.����directx3d����
        m_d3d = Direct3DCreate9(D3D_SDK_VERSION);
        if (!m_d3d)
        {
            LOG(ERROR) << "Direct3DCreate9 failed!";
            return false;
        }

        // 2.�����Կ����豸����
        D3DPRESENT_PARAMETERS pa;
        ZeroMemory(&pa, sizeof(pa));
        pa.Windowed = true;
        pa.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
        pa.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pa.hDeviceWindow = GetDesktopWindow();
        m_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 0,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &pa, &m_device);
        if (!m_device)
        {
            LOG(ERROR) << "CreateDevice failed";
            return false;
        }

        // 3.������������
        m_width = GetSystemMetrics(SM_CXSCREEN);
        m_height = GetSystemMetrics(SM_CYSCREEN);
        m_device->CreateOffscreenPlainSurface(m_width, m_height,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &m_surface, nullptr);
        if (!m_surface)
        {
            LOG(ERROR) << "CreateOffscreenPlainSurface failed";
            return false;
        }

        // ��ʼ��ץ������
        m_rect = new D3DLOCKED_RECT();
        ZeroMemory(m_rect, sizeof(*m_rect));
    }
    else if (VID_CAP_MODE_QT == m_mode)
    {
        //m_screen = QGuiApplication::primaryScreen();
        //m_width = m_screen->size().width();
        //m_height = m_screen->size().height();
    }
    else
    {
        return false;
    }

    m_mutex.lock();
    m_init = true;
    m_mutex.unlock();

    return true;
}

bool VideoCapture::uninit()
{
    //Direct3D
    m_mutex.lock();
    m_init = false;
    m_mutex.unlock();
    return false;
}

void VideoCapture::run()
{
    QTime time;
    int ms_wait = 0;
    int fps_base = 1000 / m_fps;
    int use_time = 0;
    void *data = nullptr;

    while (m_start)
    {
        time.restart();
        m_mutex.lock();
        if (m_datas.size() >= m_cacheSize)
        {
            m_mutex.unlock();
            LOG(WARNING) << "cache buff full, wait 10ms, cache size:" << m_cacheSize;
            msleep(10); // �������������ȴ�10ms
            continue;
        }
        m_mutex.unlock();

        // ��ȡһ֡����
        data = new char[m_width * m_height * m_bitSize];
        if (!captureData(data))
        {
            delete data;
            LOG(WARNING) << "video captureData failed";
            continue;
        }

        // д�뻺�����
        m_mutex.lock();
        m_datas.push_back(data);
        m_mutex.unlock();

        // ��ץȡ��֡�ʿ���FPS,ͬ��ץȡ��֡��
        use_time = time.restart();
        ms_wait = fps_base - use_time;
        if (ms_wait > 0)
        {
            msleep(ms_wait);
        }
        std::cout << "fps_base:" << fps_base << " use:" << use_time << " ms_wait:" << ms_wait << std::endl;
    }
}

bool VideoCapture::captureData(void *data)
{
    if (!data)
    {
        LOG(ERROR) << "input data is null!";
        return false;
    }

    if (VID_CAP_MODE_DIRECTX == m_mode)
    {
        return captureWithDirectX(data);
    }
    else if (VID_CAP_MODE_QT == m_mode)
    {
        return captureWithQt(data);
    }

    return nullptr;
}

bool VideoCapture::captureWithDirectX(void *data)
{
    // ץ��
    m_device->GetFrontBufferData(0, m_surface);

    // ȡ������
    if (m_surface->LockRect(m_rect, nullptr, 0) != S_OK)
    {
        m_surface->UnlockRect();
        LOG(ERROR) << "directx surface LockRect failed";
        return false;
    }
    memcpy(data, m_rect->pBits, m_width * m_height * m_bitSize);
    m_surface->UnlockRect();

    return true;
}

bool VideoCapture::captureWithQt(void *data)
{
    //winId�൱��windows��̵�WHND
    //QPixmap pixmap = m_screen->grabWindow(QApplication::desktop()->winId());
    //m_screen->grabWindow(QApplication::desktop()->winId());
    //memcpy(data, &pixmap.data_ptr(), m_width * m_height * m_bitSize);
    return false;
}
