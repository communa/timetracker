#include <QString>
#include <QFile>
#include <string>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_timerButton_clicked()
{
    MainWindow::timer++;
    QString s = QString::number(MainWindow::timer);
    ui->timerButton->setText(s);

    QFile file("/Users/ivan/code/communa/out.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);

    out << s;

    // optional, as QFile destructor will already do it:
    file.close();
}
