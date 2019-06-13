#include "ytxTester.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ytxTester w;

    QString fontPath = ":/fonts/Ubuntu-L.ttf";
    int fontId = QFontDatabase::addApplicationFont(fontPath);
    if (fontId != -1)
    {
        QFont font("Ubuntu-L");
        a.setFont(font);
    }

    w.show();

    return a.exec();
}
