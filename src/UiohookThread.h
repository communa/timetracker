#ifndef UIOHOOKTHREAD_H
#define UIOHOOKTHREAD_H

#include <QThread>
#include <QReadWriteLock>

struct _uiohook_event;

class UiohookThread : public QThread
{
    Q_OBJECT
public:
    explicit UiohookThread(QObject *parent = nullptr);
    ~UiohookThread() override;

    void peek(int &strokes, int &clicks, int &odometer) const;
    void withdraw(int &strokes, int &clicks, int &odometer);
    void clear();

signals:
    void error(const QString &text);

protected:
    void run() override;

private:
    static void dispatch(struct _uiohook_event *const event);

    int strokes;
    int clicks;
    int odometer;
    mutable QReadWriteLock mutex;
};

#endif // UIOHOOKTHREAD_H
