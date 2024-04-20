# QRCodeScanner

基于 [zxing-cpp](https://github.com/zxing-cpp/zxing-cpp) 库的一维码、二维码识别与生成工具，支持打开文件与相机采集方式识别，识别后标记识别的区域。

zxing-cpp本身未引用Qt库，仅通过定义头文件`ZXingQtReader.h`实现相应接口的转换。

已测试环境：VS2022 + Qt5.15.2 / Qt6.5.3
