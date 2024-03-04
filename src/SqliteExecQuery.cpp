#include <QRandomGenerator>
#include <QThreadPool>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QCborMap>
#include <QDateTime>

#include "SqliteExecQuery.h"
#include "SqliteProducer.h"
#include "SystemHelper.h"

//#define TRACE_SQLITEEXECQUERY
#ifdef TRACE_SQLITEEXECQUERY
#include <QTime>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

SqliteExecQuery::SqliteExecQuery(QObject *parent)
    : QObject(parent)
    , db_filepath(SystemHelper::appDataPath(SqliteProducer::dataBaseFile))
    , time_step(600) // 10 minutes by default
{
    TRACE();
}

SqliteExecQuery::~SqliteExecQuery()
{
    TRACE();
}

int SqliteExecQuery::timeStep() const
{
    return time_step;
}

void SqliteExecQuery::setTimeStep(int seconds)
{
    TRACE_ARG(seconds);

    if (seconds != time_step) {
        time_step = seconds;
        emit timeStepChanged();
    }
}

QString SqliteExecQuery::lastError() const
{
    return last_error;
}

void SqliteExecQuery::setLastError(const QString &text)
{
    TRACE_ARG(text);

    if (text != last_error) {
        last_error = text;
        emit lastErrorChanged(last_error);
    }
}

// static
QString SqliteExecQuery::columnIdName(int column)
{
    return SqliteProducer::columnName(column);
}

// static
int SqliteExecQuery::columnIdIndex(const QString &name)
{
    return SqliteProducer::columnIndex(name);
}

int SqliteExecQuery::request(const QString &query)
{
    TRACE_ARG(query);

    auto task = new SqliteExecTask(db_filepath, query.simplified());
    connect(task, &SqliteConsumer::queryError, this, [this, task](const QString &text) {
        task_hash.remove(task);
        setLastError(text);
    }, Qt::QueuedConnection);
    connect(task, &SqliteConsumer::queryResult, this, [this, task](const CborMapArray &rows) {
        int reqid = task_hash.take(task);
        emit response(reqid, makeJsonArray(rows));
    }, Qt::QueuedConnection);

    int reqid = QRandomGenerator::global()->generate();
    if (!reqid) reqid++; // skip over 0
    task_hash.insert(task, reqid);

    auto pool = QThreadPool::globalInstance();
    pool->setExpiryTimeout(5000);
    pool->start(task);

    setLastError(QString());
    return reqid;
}

// static
qint64 SqliteExecQuery::fromUtcSeconds(qint64 seconds)
{
    return QDateTime::fromSecsSinceEpoch(seconds, Qt::UTC).toLocalTime().toSecsSinceEpoch();
}

QJsonArray SqliteExecQuery::makeJsonArray(const CborMapArray &rows) const
{
    TRACE_ARG(rows.size());

    QJsonArray array;
    for (const auto &row : rows) {
        QJsonObject obj;

        qint64 seconds = row.value(QLatin1String("column0")).toInteger(-1);
        if (seconds == -1) {
            seconds = row.value(QLatin1String("LocalTime")).toInteger(-1);
            if (seconds == -1) continue;
        }
        seconds = QDateTime::fromSecsSinceEpoch(seconds).toUTC().toSecsSinceEpoch();
        if (time_step > 0) obj.insert(QLatin1String("fromAt"), QJsonValue(seconds - time_step));
        obj.insert(QLatin1String("toAt"), QJsonValue(seconds));

        QString uuid = row.value(QLatin1String("ProjectId")).toString();
        if (uuid.isEmpty()) continue;
        obj.insert(QLatin1String("activityId"), uuid);

        QString note = row.value(QLatin1String("TextNote")).toString();
        if (note.isNull()) note = "";
        obj.insert(QLatin1String("note"), QJsonValue(note));

        int minutes = row.value(QLatin1String("minutesActive")).toInteger(0);
        obj.insert(QLatin1String("minutesActive"), QJsonValue(minutes));

        int keys = row.value(QLatin1String("KeyPresses")).toInteger(-1);
        if (keys == -1) continue;
        obj.insert(QLatin1String("keyboardKeys"), QJsonValue(keys));

        int clicks = row.value(QLatin1String("MouseClicks")).toInteger(-1);
        if (clicks == -1) continue;
        obj.insert(QLatin1String("mouseKeys"), QJsonValue(clicks));

        int distance = row.value(QLatin1String("MouseDistance")).toInteger(-1);
        if (distance == -1) continue;
        obj.insert(QLatin1String("mouseDistance"), QJsonValue(distance));

        if (keys || clicks || distance)
            array.append(obj);
    }
    return array;
}

SqliteExecTask::SqliteExecTask(const QString &filepath, const QString &query, QObject *parent)
    : SqliteConsumer(filepath, parent)
    , curr_query(query)
{
    TRACE();
}

SqliteExecTask::~SqliteExecTask()
{
    TRACE();
}

void SqliteExecTask::run()
{
    TRACE();
    SqliteConsumer::start();
    SqliteConsumer::execQuery(curr_query);
}
