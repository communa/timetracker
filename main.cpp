#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    QIcon icon("/Users/ivan/code/communa/CommunaTimeTracker/resources/logo.png");

    a.setWindowIcon(icon);

    w.setWindowTitle("Communa / TimeTracker");
    w.show();

    return a.exec();
}
