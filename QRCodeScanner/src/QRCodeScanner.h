#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QRCodeScanner.h"
#include <QTimer>

class QCamera;
class QVideoWidget;
class QRCodeGenerator;
class ImageView;

class QRCodeScanner : public QMainWindow
{
    Q_OBJECT

public:
    QRCodeScanner(QWidget *parent = nullptr);
    ~QRCodeScanner();

signals:
    void recognSuccess(const QStringList &texts, const QStringList &types);
    void recognOutline(const QImage &img, const QList<QPolygon> &rects);
    void recognFailed();

public slots:
    void freshCameras();
    void recognImage(int id, const QImage &img);
    void saveResultToFile();
    void openQRGeneratorWidget();
    void openImageFile();

protected slots:
    void onCameraIndexChanged(int index);
    void onCameraErrorOccurred();
    void onResultsRecieved(const QStringList &texts, const QStringList &types);
    void onResultsOutline(const QImage &img, const QList<QPolygon> &rects) const;

private:
    Ui::QRCodeScannerClass ui;
    QCamera *m_camera = nullptr;
    QVideoWidget *m_videoWidget = nullptr;
    QTimer *m_timer = nullptr;
    QRCodeGenerator *m_qrgWidget = nullptr;
    ImageView *m_viewer = nullptr;
};
