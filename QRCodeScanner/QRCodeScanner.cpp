#include "QRCodeScanner.h"
#include <QMediaDevices>
#include <QMediaCaptureSession>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QImageCapture>
#include <QMessageBox>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>

#include "include/QZXing.h"
#include "ImageCaptureWidget.h"

QRCodeScanner::QRCodeScanner(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    ui.startBtn->setEnabled(false);
    ui.stopBtn->setEnabled(false);

    m_camera = new QCamera(this);

    // 切换相机
    connect(m_camera, &QCamera::cameraDeviceChanged, this, [=] {
        if (m_camera->isAvailable())
        {
            ui.startBtn->setEnabled(true);
            ui.statusBar->showMessage(tr("Ready"));
        }
            });

    // 相机发生错误
    connect(m_camera, &QCamera::errorOccurred, this, [=] {
        m_camera->stop();
        ui.stopBtn->setEnabled(false);
        QMessageBox::critical(this, tr("错误"), m_camera->errorString());
            });

    ui.statusBar->showMessage(tr("No camera found"));

    auto mediaDevices = new QMediaDevices(this);

    // 相机选择
    auto onIndexChanged = [=](int index) {
        m_camera->stop();
        ui.startBtn->setEnabled(false);
        ui.stopBtn->setEnabled(false);
        auto devices = QMediaDevices::videoInputs();
        for (auto &d : devices)
        {
            if (d.id() == ui.cameraComBox->currentData().toByteArray())
            {
                m_camera->setCameraDevice(d);
                break;
            }
        }
    };

    connect(ui.cameraComBox, &QComboBox::currentIndexChanged, this, onIndexChanged);

    // 刷新相机
    auto freshCameras = [=] {
        ui.cameraComBox->clear();
        for (auto &cameraDevice : QMediaDevices::videoInputs())
        {
            if (!cameraDevice.description().isEmpty())
            {
                ui.cameraComBox->addItem(cameraDevice.description(), cameraDevice.id());
            }
            else
            {
                ui.cameraComBox->addItem(cameraDevice.id(), cameraDevice.id());
            }
        }
    };

    connect(mediaDevices, &QMediaDevices::videoInputsChanged, this, freshCameras);
    freshCameras();

    if (ui.cameraComBox->count() > 0)
    {
        ui.cameraComBox->setCurrentIndex(0);
        if (m_camera->isAvailable())
        {
            ui.startBtn->setEnabled(true);
            ui.statusBar->showMessage(tr("Ready"));
        }
    }

    // 持续采集会话
    auto capture = new QMediaCaptureSession(this);
    capture->setCamera(m_camera);

    // 视频输出窗口
    auto videoWidget = new QVideoWidget(ui.previewFrame);
    ui.previewFrameLayout->addWidget(videoWidget);
    capture->setVideoOutput(videoWidget);

    // 图像捕获
    auto imageCapture = new QImageCapture(this);
    capture->setImageCapture(imageCapture);

    // 定时器触发捕获
    m_timer = new QTimer(this);
    m_timer->setInterval(500);
    connect(m_timer, &QTimer::timeout, imageCapture, &QImageCapture::capture);
    connect(imageCapture, &QImageCapture::imageCaptured, this, &QRCodeScanner::decodeImage);

    // 显示结果
    connect(this, &QRCodeScanner::showResult, this, [=](const QString &text) {
        ui.stopBtn->click();
        ui.textBrowser->setText(text);
            });

    //m_widget = new ImageCaptureWidget(imageCapture, nullptr);
    //m_widget->show();

    //auto fpsLabel = new QLabel("FPS: 0", this);
    //ui.statusBar->addPermanentWidget(fpsLabel);

    connect(ui.action_quit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui.startBtn, &QPushButton::clicked, this, [=] {
        m_camera->start();
        ui.startBtn->setEnabled(false);
        ui.stopBtn->setEnabled(true);
        ui.statusBar->showMessage(tr("Capturing"));
        m_timer->start();
            });
    connect(ui.stopBtn, &QPushButton::clicked, this, [=] {
        m_timer->stop();
        m_camera->stop();
        ui.stopBtn->setEnabled(false);
        ui.startBtn->setEnabled(true);
        ui.statusBar->showMessage(tr("Stopped"));
            });
}

QRCodeScanner::~QRCodeScanner()
{
}

void QRCodeScanner::closeEvent(QCloseEvent *e)
{
    //m_widget->close();
    //m_widget->deleteLater();
    QWidget::closeEvent(e);
}

void QRCodeScanner::decodeImage(int id, const QImage &img)
{
    if (img.isNull())
        return;

    auto task = [=] {

        // QZXing配置
        auto decoder = new QZXing();
        decoder->setDecoder(QZXing::DecoderFormat_QR_CODE | QZXing::DecoderFormat_EAN_13 |
                            QZXing::DecoderFormat_CODE_39 | QZXing::DecoderFormat_CODABAR);
        decoder->setSourceFilterType(QZXing::SourceFilter_ImageNormal | QZXing::SourceFilter_ImageInverted);
        decoder->setTryHarder(false);

        auto result = decoder->decodeImage(img);
        // 识别成功
        if (!result.isEmpty())
        {
            qDebug() << "time:" << decoder->getProcessTimeOfLastDecoding();
            emit showResult(result);
        }
        decoder->deleteLater();
    };
    QtConcurrent::run(task);
}
