# QRCodeScanner

基于 [zxing-cpp](https://github.com/zxing-cpp/zxing-cpp) 库写的一维码、二维码识别工具，需要调用相机采集图像，识别后标记识别的区域。

zxing-cpp库本身未引用Qt库，仅通过定义头文件`ZXingQtReader.h`实现相应的转换。
