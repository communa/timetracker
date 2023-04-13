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
    w.timer->time_read();

    QIcon icon(":/resources/logo.png");

    a.setWindowIcon(icon);
    w.setWindowTitle("Communa / TimeTracker");
    w.show();

    return a.exec();
}
