#pragma once
#include "MediaFormat.h"
#include <mutex>
#include <d3d9.h>

class QScreen;

class ScreenImageCapture
{
public:
    ScreenImageCapture();
    virtual ~ScreenImageCapture();
    bool init(int width, int height, VidCapMode mode = VID_CAP_MODE_DX9, int wid = 0);
    bool deinit();
    bool captureScreen(char *data);
protected:
    virtual bool captureScreenWithDx9(char *data);
    virtual bool captureScreenWithQt(char *data);
private:
    std::mutex  m_lock;     // �̰߳�ȫ
    VidCapMode  m_mode;     // ץȡ��Ļģʽ
    bool        m_init = false;

    // DirectX 9
    IDirect3D9          *m_d3d = nullptr;       // directx3d ����
    IDirect3DDevice9    *m_device = nullptr;    // �Կ��豸����
    IDirect3DSurface9   *m_surface = nullptr;   // ��������
    D3DLOCKED_RECT      *m_rect = nullptr;      // ץ������

    // QT
    QScreen *m_screen = nullptr;
    int     m_wid = 0;

    int         m_width;
    int         m_height;
};

