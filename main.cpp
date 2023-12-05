#include <QFile>
#include <QApplication>
#include <QScreen>

#include "mainwindow.h"
#include "timer.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    Timer timer;
    QScreen *screen = a.primaryScreen();
    QIcon icon(":/resources/logo.png");

    int width = w.frameGeometry().width();
    int height = w.frameGeometry().height();

    int screenWidth = screen->geometry().width();
    int screenHeight = screen->geometry().height();

    w.timer = &timer;
    w.timer->time_read();

    a.setWindowIcon(icon);
    w.setWindowTitle("Communa - TimeTracker");
    w.setGeometry((screenWidth/2)-(width/2), (screenHeight/2)-(height/2), width, height);

    w.show();

    return a.exec();
}
