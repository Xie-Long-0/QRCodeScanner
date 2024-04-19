#pragma once

#include <QWidget>
#include "ui_QRCodeGenerator.h"
#include <QImage>

class ImageView;

class QRCodeGenerator : public QWidget
{
    Q_OBJECT

public:
    QRCodeGenerator(QWidget *parent = nullptr);
    ~QRCodeGenerator();

public slots:
    void onGenerateBtnClicked();
    void onSaveImgBtnClicked();

private:
    Ui::QRCodeGeneratorClass ui;
    QImage m_qrImg;
    ImageView *m_viewer = nullptr;
};
