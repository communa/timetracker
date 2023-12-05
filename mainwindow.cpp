#include <QString>
#include <QFile>
#include <QCloseEvent>
#include <QPixmap>

#include "auth.h"
#include "mainwindow.h"
#include "timer.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    Auth auth;

    ui->setupUi(this);
    _timer = new QTimer(this);

    QPixmap pm = auth.buildQrCode();

    qDebug() << pm;


    ui->label->setPixmap(pm);

    connect(
        _timer,
        SIGNAL(timeout()),
        this,
        SLOT(on_tick())
    );

    this->show_login();
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
    ui->timeLabel->setText(text);

    qDebug() << text;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    MainWindow::timer->time_write();

    event->accept();
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
        ui->timerButton->setText("Start Timer");
    }
}

void MainWindow::on_buttonAuthCreate_clicked()
{
    this->show_timer();
}

void MainWindow::on_buttonAuthRestore_clicked()
{
    this->show_timer();
}

void MainWindow::on_buttonAuthLogout_clicked()
{
    this->show_login();
}

void MainWindow::show_login()
{
    ui->frameTimer->hide();
    ui->frameLogin->show();
    ui->frameTimer->setGeometry(0,0, 600, 800);

    this->setFixedSize(440, 690);
}

void MainWindow::show_timer()
{
    ui->frameTimer->show();
    ui->frameLogin->hide();
    ui->frameTimer->setGeometry(0,0, 600, 800);

    this->setFixedSize(400, 460);
}
