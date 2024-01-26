#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QRCodeScanner.h"
#include <QTimer>

class QCamera;
class QVideoWidget;

class QRCodeScanner : public QMainWindow
{
    Q_OBJECT

public:
    QRCodeScanner(QWidget *parent = nullptr);
    ~QRCodeScanner();

signals:
    void recognSuccess(const QStringList &texts, const QStringList &types);
    void recognOutline(const QImage &img, const QList<QPolygon> &rects);
    void windowClosed();

public slots:
    void freshCameras();
    void recognImage(int id, const QImage &img);
    void saveResultToFile();

protected slots:
    void onCameraIndexChanged(int index);
    void onCameraErrorOccurred();
    void onResultsRecieved(const QStringList &texts, const QStringList &types);
    void onResultsOutline(const QImage &img, const QList<QPolygon> &rects) const;

protected:
    void closeEvent(QCloseEvent *e) override;

private:
    Ui::QRCodeScannerClass ui;
    QCamera *m_camera = nullptr;
    QVideoWidget *m_videoWidget = nullptr;
    QTimer *m_timer = nullptr;
};
