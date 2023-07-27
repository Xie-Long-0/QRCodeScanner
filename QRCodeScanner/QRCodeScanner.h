#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QRCodeScanner.h"
#include <QCamera>
#include <QTimer>
#include <QMap>

#include "QZXing.h"

class ImageCaptureWidget;

class QRCodeScanner : public QMainWindow
{
    Q_OBJECT

public:
    QRCodeScanner(QWidget *parent = nullptr);
    ~QRCodeScanner();

signals:
    void showResult(const QString &text);

protected:
    void closeEvent(QCloseEvent *e) override;

    void decodeImage(int id, const QImage &img);

private:
    Ui::QRCodeScannerClass ui;
    QCamera *m_camera = nullptr;
    ImageCaptureWidget *m_widget = nullptr;
    QTimer *m_timer = nullptr;
};
