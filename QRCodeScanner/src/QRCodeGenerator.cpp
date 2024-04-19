#include "QRCodeGenerator.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

#include "ZXing/MultiFormatWriter.h"
#include "ZXing/BitMatrix.h"
#include "ZXing/Matrix.h"
#include "ImageView.h"

QRCodeGenerator::QRCodeGenerator(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    m_viewer = new ImageView(ui.viewWidget);
    auto layout = new QHBoxLayout(ui.viewWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_viewer);
    ui.viewWidget->setLayout(layout);

    ui.typeComboBox->addItem(tr("二维码 (QRCode)"), (int)ZXing::BarcodeFormat::QRCode);
    ui.typeComboBox->addItem(tr("二维码 (DataMatrix)"), (int)ZXing::BarcodeFormat::DataMatrix);
    ui.typeComboBox->addItem(tr("二维码 (PDF417)"), (int)ZXing::BarcodeFormat::PDF417);
    ui.typeComboBox->addItem(tr("二维码 (Aztec)"), (int)ZXing::BarcodeFormat::Aztec);

    ui.typeComboBox->addItem(tr("条形码 (Code128)"), (int)ZXing::BarcodeFormat::Code128);
    ui.typeComboBox->addItem(tr("条形码 (EAN8)"), (int)ZXing::BarcodeFormat::EAN8);
    ui.typeComboBox->addItem(tr("条形码 (EAN13)"), (int)ZXing::BarcodeFormat::EAN13);
    ui.typeComboBox->addItem(tr("条形码 (Codabar)"), (int)ZXing::BarcodeFormat::Codabar);
    ui.typeComboBox->addItem(tr("条形码 (Code39)"), (int)ZXing::BarcodeFormat::Code39);
    ui.typeComboBox->addItem(tr("条形码 (Coda93)"), (int)ZXing::BarcodeFormat::Code93);
    ui.typeComboBox->addItem(tr("条形码 (ITF)"), (int)ZXing::BarcodeFormat::ITF);
    ui.typeComboBox->addItem(tr("条形码 (UPCA)"), (int)ZXing::BarcodeFormat::UPCA);
    ui.typeComboBox->addItem(tr("条形码 (UPCE)"), (int)ZXing::BarcodeFormat::UPCE);
    ui.typeComboBox->setCurrentIndex(0);

    connect(ui.generateBtn, &QPushButton::clicked, this, &QRCodeGenerator::onGenerateBtnClicked);
    connect(ui.saveImgBtn, &QPushButton::clicked, this, &QRCodeGenerator::onSaveImgBtnClicked);
}

QRCodeGenerator::~QRCodeGenerator()
{}

void QRCodeGenerator::onGenerateBtnClicked()
{
    QString content = ui.textEdit->toPlainText();
    if (content.isEmpty())
    {
        QMessageBox::warning(this, tr("错误"), tr("内容为空！"));
        return;
    }

    int width = ui.widthSpinBox->value();
    int height = ui.heightSpinBox->value();
    int margin = ui.marginSpinBox->value();

    ZXing::BitMatrix bitMatrix;
    try
    {
        ZXing::MultiFormatWriter writer(ZXing::BarcodeFormat(ui.typeComboBox->currentData().toInt()));
        writer.setEncoding(ZXing::CharacterSet::UTF8);
        writer.setEccLevel(ui.eccLevelComboBox->currentIndex());
        writer.setMargin(margin);

        bitMatrix = writer.encode(content.toStdString(), width, height);
    }
    catch (const std::invalid_argument &e)
    {
        QMessageBox::critical(this, tr("错误"), tr("生成失败！\n输入的内容不符合该编码生成限制。\n%1").arg(e.what()));
        return;
    }

    // 需要转换为uint8(unsigned char)类型的数据才可使用
    auto result = ZXing::ToMatrix<uint8_t>(bitMatrix);

    QImage img(result.data(), result.width(), result.height(), QImage::Format_Indexed8);
    m_qrImg = img.convertToFormat(QImage::Format_ARGB32);
    m_viewer->setImage(m_qrImg);
}

void QRCodeGenerator::onSaveImgBtnClicked()
{
    if (m_qrImg.isNull())
    {
        QMessageBox::warning(this, tr("错误"), tr("图像为空！"));
        return;
    }

    static bool last = false;
    QString path;
    if (!last)
        path = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    QString fileName = QFileDialog::getSaveFileName(this, tr("保存图片"), path, "图片 (*.jpg *.png *.bmp *.jpeg *.webp)");
    if (!fileName.isEmpty())
    {
        last = true;
        if (m_qrImg.save(fileName))
        {
            QMessageBox::information(this, tr("提示"), tr("图像保存成功！"));
        }
        else
        {
            QMessageBox::critical(this, tr("错误"), tr("图像保存失败！\n文件：%1").arg(fileName));
        }
    }
}
