#include <QCoreApplication>
#include <QSettings>
#include <QTimer>
#include <QThread>
#include <QAtomicInt>
#include <QUuid>
#include <QPoint>
#include <QDateTime>
#include <QMetaObject>
#include <QtDebug>

#include "ActivityCounter.h"
#include "ActivityRecord.h"
#include "SqliteProducer.h"
#include "SystemHelper.h"
#include "uiohook.h"

//#define TRACE_ACTIVITYCOUNTER
#ifdef TRACE_ACTIVITYCOUNTER
#include <QTime>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

//#define TRACE_UIOHOOKCOUNTERTHREAD
#ifdef TRACE_UIOHOOKCOUNTERTHREAD
#include <QTime>
#define TRACE_UIOHOOK(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE_UIOHOOK(x)
#endif

Q_GLOBAL_STATIC(ActivityCounter, globalActivityCounter)

static const struct errorStatusEntry {
    const int status;
    const char *text;
} errorStatusTable[] = {
    // System level errors
    { UIOHOOK_ERROR_OUT_OF_MEMORY,           "Failed to allocate memory" },

    // Unix specific errors
    { UIOHOOK_ERROR_X_OPEN_DISPLAY,          "Failed to open X11 display" },
    { UIOHOOK_ERROR_X_RECORD_NOT_FOUND,      "Unable to locate XRecord extension" },
    { UIOHOOK_ERROR_X_RECORD_ALLOC_RANGE,    "Unable to allocate XRecord range" },
    { UIOHOOK_ERROR_X_RECORD_CREATE_CONTEXT, "Unable to allocate XRecord context" },
    { UIOHOOK_ERROR_X_RECORD_ENABLE_CONTEXT, "Failed to enable XRecord context" },
    { UIOHOOK_ERROR_X_RECORD_GET_CONTEXT,    "Failed to get XRecord context" },

    // Windows specific errors
    { UIOHOOK_ERROR_SET_WINDOWS_HOOK_EX,     "Failed to register low level windows hook" },
    { UIOHOOK_ERROR_GET_MODULE_HANDLE,       "Failed to get windows hook module handle" },

    // Darwin specific errors
    { UIOHOOK_ERROR_AXAPI_DISABLED,          "Failed to enable access for assistive devices" },
    { UIOHOOK_ERROR_CREATE_EVENT_PORT,       "Failed to create apple event port" },
    { UIOHOOK_ERROR_CREATE_RUN_LOOP_SOURCE,  "Failed to create apple run loop source" },
    { UIOHOOK_ERROR_GET_RUNLOOP,             "Failed to acquire apple run loop" },
    { UIOHOOK_ERROR_CREATE_OBSERVER,         "Failed to create apple run loop observer" },
    { 0, 0 }
};

static QString errorStatusText(int status)
{
    for (const struct errorStatusEntry *e = errorStatusTable; e && e->status && e->text; ++e) {
        if (e->status == status)
            return QLatin1String(e->text);
    }
    return QLatin1String("Unknown error occurred");
}

class UiohookCounterThread : public QThread
{
public:
    UiohookCounterThread() : QThread(), uiohook_status(UIOHOOK_FAILURE) {
        TRACE_UIOHOOK("Create");

        hook_set_dispatch_proc(&uiohookEvent);
    }
    ~UiohookCounterThread() override;

    QAtomicInt uiohook_status;
    QAtomicInt count_actions;
    QAtomicInt key_presses;
    QAtomicInt mouse_clicks;
    QAtomicInt mouse_distance;

protected:
    void run() override {
        setTerminationEnabled(true);
        uiohook_status = hook_run();
        TRACE_UIOHOOK("hook_run() status" << uiohook_status);
    }
private:
    static void uiohookEvent(struct _uiohook_event *const event);
    void setLastPos(int x, int y);
    QPoint last_pos;
};

UiohookCounterThread::~UiohookCounterThread()
{
    TRACE_UIOHOOK("Destroy");

    if (isRunning()) {
        requestInterruption();
        for (int i = 0; !wait(250) && i < 4; i++) {
            if (i) qWarning() << Q_FUNC_INFO << "Still running, trying terminate" << i;
            else { TRACE_UIOHOOK("Terminate"); }
            terminate();
        }
    }
}

// static
void UiohookCounterThread::uiohookEvent(uiohook_event *const event)
{
    //TRACE_UIOHOOK(event);

    if (QThread::currentThread()->isInterruptionRequested()) return;

    auto self = ActivityCounter::instance()->uiohook_thread;
    Q_ASSERT(self);

    if (event->type == EVENT_HOOK_ENABLED) {
        TRACE_UIOHOOK("Hook enabled");
        self->uiohook_status = UIOHOOK_SUCCESS;
        return;
    }
    if (event->type == EVENT_HOOK_DISABLED) {
        TRACE_UIOHOOK("Hook disabled");
        self->uiohook_status = UIOHOOK_FAILURE;
        return;
    }
    if (!self->count_actions) return;

    switch (event->type) {
    case EVENT_KEY_PRESSED:
        TRACE_UIOHOOK(event->type << "keyPressed" << event->data.keyboard.keycode
                                  << "rawCode" << event->data.keyboard.rawcode);
        break;
    case EVENT_KEY_RELEASED:
        if (event->data.keyboard.keycode || event->data.keyboard.rawcode)
            self->key_presses++;
        TRACE_UIOHOOK(event->type << "keyReleased" << event->data.keyboard.keycode
                                  << "rawCode" << event->data.keyboard.rawcode);
        break;
    case EVENT_KEY_TYPED:
        TRACE_UIOHOOK(event->type << "keyChar" << event->data.keyboard.keychar
                                  << "rawCode" << event->data.keyboard.rawcode);
        break;
    case EVENT_MOUSE_PRESSED:
        TRACE_UIOHOOK(event->type << "posPressed" << event->data.mouse.x << event->data.mouse.y
                                  << "button" << event->data.mouse.button
                                  << "clicks" << event->data.mouse.clicks);
        break;
    case EVENT_MOUSE_RELEASED:
        TRACE_UIOHOOK(event->type << "posReleased" << event->data.mouse.x << event->data.mouse.y
                                  << "button" << event->data.mouse.button
                                  << "clicks" << event->data.mouse.clicks);
        break;
    case EVENT_MOUSE_CLICKED:
        if (event->data.mouse.button || event->data.mouse.clicks)
            self->mouse_clicks++;
        TRACE_UIOHOOK(event->type << "posClicked" << event->data.mouse.x << event->data.mouse.y
                                  << "button" << event->data.mouse.button
                                  << "clicks" << event->data.mouse.clicks);
        break;
    case EVENT_MOUSE_MOVED:
        self->setLastPos(event->data.mouse.x, event->data.mouse.y);
        TRACE_UIOHOOK(event->type << "posMoved" << event->data.mouse.x << event->data.mouse.y
                                  << "button" << event->data.mouse.button
                                  << "clicks" << event->data.mouse.clicks);
        break;
    case EVENT_MOUSE_DRAGGED:
        self->setLastPos(event->data.mouse.x, event->data.mouse.y);
        TRACE_UIOHOOK(event->type << "posDragged" << event->data.mouse.x << event->data.mouse.y
                                  << "button" << event->data.mouse.button
                                  << "clicks" << event->data.mouse.clicks);
        break;
    case EVENT_MOUSE_WHEEL:
        TRACE_UIOHOOK(event->type << "wheelType" << event->data.wheel.type
                                  << "amount" << event->data.wheel.amount
                                  << "rotation" << event->data.wheel.rotation);
        break;
    default:
        qWarning() << Q_FUNC_INFO << "Unexpected event type" << event->type;
    }
}

void UiohookCounterThread::setLastPos(int x, int y)
{
    QPoint pos(x, y);
    if (!last_pos.isNull()) {
        QPoint diff = pos - last_pos;
        mouse_distance += diff.manhattanLength();
    }
    last_pos = pos;
}

static QString loadTextNote(const QString &id)
{
    if (!id.isEmpty()) {
        auto lines = SystemHelper::loadText(SystemHelper::appCachePath(id));
        if (!lines.isEmpty()) {
            if (lines.size() == 1) return lines.at(0);
            if (lines.size() > 3) return lines.mid(0, 3).join('\n');
            return lines.join('\n');
        }
    }
    return QString();
}

static int msecsToMidnight()
{
    QDateTime now = QDateTime::currentDateTime();
    int msec = now.msecsTo(now.date().addDays(1).startOfDay()) + 500; // make sure next day
    TRACE_ARG("Next time" << QTime(0,0).addMSecs(msec).toString());
    return msec;
}

ActivityCounter::ActivityCounter(QObject *parent)
    : QObject(parent)
    , uiohook_thread(new UiohookCounterThread)
    , sql_db(nullptr)
    , title_cache(SystemHelper::appCachePath(titleIdCache))
    , idle_time(IdleTimeDef * 60) // make seconds
    , key_presses(0)
    , mouse_clicks(0)
    , mouse_distance(0)
    , last_keys(0)
    , last_clicks(0)
    , last_distance(0)
    , daily_timer(nullptr)
    , second_timer(nullptr)
    , history_on(SqliteProducer::HistoryOn > 0)
    , second_count(0)
    , second_step(SqliteProducer::TimeStep * 60) // make seconds
    , time_count(0)
    , idle_count(0)
{
    TRACE();

    qRegisterMetaType<ActivityRecord>("ActivityRecord");
    qRegisterMetaType<ServerStatusMap>("ServerStatusMap");

    QSettings settings;
    settings.beginGroup(QLatin1String(settingsGroup));

    int minutes = settings.value(QLatin1String(idleTimeMinut), (int)IdleTimeDef).toInt();
    idle_time = qBound((int)IdleTimeMin, minutes, (int)IdleTimeMax) * 60; // make seconds

    QUuid uuid(settings.value(QLatin1String(lastProjectId)).toString());
    if (!uuid.isNull()) {
        project_id = uuid.toString(QUuid::WithoutBraces);
        text_note = loadTextNote(project_id);
    }
    int seconds = settings.value(QLatin1String(lastTimeCount), 0).toInt();
    if (seconds > 0) {
        QDateTime dt = QDateTime::fromString(settings.value(QLatin1String(lastWorkTime)).toString(), Qt::ISODate);
        if (dt.isValid()) {
            QDateTime now = QDateTime::currentDateTime();
            if (dt.date() == now.date()) {
                int max = dt.time().secsTo(now.time());
                if (max > 0) time_count = seconds < max ? seconds : max;
            }
        }
    }
    settings.endGroup();

#ifdef TRACE_ACTIVITYCOUNTER
    const auto &tc = title_cache.cacheHash();
    for (auto it = tc.constBegin(); it != tc.constEnd(); ++it) {
        qDebug() << "\t" << it.key() << it.value();
    }
#endif

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &ActivityCounter::cleanUp);

    start();
}

ActivityCounter::~ActivityCounter()
{
    TRACE();

    delete uiohook_thread;
}

void ActivityCounter::start()
{
    TRACE_ARG(uiohook_thread->isRunning());

    if (!uiohook_thread->isRunning()) {
        QTimer::singleShot(startUpDelay, this, &ActivityCounter::startUp);
        uiohook_thread->start(QThread::TimeCriticalPriority);
    }
}

void ActivityCounter::startUp()
{
    TRACE();

    int status = uiohook_thread->uiohook_status;
    if (status == UIOHOOK_SUCCESS) {
        if (!daily_timer) {
            daily_timer = new QTimer(this);
            daily_timer->setSingleShot(true);
            connect(daily_timer, &QTimer::timeout, this, &ActivityCounter::onDailyTimer);
        }
        if (!second_timer) {
            second_timer = new QTimer(this);
            second_timer->setInterval(1000);
            second_timer->setSingleShot(false);
            connect(second_timer, &QTimer::timeout, this, &ActivityCounter::onSecondTimer);
        }
        if (!sql_db) {
            auto thread = new SqliteProducerThread(SystemHelper::appDataPath(SqliteProducer::dataBaseFile), this);
            sql_db = thread->worker();
            Q_ASSERT(sql_db);
            connect(sql_db, &SqliteProducer::errorOccurred, this, &ActivityCounter::setLastError, Qt::QueuedConnection);
            connect(sql_db, &SqliteProducer::dataChanged, this, &ActivityCounter::sqlDbChanged, Qt::QueuedConnection);
            connect(sql_db, &SqliteProducer::configChanged, this,&ActivityCounter::onSqlConfigChanged, Qt::QueuedConnection);
            thread->start();
        }
        if (!daily_timer->isActive())
            daily_timer->start(msecsToMidnight());

    } else setLastError(errorStatusText(status));

    emit availableChanged(status == UIOHOOK_SUCCESS);
}

void ActivityCounter::cleanUp()
{
    TRACE();

    if (daily_timer) daily_timer->stop();

    if (second_timer && second_timer->isActive()) {
        uiohook_thread->count_actions = 0;
        second_timer->stop();
        if (time_count > 0) saveWorkTime();
    }
    if (uiohook_thread->isRunning()) {
        uiohook_thread->requestInterruption();
        hook_stop();
    }
}

void ActivityCounter::onSqlConfigChanged(const QVariantMap &map)
{
    TRACE_ARG(map.size());

    if (map.isEmpty()) return;

    if (map.contains(QStringLiteral("HistoryOn"))) {
        history_on = (map.value(QStringLiteral("HistoryOn")).toInt() > 0);
    }
    if (map.contains(QStringLiteral("TimeStep"))) {
        int step = map.value(QStringLiteral("TimeStep")).toInt() * 60; // make seconds
        if (step != second_step) {
            second_step = step;
            emit timeStepChanged();
        }
    }
    TRACE_ARG("HistoryOn" << history_on << "TimeStep" << second_step);
}

void ActivityCounter::saveWorkTime()
{
    TRACE();

    QSettings settings;
    settings.beginGroup(QLatin1String(settingsGroup));
    settings.setValue(QLatin1String(lastWorkTime), QDateTime::currentDateTime().toString(Qt::ISODate));
    settings.setValue(QLatin1String(lastTimeCount), time_count);
    settings.endGroup();
    settings.sync();
}

// static
ActivityCounter *ActivityCounter::instance()
{
    auto singleton = globalActivityCounter;
    Q_ASSERT(singleton);
    return singleton;
}

void ActivityCounter::onDailyTimer()
{
    TRACE();

    bool err_changed = !last_error.isEmpty();
    if (err_changed) last_error.clear();

    bool keys_changed = (key_presses != 0);
    bool clicks_changed = (mouse_clicks != 0);
    bool distance_changed = (mouse_distance != 0);

    uiohook_thread->key_presses = uiohook_thread->mouse_clicks = uiohook_thread->mouse_distance = 0;

    key_presses = last_keys = 0;
    mouse_clicks = last_clicks = 0;
    mouse_distance = last_distance = 0;

    daily_timer->start(msecsToMidnight());

    if (time_count) {
        time_count = 0;
        emit timeCountChanged();
    }
    if (err_changed) emit lastErrorChanged(QString());
    if (keys_changed) emit keyPressesChanged();
    if (clicks_changed) emit mouseClicksChanged();
    if (distance_changed) emit mouseDistanceChanged();
}

void ActivityCounter::onSecondTimer()
{
    if (uiohook_thread->isFinished()) {
        TRACE_ARG("hook thread finished");
        setRunning(false);
        setLastError(tr("Unexpected UIOhook thread termination"));
        emit availableChanged(false);
        return;
    }
    int status = uiohook_thread->uiohook_status;
    if (status != UIOHOOK_SUCCESS) {
        TRACE_ARG("uiohook_status:" << status);
        setRunning(false);
        setLastError(errorStatusText(status));
        emit availableChanged(false);
        return;
    }

    int keys = uiohook_thread->key_presses;
    int clicks = uiohook_thread->mouse_clicks;
    int distance = uiohook_thread->mouse_distance;

    bool keys_changed = (keys != key_presses);
    bool clicks_changed = (clicks != mouse_clicks);
    bool distance_changed = (distance != mouse_distance);

    key_presses = keys;
    mouse_clicks = clicks;
    mouse_distance = distance;

    second_count++;
    if (history_on && second_step && (second_count % second_step) == 0 &&
            !table_name.isEmpty() && !project_id.isEmpty()) {
        keys -= last_keys;
        clicks -= last_clicks;
        distance -= last_distance;
        if (keys || clicks || distance) {
            QDateTime now = QDateTime::currentDateTime();
            TRACE_ARG("second_count" << second_count << "Now" << now.toString());
            ActivityRecord record(table_name, now, QUuid(project_id), text_note, keys, clicks, distance);
            if (record.isValid() && sql_db) {
                QMetaObject::invokeMethod(sql_db, "insertRow", Qt::QueuedConnection,
                                          Q_ARG(ActivityRecord, record));
            }
            last_keys = key_presses;
            last_clicks = mouse_clicks;
            last_distance = mouse_distance;
        }
    }

    time_count++;
    emit timeCountChanged();

    if (keys_changed) emit keyPressesChanged();
    if (clicks_changed) emit mouseClicksChanged();
    if (distance_changed) emit mouseDistanceChanged();

    if (keys_changed || clicks_changed || distance_changed) {
        idle_count = 0;
    } else if (++idle_count >= idle_time) {
        TRACE_ARG("idle_count" << idle_count);
        setRunning(false);
        emit notification(tr("Idle time for %1 minutes").arg(idleTime()));
    }
}

QString ActivityCounter::tableName() const
{
    return table_name;
}

void ActivityCounter::setTableName(const QString &name)
{
    TRACE_ARG(name);

    if (name != table_name) {
        table_name = name;
        if (table_name.isEmpty()) setRunning(false);
        emit tableNameChanged();
    }
}

QString ActivityCounter::projectId() const
{
    return project_id;
}

void ActivityCounter::setProjectId(const QString &id)
{
    TRACE_ARG(id);

    if (id == project_id) return;

    if (!id.isEmpty()) {
        QUuid uuid(id);
        if (uuid.isNull()) {
            setLastError(tr("Invalid UUID format"));
            return;
        }
    }
    project_id = id;

    QString note;
    if (!project_id.isEmpty()) {
        QSettings settings;
        settings.beginGroup(QLatin1String(settingsGroup));
        settings.setValue(QLatin1String(lastProjectId), project_id);
        settings.endGroup();

        note = loadTextNote(project_id);
    }
    if (note != text_note) {
        text_note = note;
        emit textNoteChanged();
    }

    setRunning(false);
    emit projectIdChanged();
}

QString ActivityCounter::textNote() const
{
    return text_note;
}

void ActivityCounter::setTextNote(const QString &text)
{
    TRACE_ARG(text);

    if (text != text_note) {
        text_note = text;
        if (!project_id.isEmpty()) {
            QString path = SystemHelper::saveText(SystemHelper::appCachePath(project_id), text_note.split('\n'));
            if (text_note.isEmpty() && !path.isEmpty())
                SystemHelper::removeFile(path);
        }
        emit textNoteChanged();
    }
}

int ActivityCounter::idleTime() const
{
    return idle_time / 60; // make minutes
}

void ActivityCounter::setIdleTime(int minutes)
{
    int seconds = qBound((int)IdleTimeMin, minutes, (int)IdleTimeMax) * 60; // make seconds
    if (seconds != idle_time) {
        idle_time = seconds;

        QSettings settings;
        settings.beginGroup(QLatin1String(settingsGroup));
        settings.setValue(QLatin1String(idleTimeMinut), idleTime());
        settings.endGroup();

        emit idleTimeChanged();
    }
}

bool ActivityCounter::running() const
{
    return (second_timer && second_timer->isActive());
}

void ActivityCounter::setRunning(bool enable)
{
    TRACE_ARG(enable);

    if (!second_timer || enable == second_timer->isActive()) return;

    idle_count = 0;
    if (enable) {
        if (!available()) {
            setLastError(tr("UIOhook is not available"));
            return;
        }
        if (project_id.isEmpty()) {
            setLastError(tr("No project selected"));
            return;
        }
        QTime now = QTime::currentTime();
        second_count = now.second();
        if (second_count < 30) second_count += 60;
        TRACE_ARG("second_count" << second_count << "Now" << now.toString());
        second_timer->start();
        uiohook_thread->count_actions = 1;
    } else {
        uiohook_thread->count_actions = 0;
        second_count = 0;
        second_timer->stop();
        if (time_count > 0) saveWorkTime();
    }
    emit runningChanged();
}

bool ActivityCounter::available() const
{
    return (second_timer && uiohook_thread->isRunning() && uiohook_thread->uiohook_status == UIOHOOK_SUCCESS);
}

QString ActivityCounter::lastError() const
{
    return last_error;
}

void ActivityCounter::setLastError(const QString &text)
{
    TRACE_ARG(text);

    if (text != last_error) {
        last_error = text;
        emit lastErrorChanged(last_error);
    }
}

int ActivityCounter::keyPresses() const
{
    return key_presses;
}

int ActivityCounter::mouseClicks() const
{
    return mouse_clicks;
}

int ActivityCounter::mouseDistance() const
{
    return mouse_distance;
}

int ActivityCounter::timeStep() const
{
    return second_step;
}

QString ActivityCounter::timeCount() const
{
    QTime zero(0, 0);
    return zero.addSecs(time_count).toString(QStringLiteral("hh:mm:ss"));
}

void ActivityCounter::setTitleCache(const QString &id, const QString &title)
{
    TRACE_ARG(id << title);

    if (id.count('-') != 4 || title.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Argument is invalid";
        return;
    }
    QUuid uuid(id);
    if (uuid.isNull()) {
        qWarning() << Q_FUNC_INFO << "UUID is invalid" << id;
        return;
    }
    title_cache.cacheInsert(uuid, title);
}

bool ActivityCounter::isTitleCache(const QString &id) const
{
    if (id.count('-') == 4) {
        QUuid uuid(id);
        if (!uuid.isNull())
            return title_cache.cacheContains(uuid);
    }
    return false;
}

QString ActivityCounter::titleCache(const QString &id) const
{
    if (id.count('-') == 4) {
        QUuid uuid(id);
        if (!uuid.isNull()) return title_cache.cacheValue(uuid);
    }
    return QString();
}

QString ActivityCounter::uuidCache(const QUuid &uuid) const
{
    return !uuid.isNull() ? title_cache.cacheValue(uuid) : QString();
}

void ActivityCounter::setSqlDbStatus(const QJsonArray &array)
{
    TRACE_ARG(array);

    if (!array.isEmpty() && sql_db) {
        ServerStatusMap status;
        for (int i = 0; i < array.size(); i++) {
            QJsonObject obj = array.at(i).toObject();
            if (obj.isEmpty()) continue;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            qint64 seconds = obj.value(QLatin1String("toAt")).toInt(-1);
#else
            qint64 seconds = obj.value(QLatin1String("toAt")).toInteger(-1);
#endif
            if (seconds == -1) continue;
            QString message = obj.value(QLatin1String("message")).toString();
            if (message.isEmpty()) continue;
            QString name = obj.value(QLatin1String("name")).toString();
            if (!name.isEmpty()) message.prepend(name + ": ");
            status.insert(seconds, message);
        }
        if (status.size() == array.size()) {
            QMetaObject::invokeMethod(sql_db, "setServerStatus", Qt::QueuedConnection,
                                      Q_ARG(ServerStatusMap, status));
        } else {
            qWarning() << Q_FUNC_INFO << "Malformed JsonArray" << array.size() << status.size();
        }
    }
}
