#pragma once

#include <QWidget>
#include "ui_ImageCaptureWidget.h"
#include <QImage>
#include <QImageCapture>

class ImageCaptureWidget : public QWidget
{
    Q_OBJECT

public:
    ImageCaptureWidget(QImageCapture *imgCapture, QWidget *parent = nullptr);
    ~ImageCaptureWidget();

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    Ui::ImageCaptureWidgetClass ui;
    QImageCapture *m_imgCapture = nullptr;
    QImage m_image;
};
