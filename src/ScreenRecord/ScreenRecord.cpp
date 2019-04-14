#include "ScreenRecord.h"

#include "VideoCapture.h"

#include <QPainter>
#include <QTime>

static VideoCapture *ct = nullptr;

ScreenRecord::ScreenRecord(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    
    ct = new VideoCapture();
    //ct->init(VideoCapture::VID_CAP_MODE_DIRECTX);
    ct->init(VideoCapture::VID_CAP_MODE_DIRECTX);
    ct->startCapture();

    // ���Դ���
    startTimer(100);
}

// qt��ʱ�����źŲ�ʵ�֣��������߸���ʱ��ȵĲ���
void ScreenRecord::timerEvent(QTimerEvent *event)
{
    update();
}

void ScreenRecord::paintEvent(QPaintEvent *event)
{
    uchar *data = (uchar*)ct->getData();
    if (!data) return;
    QImage image(data, ct->width(), ct->height(), QImage::Format_ARGB32);
    
    // ���ƽ���
    QPainter p;
    p.begin(this);
    p.drawImage(QPoint(), image);
    p.end();
    delete data;
}