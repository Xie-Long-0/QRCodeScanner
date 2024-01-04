#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QRCodeScanner.h"
#include <QCamera>
#include <QTimer>

class QRCodeScanner : public QMainWindow
{
    Q_OBJECT

public:
    QRCodeScanner(QWidget *parent = nullptr);
    ~QRCodeScanner();

signals:
    void recognSuccess(const QStringList &texts, const QStringList &types);
    void recognOutline(const QImage &img, const QList<QRect> &rects);
    void windowClosed();

public slots:
    void recognImage(int id, const QImage &img);

protected slots:
    void onCameraIndexChanged(int index);
    void onResultsRecieved(const QStringList &texts, const QStringList &types);
    void onResultsOutline(const QImage &img, const QList<QRect> &rects);

protected:
    void closeEvent(QCloseEvent *e) override;

private:
    Ui::QRCodeScannerClass ui;
    QCamera *m_camera = nullptr;
    QTimer *m_timer = nullptr;
};
