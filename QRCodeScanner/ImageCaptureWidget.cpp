#include "ImageCaptureWidget.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

ImageCaptureWidget::ImageCaptureWidget(QImageCapture *imgCapture, QWidget *parent)
    : QWidget(parent)
    , m_imgCapture(imgCapture)
{
    ui.setupUi(this);

    connect(m_imgCapture, &QImageCapture::imageCaptured, this, [=](int id, const QImage &img) {
        m_image = img;
        ui.previewLabel->setPixmap(QPixmap::fromImage(img).scaled(ui.previewLabel->size(), Qt::KeepAspectRatio));
        ui.imgidLabel->setText(QString("ID: %1").arg(id));
            });

    connect(m_imgCapture, &QImageCapture::errorOccurred, this, [=](int id, QImageCapture::Error error, const QString &errorString) {
        QMessageBox::critical(this, tr("错误"), tr("捕获图像(id: %1)失败：%2").arg(id).arg(errorString));
            });

    connect(ui.captureBtn, &QPushButton::clicked, m_imgCapture, &QImageCapture::capture);

    connect(ui.saveBtn, &QPushButton::clicked, this, [=] {
        if (m_image.isNull())
            return;

        auto fileName = QFileDialog::getSaveFileName(this, tr("保存图片"),
                                                     QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                                     tr("图片 (*.png *.jpg *.bmp)"));

        if (!fileName.isEmpty())
        {
            m_image.save(fileName);
        }
            });
}

ImageCaptureWidget::~ImageCaptureWidget()
{}

void ImageCaptureWidget::resizeEvent(QResizeEvent * e)
{
    QWidget::resizeEvent(e);
    if (!m_image.isNull())
    {
        ui.previewLabel->setPixmap(QPixmap::fromImage(m_image).scaled(ui.previewLabel->size(), Qt::KeepAspectRatio));
    }
}
