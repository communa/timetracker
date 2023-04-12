#include <QString>
#include <QFile>
#include <QCloseEvent>
#include <string>

#include "mainwindow.h"
#include "timer.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_timerButton_clicked()
{
    if (!MainWindow::timer->isStarted) {
        MainWindow::timer->start();
        ui->timerButton->setText("Stop");
    } else {
        MainWindow::timer->stop();
        ui->timerButton->setText("Start");
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    MainWindow::timer->stop();

    event->accept();
}
