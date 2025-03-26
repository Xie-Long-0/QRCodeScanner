#include "QRCodeScanner.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QRCodeScanner w;
    w.show();
    return a.exec();
}
