#include "QtScreen.h"

#include <QDesktopWidget> 
#include <QScreen>
#include <QPainter>
#include <QTime>

#include <d3d9.h>
#include <iostream>

#pragma comment(lib, "d3d9.lib")

using namespace std;

void captureScreen(void *data, int width, int height, int bitsize)
{
    // 1.����directx3d����
    static IDirect3D9 *d3d = nullptr;
    if (!d3d)
    {
        d3d = Direct3DCreate9(D3D_SDK_VERSION);
        if (!d3d)
        {
            cout << "Direct3DCreate9 failed" << endl;
            return;
        }
    }

    // 2 �����Կ����豸����
    static IDirect3DDevice9 *device = nullptr;
    if (!device)
    {
        D3DPRESENT_PARAMETERS pa;
        ZeroMemory(&pa, sizeof(pa));
        pa.Windowed = true;
        pa.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
        pa.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pa.hDeviceWindow = GetDesktopWindow();
        d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 0,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &pa, &device);
        if (!device)
        {
            cout << "CreateDevice failed" << endl;
            return;
        }
    }

    // 3.������������
    static IDirect3DSurface9 *sur = nullptr;
    if (!sur)
    {
        device->CreateOffscreenPlainSurface(width, height,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &sur, nullptr);
        if (!sur)
        {
            cout << "CreateOffscreenPlainSurface failed" << endl;
            return;
        }
    }

    // 4.ץ��
    device->GetFrontBufferData(0, sur);

    // 5.ȡ������
    D3DLOCKED_RECT rect;
    ZeroMemory(&rect, sizeof(rect));

    if (sur->LockRect(&rect, nullptr, 0) != S_OK)
    {
        cout << "sur->LockRect failed" << endl;
        return;
    }
    memcpy(data, rect.pBits, width * height * bitsize);
    sur->UnlockRect();
}

QtScreen::QtScreen(QWidget *parent)
{
    ui.setupUi(this);
    startTimer(10);
}

// qt��ʱ�����źŲ�ʵ�֣��������߸���ʱ��ȵĲ���
void QtScreen::timerEvent(QTimerEvent * event)
{
    update();
}

void QtScreen::paintEvent(QPaintEvent * event)
{
    // 1.��ȡQScreen
    static QScreen *screen = nullptr;
    if (!screen)
    {
        screen = QGuiApplication::primaryScreen();
    }

    int width = 2560;
    int height = 1440;
    int bitsize = 4;
    int bufsize = width * height * bitsize;
    //int width = QApplication::desktop()->width();
    //int height = QApplication::desktop()->height();


    static QImage *image = nullptr;
    static QTime tm;
    static int directTime = 0;
    static int qtTime = 0;
    int tmp;
    if (!image)
    {
        uchar *buf = new uchar[bufsize];
        image = new QImage(buf, width, height, QImage::Format_ARGB32);
        tm.start();
    }
    tm.restart();
    captureScreen(image->bits(), width, height, bitsize);
    tmp = tm.restart();
    cout << "directx:" << tmp << endl;
    directTime += tmp;
    
    // ��ȡȫ��
    tm.restart();
    QPixmap pix = screen->grabWindow(QApplication::desktop()->winId());   //winId�൱��windows��̵�WHND
    tmp = tm.restart();
    cout << "qt     :" << tmp << endl;
    qtTime += tmp;
    
    cout << "directTime:" << directTime << ", qtTime:" << qtTime << endl;;

    // ���ƽ���
    QPainter p;
    p.begin(this);
    p.drawImage(QPoint(), *image);
    p.end();
}