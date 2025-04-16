#include "QRCodeGenerator.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

#include "ZXing/WriteBarcode.h"
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
    ui.typeComboBox->addItem(tr("二维码 (MicroQRCode)"), (int)ZXing::BarcodeFormat::MicroQRCode);
    ui.typeComboBox->addItem(tr("二维码 (RMQRCode)"), (int)ZXing::BarcodeFormat::RMQRCode);
    ui.typeComboBox->addItem(tr("二维码 (DataMatrix)"), (int)ZXing::BarcodeFormat::DataMatrix);
    ui.typeComboBox->addItem(tr("二维码 (PDF417)"), (int)ZXing::BarcodeFormat::PDF417);
    ui.typeComboBox->addItem(tr("二维码 (Aztec)"), (int)ZXing::BarcodeFormat::Aztec);
    ui.typeComboBox->addItem(tr("二维码 (MaxiCode)"), (int)ZXing::BarcodeFormat::MaxiCode);

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

    ui.eccLevelComboBox->addItem("0", "0");
    ui.eccLevelComboBox->addItem("1 (L~7%)", "1");
    ui.eccLevelComboBox->addItem("2 (M~15%)", "2");
    ui.eccLevelComboBox->addItem("3 (Q~25%)", "3");
    ui.eccLevelComboBox->addItem("4 (H~30%)", "4");
    ui.eccLevelComboBox->addItem("5 (~35%)", "5");
    ui.eccLevelComboBox->addItem("6 (~40%)", "6");
    ui.eccLevelComboBox->addItem("7 (~45%)", "7");
    ui.eccLevelComboBox->addItem("8 (~50%)", "8");
    ui.eccLevelComboBox->setCurrentIndex(1);

    ui.rotateComboBox->addItem(tr("不旋转"), 0);
    ui.rotateComboBox->addItem(tr("旋转90度"), 90);
    ui.rotateComboBox->addItem(tr("旋转180度"), 180);
    ui.rotateComboBox->addItem(tr("旋转270度"), 270);
    ui.rotateComboBox->setCurrentIndex(0);

    connect(ui.generateBtn, &QPushButton::clicked, this, &QRCodeGenerator::onGenerateBtnClicked);
    connect(ui.saveImgBtn, &QPushButton::clicked, this, &QRCodeGenerator::onSaveImgBtnClicked);
}

// Aztec Code ECC Level map to 0~8
// 理论上 Aztec Code 纠错级别支持 1% ~ 99%，但在ZXing内只支持 10%, 23%, 36%, 50% 4个级别
static std::string AztecEcl[9] = { "1%", "7%", "15%", "25%", "30%", "35%", "40%", "45%", "50%" };

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

    auto format = ZXing::BarcodeFormat(ui.typeComboBox->currentData().toInt());
    std::string ecl = ZXing::IsLinearBarcode(format) ?
        "" : ui.eccLevelComboBox->currentData().toString().toStdString();
    if (format == ZXing::BarcodeFormat::Aztec)
        ecl = AztecEcl[ui.eccLevelComboBox->currentIndex()];

    auto crtOpt = ZXing::CreatorOptions(format).ecLevel(ecl);
    auto wrtOpt = ZXing::WriterOptions().rotate(ui.rotateComboBox->currentData().toInt())
        .sizeHint(width).withHRT(true);

    try
    {
        auto barcode = ZXing::CreateBarcodeFromText(content.toStdString(), crtOpt);
        auto result = ZXing::WriteBarcodeToImage(barcode, wrtOpt);
        QImage img(result.data(), result.width(), result.height(), result.width(), QImage::Format_Grayscale8);
        m_qrImg = img.copy();
        m_viewer->setImage(m_qrImg);
    }
    catch (const std::invalid_argument &e)
    {
        QMessageBox::critical(this, tr("错误"), tr("生成失败！\n输入的内容或选项不符合该编码规范。\n%1").arg(e.what()));
        return;
    }
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
