#pragma once
#include "CaptureThread.h"
#include <d3d9.h>

class QScreen;
class QPixmap;

class VideoCapture : public CaptureThread
{
public:
    enum VidCapMode
    {
        VID_CAP_MODE_NONE = 0,
        VID_CAP_MODE_DIRECTX,
        VID_CAP_MODE_QT,
        VID_CAP_MODE_CNT
    };
    static const char *VidCapModeStr(VidCapMode mode);

    VideoCapture(VidCapMode mode, int fps);
    virtual ~VideoCapture();

    virtual bool init();
    virtual bool uninit();

    int width() const { return m_width; }
    int height() const { return m_height; }
    int fps() const { return m_fps; }

protected:
    virtual void run();
    virtual bool captureData(void *data);
    
private:
    bool captureWithDirectX(void *data);
    bool captureWithQt(void *data);

private:
    
    VidCapMode          m_mode = VID_CAP_MODE_DIRECTX;
    
    // DirectX 9
    IDirect3D9          *m_d3d = nullptr;       // directx3d ����
    IDirect3DDevice9    *m_device = nullptr;    // �Կ��豸����
    IDirect3DSurface9   *m_surface = nullptr;   // ��������
    D3DLOCKED_RECT      *m_rect = nullptr;      // ץ������

    // QT
    QScreen *m_screen = nullptr;
    QPixmap *m_pixmap = nullptr;

    int m_width = 1920;
    int m_height = 1080;
    int m_bitSize = 4;
    int m_fps = 25;
};