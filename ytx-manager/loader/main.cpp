#include "loader.h"
#include <QApplication>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QPixmap pixmap(":/img/splash.jpeg");
    QSplashScreen splash(pixmap);
    splash.show();

    // Load the embedded font.
    QString fontPath = ":/fonts/Ubuntu-L.ttf";
    int fontId = QFontDatabase::addApplicationFont(fontPath);
    if (fontId != -1)
    {
        QFont font("Ubuntu-L");
        a.setFont(font);
    }

    loader w;
    w.setWindowIcon(QIcon(":/img/icon.ico"));
    QTimer::singleShot(2500, &splash, SLOT(close()));
    QTimer::singleShot(2500, &w, SLOT(show()));



    return a.exec();
}
