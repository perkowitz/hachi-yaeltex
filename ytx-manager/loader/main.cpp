#include "loader.h"
#include <QApplication>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //QTranslator translator;
    // look up e.g. :/translations/myapp_de.qm
    //translator.load(QLocale(), QLatin1String("octaManager"), QLatin1String("_"), QLatin1String(":/translations"));
    //a.installTranslator(&translator);

    QPixmap pixmap(":/img/splash.png");
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
