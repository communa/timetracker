#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.setWindowIcon(QIcon("logo.png"));
    w.setWindowTitle("Communa / TimeTracker");
    w.show();

    w.show();


    return a.exec();
}
