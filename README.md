# Lab2QRCode

**Lab2QRCode** 是一款 Windows 平台下的工具，支持将**任意二进制或文本文件** 转换为 **QRCode 图片**，以及将 **QRCode 图片** 解码回普通文件。


为了确保兼容性，无论是生成二维码还是解码二维码，程序都会先进行 **Base64 编码** 处理。这样可以避免控制字符等特殊字符影响数据的正确性，确保转换过程中的数据完整性。

如果你希望在自己的项目中使用 `Base64` 解码功能，可以参考本项目中的实现，具体代码见 [`SimpleBase64.h`](./include/SimpleBase64.h)。

## 构建

它使用 `cmake` 管理项目，依赖项：

- **[`zxing-cpp`](https://github.com/zxing-cpp/zxing-cpp)**
- **`OpenCV4`**
- **`Qt5`**

使用 visual studio 17 工具链构建。

```cmake
git clone https://gitee.com/Mq-b/Lab2QRCode
mkdir build
cd build
cmake ..
cmake --build . -j --config Release
```

构建完成后，在 `build\Release\bin\` 目录下会生成 `Lab2QRCode.exe` 可执行文件。

## 使用

![Lab2QRCode](./images/Lab2QRCode.gif)

### 读取 `QRCode` 图片还原文件

1. 点击“**浏览**”按钮选择一个 QRCode 图片文件（`.png` 格式）

2. 点击“生成化验表”按钮，会弹出一个选择保存文件的对话框，选择保存路径后会将 QRCode 图片还原为原始文件，默认文件后缀为 `.rfa`。
