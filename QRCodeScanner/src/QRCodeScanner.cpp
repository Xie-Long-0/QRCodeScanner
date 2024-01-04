#include "QRCodeScanner.h"
#include <QImageCapture>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <QTimer>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QPainter>
#include <QPainterPath>

#include "ZXing/ReadBarcode.h"
#include "ZXing/ZXingQtReader.h"

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
            ui.statusBar->showMessage(tr("就绪"));
        }
        });

    // 相机发生错误
    connect(m_camera, &QCamera::errorOccurred, this, [=] {
        // 调用停止
        ui.stopBtn->click();
        ui.startBtn->setEnabled(false);
        // 显示错误
        QMessageBox::critical(this, tr("错误"), m_camera->errorString());
        ui.statusBar->showMessage(tr("相机发生错误"));
        });

    ui.statusBar->showMessage(tr("未找到相机"));

    auto mediaDevices = new QMediaDevices(this);

    connect(ui.cameraComBox, &QComboBox::currentIndexChanged, this, &QRCodeScanner::onCameraIndexChanged);

    // 刷新相机
    auto freshCameras = [=] {
        auto cid = ui.cameraComBox->currentData();
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
        if (cid.isValid())
        {
            ui.cameraComBox->setCurrentIndex(ui.cameraComBox->findData(cid));
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
            ui.statusBar->showMessage(tr("就绪"));
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
    connect(imageCapture, &QImageCapture::imageCaptured, this, &QRCodeScanner::recognImage);

    // 显示结果
    connect(this, &QRCodeScanner::recognSuccess, this, &QRCodeScanner::onResultsRecieved);
    connect(this, &QRCodeScanner::recognOutline, this, &QRCodeScanner::onResultsOutline);

    // TODO
    //auto fpsLabel = new QLabel("FPS: 0", this);
    //ui.statusBar->addPermanentWidget(fpsLabel);

    // 菜单->关闭
    connect(ui.action_quit, &QAction::triggered, this, &QMainWindow::close);
    // 菜单->保存
    connect(ui.action_save, &QAction::triggered, this, [=] {
        // TODO
        //auto fileName = QFileDialog::getSaveFileName(this, tr("保存文本"),
        //QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        //tr("文本文档 (*.txt)"));
        });

    // 开始按钮
    connect(ui.startBtn, &QPushButton::clicked, this, [=] {
        m_camera->start();
        ui.startBtn->setEnabled(false);
        ui.stopBtn->setEnabled(true);
        ui.statusBar->showMessage(tr("正在捕捉"));
        m_timer->start();
        });
    // 停止按钮
    connect(ui.stopBtn, &QPushButton::clicked, this, [=] {
        m_timer->stop();
        m_camera->stop();
        ui.stopBtn->setEnabled(false);
        ui.startBtn->setEnabled(true);
        ui.statusBar->showMessage(tr("已停止"));
        });
}

QRCodeScanner::~QRCodeScanner()
{
}

void QRCodeScanner::recognImage(int id, const QImage &img)
{
    if (img.isNull())
        return;

    using namespace ZXing;
    using namespace ZXingQt;

    auto task = [=] {

        // ZXing参数
        auto &&options = ReaderOptions()
            // 识别的格式，Any = LinearCodes | MatrixCodes
            .setFormats(BarcodeFormat::Any)
            .setTryHarder(false)
            .setTryRotate(true)
            .setTryInvert(true)
            .setTextMode(TextMode::HRI)
            .setMaxNumberOfSymbols(5);

        // 调用ZXing接口
        auto results = ReadBarcodes(img, options);

        QStringList texts;
        QStringList types;
        QList<QRect> rects;

        for (auto &result : results)
        {
            auto &&pos = result.position();
            int tlx = std::min({ pos[0].x(), pos[1].x(), pos[2].x(), pos[3].x() });
            int tly = std::min({ pos[0].y(), pos[1].y(), pos[2].y(), pos[3].y() });
            int brx = std::max({ pos[0].x(), pos[1].x(), pos[2].x(), pos[3].x() });
            int bry = std::max({ pos[0].y(), pos[1].y(), pos[2].y(), pos[3].y() });
            QRect rect(QPoint(tlx, tly), QPoint(brx, bry));

            qDebug() << "Text:    " << result.text();
            qDebug() << "Format:  " << result.formatName();
            qDebug() << "Content: " << result.contentTypeName();
            qDebug() << "Position:" << pos[0] << pos[1] << pos[2] << pos[3] << rect << Qt::endl;

            // 保存结果
            texts += result.text();
            types += result.formatName();
            rects += rect;
        }

        if (!texts.isEmpty())
        {
            // 发送已识别信号
            emit recognSuccess(texts, types);
            emit recognOutline(img, rects);
        }
        };
    // 运行识别任务
    QtConcurrent::run(task);
}

// 相机选择
void QRCodeScanner::onCameraIndexChanged(int index)
{
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
}

void QRCodeScanner::onResultsRecieved(const QStringList &texts, const QStringList &types)
{
    ui.stopBtn->click();
    for (int i = 0; i < texts.size(); i++)
    {
        ui.historyBrowser->setTextColor(Qt::gray);
        ui.historyBrowser->append(QString("[%1/%2 %3]").arg(i + 1).arg(texts.size()).arg(types[i]));
        ui.historyBrowser->setTextColor(Qt::black);
        ui.historyBrowser->append(texts[i]);
    }
    ui.historyBrowser->setTextColor(Qt::gray);
    ui.historyBrowser->append(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]\n"));
}

void QRCodeScanner::onResultsOutline(const QImage &img, const QList<QRect> &rects)
{
    QImage rsimg = img;
    QPainter p(&rsimg);

    QPainterPath pp;
    pp.addRect(rsimg.rect());

    QFont font = qApp->font();
    font.setPointSizeF(20);
    font.setBold(true);
    p.setFont(font);
    p.setPen(QPen(Qt::cyan, 5));
    p.setRenderHint(QPainter::TextAntialiasing);

    // 标记识别到的位置
    for (int i = 0; i < rects.size(); i++)
    {
        auto &r = rects[i];
        pp.addRect(r);
        p.drawRect(r);
        p.drawText(r.center(), QString("[%1]").arg(i + 1));
    }
    p.fillPath(pp, QColor(0, 0, 0, 128));
    p.end();

    // 新窗口显示图片
    auto *label = new QLabel();
    label->setWindowTitle(tr("识别结果"));
    label->resize(720, 400);
    label->setPixmap(QPixmap::fromImage(rsimg));
    label->setAlignment(Qt::AlignCenter);
    label->setScaledContents(true);
    connect(this, &QRCodeScanner::windowClosed, label, &QWidget::deleteLater);
    label->show();
}

void QRCodeScanner::closeEvent(QCloseEvent *e)
{
    QMainWindow::closeEvent(e);
    emit windowClosed();
}
