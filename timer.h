#ifndef MYTIMER_H
#define MYTIMER_H

#include <QTimer>
#include <QFile>

class Timer : public QObject
{
    Q_OBJECT

public:
    Timer();
    int time = 0;
    bool isStarted = false;

    void start();
    void stop();
    void time_write();
    void time_read();

protected:
    QTimer *timer;
    std::string filePath;

protected slots:
    void TimerSlot();
};

#endif // MYTIME
