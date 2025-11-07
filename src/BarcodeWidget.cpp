#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QPixmap>
#include <QMessageBox>
#include <QFont>
#include <opencv2/opencv.hpp>
#include <ZXing/BarcodeFormat.h>
#include <ZXing/BitMatrix.h>
#include <ZXing/MultiFormatWriter.h>
#include <ZXing/TextUtfEncoding.h>
#include <ZXing/ReadBarcode.h>
#include <ZXing/DecodeHints.h>
#include <SimpleBase64.h>
#include "BarcodeWidget.h"

QImage BarcodeWidget::MatToQImage(const cv::Mat& mat)
{
    if (mat.type() == CV_8UC1)
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    else if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    }
    return QImage();
}

BarcodeWidget::BarcodeWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("Binary to QRCode Generator");
    setMinimumSize(500, 500);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QFont font;
    font.setPointSize(11);
    setFont(font);

    // 文件路径 + 按钮
    auto* fileLayout = new QHBoxLayout();
    filePathEdit = new QLineEdit(this);
    filePathEdit->setPlaceholderText("选择一个二进制或文本文件...");
    filePathEdit->setFont(QFont("Arial", 15));  // 设置加粗字体

    QPushButton* browseButton = new QPushButton("浏览", this);
    browseButton->setFixedWidth(100);
    browseButton->setFont(QFont("Arial", 15));  // 设置加粗字体
    fileLayout->addWidget(filePathEdit);
    fileLayout->addWidget(browseButton);
    mainLayout->addLayout(fileLayout);

    // 生成与保存按钮
    auto* buttonLayout = new QHBoxLayout();
    generateButton = new QPushButton("生成 QRCode", this);
    decodeToChemFile = new QPushButton("生成化验表");
    saveButton = new QPushButton("保存图片", this);
    generateButton->setFixedHeight(40);
    saveButton->setFixedHeight(40);
    decodeToChemFile->setFixedHeight(40);
    generateButton->setFont(QFont("Arial", 15));
    decodeToChemFile->setFont(QFont("Arial", 15));
    saveButton->setFont(QFont("Arial", 15));
    saveButton->setEnabled(false);
    buttonLayout->addWidget(generateButton);
    buttonLayout->addWidget(decodeToChemFile);
    buttonLayout->addWidget(saveButton);
    mainLayout->addLayout(buttonLayout);

    // 图片展示
    barcodeLabel = new QLabel(this);
    barcodeLabel->setAlignment(Qt::AlignCenter);
    barcodeLabel->setMinimumHeight(320);
    barcodeLabel->setStyleSheet("QLabel { background-color: #f0f0f0; border: 1px solid #ccc; }");
    mainLayout->addWidget(barcodeLabel);

    connect(browseButton, &QPushButton::clicked, this, &BarcodeWidget::onBrowseFile);
    connect(generateButton, &QPushButton::clicked, this, &BarcodeWidget::onGenerateClicked);
    connect(decodeToChemFile, &QPushButton::clicked, this, &BarcodeWidget::onDecodeToChemFileClicked);
    connect(saveButton, &QPushButton::clicked, this, &BarcodeWidget::onSaveClicked);
}

void BarcodeWidget::onBrowseFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select File", "", "RFA Files (*.rfa);;Image Files (*.png);;All Files (*)");
    if (!fileName.isEmpty())
        filePathEdit->setText(fileName);
}

void BarcodeWidget::onGenerateClicked()
{
    QString filePath = filePathEdit->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择一个文件.");
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "错误", "不能打开文件.");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    try {
        std::string text = SimpleBase64::encode(reinterpret_cast<const std::uint8_t*>(data.constData()), data.size());

        ZXing::MultiFormatWriter writer(ZXing::BarcodeFormat::QRCode);
        writer.setMargin(1);

        auto bitMatrix = writer.encode(text, 300, 300);
        int width = bitMatrix.width();
        int height = bitMatrix.height();
        cv::Mat img(height, width, CV_8UC1);

        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                img.at<uint8_t>(y, x) = bitMatrix.get(x, y) ? 0 : 255;

        lastImage = MatToQImage(img);
        barcodeLabel->setPixmap(QPixmap::fromImage(lastImage).scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        saveButton->setEnabled(true);
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Failed to generate QRCode:\n%1").arg(e.what()));
    }
}

void BarcodeWidget::onDecodeToChemFileClicked()
{
    QString filePath = filePathEdit->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择一个PNG图片文件.");
        return;
    }

    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    QStringList imageFormats = { "png" };

    if (!imageFormats.contains(suffix)) {
        QMessageBox::warning(this, "警告", "选择的文件不是PNG图片格式。\n请选择300x300像素的PNG格式图片");
        return;
    }

    try {
        cv::Mat img = cv::imread(filePath.toStdString(), cv::IMREAD_COLOR);
        if (img.empty()) {
            QMessageBox::critical(this, "错误", "无法加载图片文件。");
            return;
        }

        // 转换为灰度图
        cv::Mat grayImg;
        cv::cvtColor(img, grayImg, cv::COLOR_BGR2GRAY);

        // 使用ZXing解码QR码
        ZXing::ImageView imageView(grayImg.data, grayImg.cols, grayImg.rows, ZXing::ImageFormat::Lum);
        auto result = ZXing::ReadBarcode(imageView);

        if (!result.isValid()) {
            QMessageBox::warning(this, "警告", "无法识别QR码或QR码格式不正确。");
            return;
        }

        // 获取解码后的文本并Base64解码
        std::string encodedText = result.text();
        auto decodedData = SimpleBase64::decode(encodedText);

        // 弹出保存文件对话框
        QString defaultName = fileInfo.completeBaseName() + ".rfa";
        QString savePath = QFileDialog::getSaveFileName(this, "保存化验表文件",
            fileInfo.dir().filePath(defaultName),
            "RFA Files (*.rfa)");

        if (savePath.isEmpty()) {
            return; // 用户取消保存
        }

        // 将解码后的数据写入文件
        QFile outputFile(savePath);
        if (!outputFile.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "错误", "无法创建输出文件。");
            return;
        }

        outputFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
        outputFile.close();

        QMessageBox::information(this, "成功", QString("化验表文件已保存至:\n%1").arg(savePath));

    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, "错误", QString("解码失败:\n%1").arg(e.what()));
    }
}

void BarcodeWidget::onSaveClicked()
{
    if (lastImage.isNull()) return;
    const QString saveFile = filePathEdit->text();
    const QFileInfo fileInfo(saveFile);
    const QString fileNameWithoutExtension = fileInfo.baseName();

    const QString fileName = QFileDialog::getSaveFileName(this, "Save QRCode", fileNameWithoutExtension + ".png", "PNG Images (*.png)");
    if (!fileName.isEmpty()) {
        if (lastImage.save(fileName))
            QMessageBox::information(this, "保存", QString("图片保存成功 %1").arg(fileName));
        else
            QMessageBox::warning(this, "错误", "无法保存图片.");
    }
}
