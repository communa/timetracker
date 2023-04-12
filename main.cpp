#include <QFile>
#include <QApplication>
#include "mainwindow.h"
#include "timer.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    Timer timer;
    w.timer = &timer;

    QIcon icon("/Users/ivan/code/communa/CommunaTimeTracker/resources/logo.png");

    a.setWindowIcon(icon);

    w.setWindowTitle("Communa / TimeTracker");
    w.show();

    return a.exec();
}
