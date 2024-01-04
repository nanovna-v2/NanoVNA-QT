#include "mainwindow.H"
#include "calkitsettings.H"
#include <QApplication>
#include <QtPlugin>
#include <QTranslator>
#include <QLibraryInfo>
#include <QMetaType>

int main(int argc, char *argv[])
{
    qRegisterMetaType<string>("string");
    qRegisterMetaType<CalKitSettings>("CalKitSettings");
    //qRegisterMetaTypeStreamOperators<CalKitSettings>("CalKitSettings");

    QCoreApplication::setApplicationName("NanoVNA QT GUI");

    QApplication app(argc, argv);
    app.setStyle("fusion");


    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
            QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    QTranslator myappTranslator;
    myappTranslator.load("languages/vna_qt_" + QLocale::system().name());
    fprintf(stderr, "%s\n", QLocale::system().name().toStdString().c_str());
    app.installTranslator(&myappTranslator);


    MainWindow* w = new MainWindow();
    w->show();

    return app.exec();
}
