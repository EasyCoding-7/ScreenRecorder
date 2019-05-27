#include "ScreenImageCapture.h"
#include "glog/logging.h"

#include <QScreen>
#include <QApplication>
#include <QGuiApplication> 
#include <QDesktopWidget> 

#pragma comment(lib, "d3d9.lib")

ScreenImageCapture::ScreenImageCapture()
{
}

ScreenImageCapture::~ScreenImageCapture()
{
}

bool ScreenImageCapture::init(int width, int height, VidCapMode mode, int wid)
{
    std::lock_guard<std::mutex> lck(m_lock);

    m_width = width;
    m_height = height;
    m_mode = mode;
    m_wid = wid;

    LOG(INFO) << "init width:" << m_width << ", height:" << m_height << ", mode:" << m_mode;

    if (VID_CAP_MODE_DX9 == m_mode)
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
        //int width = GetSystemMetrics(SM_CXSCREEN);
        //int height = GetSystemMetrics(SM_CYSCREEN);
        m_device->CreateOffscreenPlainSurface(m_width, m_height,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &m_surface, nullptr);
        if (!m_surface)
        {
            LOG(ERROR) << "CreateOffscreenPlainSurface failed";
            return false;
        }

        // 4.��ʼ��ץ������
        m_rect = (D3DLOCKED_RECT*) new _D3DLOCKED_RECT;
        ZeroMemory(m_rect, sizeof(*m_rect));
    }
    else if (VID_CAP_MODE_QT == m_mode)
    {
        m_screen = QGuiApplication::primaryScreen();
    }
    else
    {
        LOG(WARNING) << "not support!";
        return false;
    }

    m_init = true;

    return true;
}

bool ScreenImageCapture::deinit()
{
    std::lock_guard<std::mutex> lck(m_lock);

    LOG(INFO) << "deinit width:" << m_width << ", height:" << m_height << ", mode:" << m_mode;
    delete m_rect;
    m_rect = nullptr;
    m_screen = nullptr;
    m_wid = 0;
    m_init = false;
    //Direct3D Ӧ�ñ���������δ�ҵ���Ӧ����

    return true;
}

bool ScreenImageCapture::captureScreen(char * data)
{
    if (!data)
    {
        LOG(ERROR) << "param is null!";
        return false;
    }

    std::lock_guard<std::mutex> lck(m_lock);

    if (!m_init)
    {
        LOG(ERROR) << "not init!";
        return false;
    }

    if (VID_CAP_MODE_DX9 == m_mode)
    {
        return captureScreenWithDx9(data);
    }
    else if (VID_CAP_MODE_QT == m_mode)
    {
        return captureScreenWithQt(data);
    }
    else
    {
        LOG(ERROR) << "not support mode:" << m_mode;
        return false;
    }
}

bool ScreenImageCapture::captureScreenWithDx9(char * data)
{
    // ץ��
    m_device->GetFrontBufferData(0, m_surface);

    // ȡ������
    if (m_surface->LockRect(m_rect, nullptr, 0) != S_OK)
    {
        m_surface->UnlockRect();
        LOG(ERROR) << "directx surface LockRect failed!";
        return false;
    }
    memcpy(data, m_rect->pBits, m_width * m_height * 4);    // bgra is 4 bytes
    m_surface->UnlockRect();

    return true;
}

bool ScreenImageCapture::captureScreenWithQt(char * data)
{
    // ��ȡȫ��
    if (!m_screen)
    {
        return false;
    }

    //winId�൱��windows��̵�WHND
    QPixmap pixmap = m_screen->grabWindow(m_wid);
    memcpy(data, pixmap.toImage().bits(), m_width * m_height * 4);

    return true;
}