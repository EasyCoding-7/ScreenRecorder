#pragma once

#include <QtWidgets/QWidget>
#include "ui_ScreenRecord.h"

class ScreenRecord : public QWidget
{
    Q_OBJECT

public:
    ScreenRecord(QWidget *parent = Q_NULLPTR);

    // ���Դ���
    void timerEvent(QTimerEvent *event);
    void paintEvent(QPaintEvent *event);

private:
    Ui::ScreenRecordClass ui;
};
