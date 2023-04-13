#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include "timer.h"

Timer::Timer()
{
    this->filePath = QDir::homePath() + "/communa.txt";
}

void Timer::time_read() {
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

void Timer::time_write() {
    QFile file(this->filePath);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);

    out << QString::number(this->time);

    file.close();
}
