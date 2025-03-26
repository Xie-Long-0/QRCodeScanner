#include "QRCodeScanner.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#define QT5VER
#endif

#include <QCamera>
#ifdef QT5VER
#include <QCameraViewfinder>
#include <QCameraInfo>
#include <QCameraImageCapture>
#else
#include <QImageCapture>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#endif // QT5VER

#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <QTimer>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QPainter>
#include <QPainterPath>
#include <QFileDialog>

#include <ZXing/ReadBarcode.h>

#include "ZXingQtReader.h"
#include "QRCodeGenerator.h"
#include "ImageView.h"

QRCodeScanner::QRCodeScanner(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

#ifdef QT5VER
    // Qt5需要注册该类型用于属性与信号槽
    qRegisterMetaType<QList<QPolygon>>("QList<QPolygon>");
#endif // QT5VER

    ui.startBtn->setEnabled(false);
    ui.stopBtn->setEnabled(false);
    ui.statusBar->showMessage(tr("未找到相机"));

    // 使用StackedWidget切换显示视频与图片控件
    m_viewer = new ImageView(ui.previewImageWidget);
    ui.previewImageLayout->addWidget(m_viewer);
    ui.stackedWidget->setCurrentIndex(0);

    // 视频输出窗口
    m_videoWidget = new QVideoWidget(ui.previewWidget);
    ui.previewFrameLayout->addWidget(m_videoWidget);
    // 图像捕获定时器
    m_timer = new QTimer(this);
    m_timer->setInterval(300);
    // 注意：启用深度识别时用时将会显著增加，需要控制定时器采集间隔
    connect(ui.tryHarderBox, &QCheckBox::stateChanged, this, [=](int state) {
        if (state != 0)
            m_timer->setInterval(1500);
        else
            m_timer->setInterval(300);
        });

#ifdef QT5VER
    connect(ui.cameraComBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QRCodeScanner::onCameraIndexChanged);
#else
    m_camera = new QCamera(this);
    // 相机状态变化
    connect(m_camera, &QCamera::errorOccurred, this, &QRCodeScanner::onCameraErrorOccurred);
    connect(ui.cameraComBox, &QComboBox::currentIndexChanged, this, &QRCodeScanner::onCameraIndexChanged);

    auto mediaDevices = new QMediaDevices(this);
    connect(mediaDevices, &QMediaDevices::videoInputsChanged, this, &QRCodeScanner::freshCameras);

    // 持续采集会话
    auto capture = new QMediaCaptureSession(this);
    capture->setCamera(m_camera);
    capture->setVideoOutput(m_videoWidget);

    // 图像捕获
    auto imageCapture = new QImageCapture(this);
    capture->setImageCapture(imageCapture);
    connect(m_timer, &QTimer::timeout, imageCapture, &QImageCapture::capture);
    connect(imageCapture, &QImageCapture::imageCaptured, this, &QRCodeScanner::recognImage);
#endif // QT5VER

    freshCameras();

    if (ui.cameraComBox->count() > 0)
    {
        ui.cameraComBox->setCurrentIndex(0);
    }

    // 显示结果
    connect(this, &QRCodeScanner::recognSuccess, this, &QRCodeScanner::onResultsRecieved);
    connect(this, &QRCodeScanner::recognOutline, this, &QRCodeScanner::onResultsOutline);
    connect(this, &QRCodeScanner::recognFailed, this, [=] {
        QMessageBox::warning(this, tr("提示"), tr("图片未识别到一维/二维码。"));
        });

    // TODO
    //auto fpsLabel = new QLabel("FPS: 0", this);
    //ui.statusBar->addPermanentWidget(fpsLabel);

    // 菜单->关闭
    connect(ui.action_quit, &QAction::triggered, this, &QMainWindow::close);
    // 菜单->打开图片
    connect(ui.action_open, &QAction::triggered, this, &QRCodeScanner::openImageFile);
    // 菜单->保存结果
    connect(ui.action_save, &QAction::triggered, this, &QRCodeScanner::saveResultToFile);
    // 菜单->打开QR生成器
    connect(ui.action_openQRG, &QAction::triggered, this, &QRCodeScanner::openQRGeneratorWidget);

    // 开始按钮
    connect(ui.startBtn, &QPushButton::clicked, this, [=] {
        ui.stackedWidget->setCurrentIndex(0);
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
    if (m_qrgWidget)
        m_qrgWidget->deleteLater();
}

void QRCodeScanner::freshCameras()
{
    auto cid = ui.cameraComBox->currentData();
    ui.cameraComBox->clear();

#ifdef QT5VER
    auto mediaDevices = QCameraInfo::availableCameras();
    for (auto &cameraDevice : mediaDevices)
    {
        auto id = cameraDevice.deviceName();
#else
    for (auto &cameraDevice : QMediaDevices::videoInputs())
    {
        auto id = cameraDevice.id();
#endif // QT5VER
        if (!cameraDevice.description().isEmpty())
        {
            ui.cameraComBox->addItem(cameraDevice.description(), id);
        }
        else
        {
            ui.cameraComBox->addItem(id, id);
        }
    }
    if (cid.isValid())
    {
        ui.cameraComBox->setCurrentIndex(ui.cameraComBox->findData(cid));
    }
}

void QRCodeScanner::recognImage(int id, const QImage & img)
{
    if (img.isNull())
        return;

    //using namespace ZXing;
    //using namespace ZXingQt;
    using ZXing::BarcodeFormat;

    auto task = [=] {
        QElapsedTimer elstimer;
        elstimer.start();

        ZXing::BarcodeFormats format = BarcodeFormat::None;
        if (ui.linearCodesBox->isChecked())
            format |= BarcodeFormat::LinearCodes;
        if (ui.matrixCodesBox->isChecked())
            format |= BarcodeFormat::MatrixCodes;

        // ZXing参数
        auto options = ZXing::ReaderOptions()
            // 识别的格式，Any = LinearCodes | MatrixCodes
            .setFormats(format)
            .setTryHarder(ui.tryHarderBox->isChecked())
            .setTryRotate(ui.tryRotateBox->isChecked())
            .setTryInvert(ui.tryInvertBox->isChecked())
            .setTextMode(ZXing::TextMode::HRI)
            .setCharacterSet(ZXing::CharacterSet::ASCII)
            .setMaxNumberOfSymbols(5);

        QStringList texts;
        QStringList types;
        QList<QPolygon> rects;

        //ZXing::ImageView image((const uint8_t *)img.constBits(), img.width(), img.height(), ZXing::ImageFormat::RGB);
        try
        {
            // 调用ZXing接口
            auto results = ZXingQt::ReadBarcodes(img, options);

            for (auto &result : results)
            {
                auto &pos = result.position();
                QPolygon polygon;
                //polygon.append({ pos[0].x, pos[0].y });
                //polygon.append({ pos[1].x, pos[1].y });
                //polygon.append({ pos[2].x, pos[2].y });
                //polygon.append({ pos[3].x, pos[3].y });
                polygon.append(pos[0]);
                polygon.append(pos[1]);
                polygon.append(pos[2]);
                polygon.append(pos[3]);

#ifdef QT_DEBUG
                qDebug() << "Text:    " << result.text();
                //qDebug() << "Format:  " << ZXing::ToString(result.format());
                //qDebug() << "Content: " << ZXing::ToString(result.contentType());
                qDebug() << "Position:" << polygon << Qt::endl;
#endif // QT_DEBUG

                // 保存结果
                texts.append(result.text());
                types.append(QString::fromStdString(ZXing::ToString(result.format())));
                rects += polygon;
            }
        }
        catch (const std::exception &e)
        {
            qWarning() << "识别失败:" << e.what();
        }

        if (!texts.isEmpty())
        {
            // 发送已识别信号
            emit recognSuccess(texts, types);
            emit recognOutline(img, rects);
        }
        else if (ui.stackedWidget->currentIndex() == 1)
        {
            emit recognFailed();
        }

        qDebug() << "time:" << elstimer.elapsed();
        };
    // 运行识别任务
    QThreadPool::globalInstance()->start(task);
}

void QRCodeScanner::saveResultToFile()
{
    auto text = ui.historyBrowser->toPlainText();
    if (text.isEmpty())
    {
        QMessageBox::warning(this, tr("警告"), tr("识别结果为空！"));
        return;
    }

    auto fileName = QFileDialog::getSaveFileName(this, tr("保存文本"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        + QDateTime::currentDateTime().toString("'/Results-'ddhhmmss'.txt'"),
        tr("文本文档 (*.txt);;所有文件 (*.*)"));

    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, tr("错误"), tr("文件创建失败：%1").arg(file.errorString()));
        return;
    }
    file.write(text.toUtf8());
    file.close();
    ui.statusBar->showMessage(tr("文件保存成功"));
}

void QRCodeScanner::openQRGeneratorWidget()
{
    if (m_qrgWidget == nullptr)
    {
        m_qrgWidget = new QRCodeGenerator();
    }
    m_qrgWidget->activateWindow();
    m_qrgWidget->show();
}

void QRCodeScanner::openImageFile()
{
    if (ui.stopBtn->isEnabled())
        ui.stopBtn->click();

    static bool open_last = false;
    QString path;
    if (!open_last)
        path = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    auto fileNames = QFileDialog::getOpenFileNames(this, tr("选择图片"), path, "图片 (*.jpg *.png *.bmp *.jpeg *.webp);; 所有文件 (*.*)");
    for (auto &file : fileNames)
    {
        open_last = true;
        QImage img(file);
        if (img.isNull())
        {
            QMessageBox::critical(this, tr("错误"), tr("图片文件无效：%1").arg(file));
            return;
        }
        m_viewer->setImage(img);
        ui.stackedWidget->setCurrentIndex(1);
        recognImage(0, img);
    }
}

// 相机选择
void QRCodeScanner::onCameraIndexChanged(int index)
{
    if (m_camera)
        m_camera->stop();
    ui.startBtn->setEnabled(false);
    ui.stopBtn->setEnabled(false);

#ifdef QT5VER
    auto devices = QCameraInfo::availableCameras();
    for (auto &d : devices)
    {
        if (d.deviceName() == ui.cameraComBox->currentData().toString())
        {
            if (m_camera)
                m_camera->deleteLater();
            m_camera = new QCamera(d, this);
            // 相机就绪
            if (m_camera->isAvailable())
            {
                ui.startBtn->setEnabled(true);
                ui.statusBar->showMessage(tr("就绪"));
            }
            // 相机发生错误
            connect(m_camera, &QCamera::errorOccurred, this, &QRCodeScanner::onCameraErrorOccurred);
            // 设置相机流输出
            m_camera->setViewfinder(m_videoWidget);
            m_camera->setCaptureMode(QCamera::CaptureStillImage);
            // 图像捕获
            auto imageCapture = new QCameraImageCapture(m_camera, this);
            // 图像不保存到文件
            imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
            connect(m_timer, &QTimer::timeout, imageCapture, [=] {
                m_camera->searchAndLock();
                imageCapture->capture();
                m_camera->unlock();
                });
            connect(imageCapture, &QCameraImageCapture::imageCaptured, this, &QRCodeScanner::recognImage);
            connect(m_camera, &QCamera::destroyed, imageCapture, &QCameraImageCapture::deleteLater);
            break;
        }
    }
#else
    auto devices = QMediaDevices::videoInputs();
    for (auto &d : devices)
    {
        if (d.id() == ui.cameraComBox->currentData().toByteArray())
        {
            m_camera->setCameraDevice(d);
            if (m_camera->isAvailable())
            {
                ui.startBtn->setEnabled(true);
                ui.statusBar->showMessage(tr("就绪"));
            }
            break;
        }
    }
#endif // QT5VER
}

void QRCodeScanner::onCameraErrorOccurred()
{
    // 调用停止
    ui.stopBtn->click();
    ui.startBtn->setEnabled(false);
    // 显示错误
    QMessageBox::critical(this, tr("错误"), m_camera->errorString());
    ui.statusBar->showMessage(tr("相机发生错误"));
}

void QRCodeScanner::onResultsRecieved(const QStringList & texts, const QStringList & types)
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

void QRCodeScanner::onResultsOutline(const QImage & img, const QList<QPolygon> &rects) const
{
    QImage rsimg = img;
    QPainter p(&rsimg);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath pp;
    pp.addRect(rsimg.rect());

    QFont font = qApp->font();
    font.setPointSizeF(24);
    //font.setBold(true);
    p.setFont(font);
    p.setPen(QPen(Qt::cyan, 5));
    p.setRenderHint(QPainter::TextAntialiasing);

    // 标记识别到的位置
    for (int i = 0; i < rects.size(); i++)
    {
        auto &r = rects[i];
        pp.addPolygon(r);
        p.drawPolygon(r);
        p.drawText((r.at(0) + r.at(2)) * 0.5, QString("[ %1 ]").arg(i + 1));
    }
    p.fillPath(pp, QColor(0, 0, 0, 128));
    p.end();

    // 显示图片
    m_viewer->setImage(rsimg);
    ui.stackedWidget->setCurrentIndex(1);
}
