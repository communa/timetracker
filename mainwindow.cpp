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
    _timer = new QTimer(this);

    connect(
        _timer,
        SIGNAL(timeout()),
        this,
        SLOT(on_tick())
    );
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_tick()
{
    MainWindow::timer->time++;
    QString text = "Time tracked: " + QString::number(MainWindow::timer->time);

    _timer->start(1000);
    ui->timerLabel->setText(text);

    qDebug() << text;
}

void MainWindow::on_timerButton_clicked()
{
    if (!_timer->isActive()) {
        _timer->start(1000);
        MainWindow::timer->isStarted = false;
        ui->timerButton->setText("Stop");
    } else {
        _timer->stop();
        MainWindow::timer->isStarted = true;
        ui->timerButton->setText("Start");
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    MainWindow::timer->time_write();

    event->accept();
}
