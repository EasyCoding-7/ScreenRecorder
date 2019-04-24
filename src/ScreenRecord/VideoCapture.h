#pragma once
#include "MediaFormat.h"
#include <d3d9.h>
#include <queue>
#include <QThread>

class QScreen;
class QPixmap;
class MemoryPool;

class VideoCapture : protected QThread
{
public:
    VideoCapture();
    virtual ~VideoCapture();

    bool init(const VidRecordParam& param);
    bool uninit();
    bool startCapture();
    bool stopCapture();
    void *getData();    //�̰߳�ȫ
    void freeData(void *p);

    int width() const { return m_width; }
    int height() const { return m_height; }
    int bitsize() const { return m_bitsize; }
    int fps() const { return m_fps; }
    VidCapMode mode() const { return m_mode; }
protected:
    virtual void run();
    virtual bool captureData(void *data);
    bool captureWithDirectX(void *data);
    bool captureWithQt(void *data);

private:
    bool m_init = false;
    bool m_start = false;
    std::mutex m_oper_lock;     // ������
    std::mutex m_data_lock;     // ������
    MemoryPool *m_mempool = nullptr;
    std::queue<void*> m_datas;
    int m_cacheSize = 5;

    VidCapMode          m_mode = VID_CAP_MODE_DIRECTX;
    VidPixFmt           m_format = VID_PIX_FMT_BGRA;

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
    int m_bitsize = 4;      // bgra
    int m_fps = 25;
};