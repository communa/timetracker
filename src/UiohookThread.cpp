#include <QtDebug>

#include <stdio.h>
#include <inttypes.h>

#include "UiohookThread.h"
#include "uiohook.h"
#include "ActivityCollector.h"

#define TRACE_UIOHOOKTHREAD
#ifdef TRACE_UIOHOOKTHREAD
#include <QTime>
#include <QtDebug>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

UiohookThread::UiohookThread(QObject *parent)
    : QThread(parent)
    , strokes(0)
    , clicks(0)
    , odometer(0)
{
    TRACE();

    setTerminationEnabled(true);
}

UiohookThread::~UiohookThread()
{
    bool cancel = isRunning();

    TRACE_ARG(cancel);

    if (cancel) {
        setTerminationEnabled(true);
        int status = hook_stop();
        switch (status) {
        case UIOHOOK_SUCCESS: // Everything is ok
            break;
        case UIOHOOK_ERROR_OUT_OF_MEMORY: // System level errors
            emit error(QStringLiteral("Failed to allocate memory, status=") + QString::number(status));
            break;
        case UIOHOOK_ERROR_X_RECORD_GET_CONTEXT: // platform specific error that occurs on hook_stop()
            emit error(QStringLiteral("Failed to get XRecord context, status=") + QString::number(status));
            break;
        case UIOHOOK_FAILURE: // Default error
        default:
            emit error(QStringLiteral("An unknown hook error occurred, status=") + QString::number(status));
            break;
        }
        while (!wait(200)) {
            qWarning() << Q_FUNC_INFO << "Thread is not stopped, force to terminate...";
            terminate();
        }
    }
}

void UiohookThread::run()
{
    TRACE();

    hook_set_dispatch_proc(dispatch);

    // Start the hook and block. If EVENT_HOOK_ENABLED was delivered, the status will always succeed.
    int status = hook_run();

    TRACE_ARG(status);

    switch (status) {
    case UIOHOOK_SUCCESS: // Everything is ok
        qWarning() << Q_FUNC_INFO << "Thread stopped";
        break;
    case UIOHOOK_ERROR_OUT_OF_MEMORY: // System level errors
        emit error(QStringLiteral("Failed to allocate memory, status=") + QString::number(status));
        break;
    case UIOHOOK_ERROR_X_OPEN_DISPLAY: // X11 specific errors
        emit error(QStringLiteral("Failed to open X11 display, status=") + QString::number(status));
        break;
    case UIOHOOK_ERROR_X_RECORD_NOT_FOUND:
        emit error(QStringLiteral("Unable to locate XRecord extension, status=") + QString::number(status));
        break;
    case UIOHOOK_ERROR_X_RECORD_ALLOC_RANGE:
        emit error(QStringLiteral("Unable to allocate XRecord range, status=") + QString::number(status));
        break;
    case UIOHOOK_ERROR_X_RECORD_CREATE_CONTEXT:
        emit error(QStringLiteral("Unable to allocate XRecord context, status=") + QString::number(status));
        break;
    case UIOHOOK_ERROR_X_RECORD_ENABLE_CONTEXT:
        emit error(QStringLiteral("Failed to enable XRecord context, status=") + QString::number(status));
        break;
    case UIOHOOK_ERROR_SET_WINDOWS_HOOK_EX: // Windows specific errors
        emit error(QStringLiteral("Failed to register low level windows hook, status=") + QString::number(status));
        break;
    case UIOHOOK_ERROR_AXAPI_DISABLED: // Darwin specific errors
        emit error(QStringLiteral("Failed to enable access for assistive devices, status=") + QString::number(status));
        break;
    case UIOHOOK_ERROR_CREATE_EVENT_PORT:
        emit error(QStringLiteral("Failed to create apple event port, status=") + QString::number(status));
        break;
    case UIOHOOK_ERROR_CREATE_RUN_LOOP_SOURCE:
        emit error(QStringLiteral("Failed to create apple run loop source, status=") + QString::number(status));
        break;
    case UIOHOOK_ERROR_GET_RUNLOOP:
        emit error(QStringLiteral("Failed to acquire apple run loop, status=") + QString::number(status));
        break;
    case UIOHOOK_ERROR_CREATE_OBSERVER:
        emit error(QStringLiteral("Failed to create apple run loop observer, status=") + QString::number(status));
        break;
    case UIOHOOK_FAILURE: // Default error
    default:
        emit error(QStringLiteral("An unknown hook error occurred, status=") + QString::number(status));
        break;
    }
}

// static
void UiohookThread::dispatch(uiohook_event *const event)
{
    TRACE_ARG(event->type);

    auto ac = ActivityCollector::instance();
    Q_ASSERT(ac);
    auto self = ac->uiohook_thread;
    Q_ASSERT(self);

    char buffer[256] = { 0 };
    size_t length = snprintf(buffer, sizeof(buffer),
                             "id=%i,when=%" PRIu64 ",mask=0x%X",
                             event->type, event->time, event->mask);

    self->mutex.lockForWrite();

    switch (event->type) {
    case EVENT_KEY_PRESSED:
#ifdef notdef
        // If the escape key is pressed, naturally terminate the program.
        if (event->data.keyboard.keycode == VC_ESCAPE) {
            int status = hook_stop();
            switch (status) {
            case UIOHOOK_SUCCESS:
                // Everything is ok.
                break;

                // System level errors.
            case UIOHOOK_ERROR_OUT_OF_MEMORY:
                logger(LOG_LEVEL_ERROR, "Failed to allocate memory. (%#X)", status);
                break;

            case UIOHOOK_ERROR_X_RECORD_GET_CONTEXT:
                // NOTE This is the only platform specific error that occurs on hook_stop().
                logger(LOG_LEVEL_ERROR, "Failed to get XRecord context. (%#X)", status);
                break;

                // Default error.
            case UIOHOOK_FAILURE:
            default:
                logger(LOG_LEVEL_ERROR, "An unknown hook error occurred. (%#X)", status);
                break;
            }
        }
#endif
    case EVENT_KEY_RELEASED:
        self->strokes++;
        snprintf(buffer + length, sizeof(buffer) - length,
                 ",keycode=%u,rawcode=0x%X",
                 event->data.keyboard.keycode, event->data.keyboard.rawcode);
        break;

    case EVENT_KEY_TYPED:
        self->strokes++;
        snprintf(buffer + length, sizeof(buffer) - length,
                 ",keychar=%lc,rawcode=%u",
                 (wint_t) event->data.keyboard.keychar,
                 event->data.keyboard.rawcode);
        break;

    case EVENT_MOUSE_MOVED:
    case EVENT_MOUSE_DRAGGED:
        self->odometer++;
        self->mutex.unlock();
        return;
    case EVENT_MOUSE_PRESSED:
    case EVENT_MOUSE_RELEASED:
    case EVENT_MOUSE_CLICKED:
        self->clicks++;
        snprintf(buffer + length, sizeof(buffer) - length,
                 ",x=%i,y=%i,button=%i,clicks=%i",
                 event->data.mouse.x, event->data.mouse.y,
                 event->data.mouse.button, event->data.mouse.clicks);
        break;

    case EVENT_MOUSE_WHEEL:
        snprintf(buffer + length, sizeof(buffer) - length,
                 ",type=%i,amount=%i,rotation=%i",
                 event->data.wheel.type, event->data.wheel.amount,
                 event->data.wheel.rotation);
        break;

    default:
        break;
    }

    fprintf(stdout, "%s\n",     buffer);
}

void UiohookThread::peek(int &s, int &c, int &o) const
{
    mutex.lockForRead();
    s = strokes;
    c = clicks;
    o = odometer;
    mutex.unlock();
}

void UiohookThread::withdraw(int &s, int &c, int &o)
{
    mutex.lockForWrite();
    s = strokes;
    c = clicks;
    o = odometer;
    strokes = clicks = odometer = 0;
    mutex.unlock();
}

void UiohookThread::clear()
{
    mutex.lockForWrite();
    strokes = clicks = odometer = 0;
    mutex.unlock();
}
