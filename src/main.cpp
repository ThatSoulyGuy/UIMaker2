#include "app/MainWindow.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Identity for QSettings so preferences (last-used directory, last opened
    // file) persist to a stable per-user location.
    QApplication::setOrganizationName("UIMaker");
    QApplication::setApplicationName("UIMaker2");

    MainWindow w;
    w.show();
    return a.exec();
}
