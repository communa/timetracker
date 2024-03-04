#ifndef SQLITECONSUMER_H
#define SQLITECONSUMER_H

#include <QObject>
#include <QHash>
#include <QCborMap>

#include "BaseThread.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
typedef QVector<QCborMap>   CborMapArray;
typedef QVector<QCborValue> CborValueArray;
#else
typedef QList<QCborMap>     CborMapArray;
typedef QList<QCborValue>   CborValueArray;
#endif

class QTimer;

class SqliteConsumer : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SqliteConsumer)

public:
    static constexpr int const maxRowsPerQuery = 24 * 60; // minutes per day

    explicit SqliteConsumer(const QString &filepath, QObject *parent = nullptr);
    virtual ~SqliteConsumer();

public slots:
    void start();
    void execQuery(const QString &request);

signals:
    void queryError(const QString &text);
    void queryResult(const CborMapArray &response);

private:
    void reopen();

    QString conn_name;
    QString db_filepath;
    QTimer *open_timer;
    int open_retry;
    qint64 db_mtime;
    QHash<QString,CborMapArray> query_cache;
};

class SqliteConsumerThread : public BaseThread<SqliteConsumer>
{
    Q_OBJECT
    Q_DISABLE_COPY(SqliteConsumerThread)

public:
    explicit SqliteConsumerThread(const QString &filepath, QObject *parent = nullptr)
        : BaseThread<SqliteConsumer>(new SqliteConsumer(filepath), parent) {
        connect(this, &QThread::started, worker(), &SqliteConsumer::start, Qt::QueuedConnection);
    }
};

#endif // SQLITECONSUMER_H
