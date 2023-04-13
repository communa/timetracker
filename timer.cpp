#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include "timer.h"

Timer::Timer()
{
    timer = new QTimer(this);
    this->filePath = QDir::homePath() + "/communa.txt";

    connect(
        timer,
        SIGNAL(timeout()),
        this,
        SLOT(TimerSlot())
    );
}

void Timer::TimerSlot()
{
    this->time++;
    qDebug() << this->time;
}

void Timer::start() {
    this->isStarted = true;
    this->time_read();

    timer->start(1000);
}

void Timer::stop() {
    this->isStarted = false;
    this->time_write();

    timer->stop();
}

void Timer::time_write()
{
    QFile file(this->filePath);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);

    out << QString::number(this->time);

    file.close();
}

void Timer::time_read()
{
    QFile file(this->filePath);
    QString s = "";

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);

        while (!in.atEnd()) {
            s += in.readLine();
        }
    }

    this->time = s.toInt();
}
