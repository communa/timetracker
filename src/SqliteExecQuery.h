#ifndef SQLITEEXECQUERY_H
#define SQLITEEXECQUERY_H

#include <QObject>
#include <QHash>
#include <QRunnable>

#include "SqliteConsumer.h"

class QCborMap;
class QJsonArray;
class SqliteExecTask;

class SqliteExecQuery : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SqliteExecQuery)

    Q_PROPERTY(int      timeStep READ timeStep  WRITE setTimeStep NOTIFY timeStepChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)

public:
    explicit SqliteExecQuery(QObject *parent = nullptr);
    ~SqliteExecQuery();

    int timeStep() const; // in seconds
    void setTimeStep(int seconds);

    QString lastError() const;

    Q_INVOKABLE static QString columnIdName(int column);
    Q_INVOKABLE static int columnIdIndex(const QString &name);

    Q_INVOKABLE int request(const QString &query); // return reqid
    Q_INVOKABLE static qint64 fromUtcSeconds(qint64 seconds);

signals:
    void timeStepChanged();
    void lastErrorChanged(const QString &text);
    void response(int reqid, const QJsonArray &array);

private:
    void setLastError(const QString &text);
    QJsonArray makeJsonArray(const CborMapArray &rows) const;

    const QString db_filepath;
    QHash<SqliteExecTask*,int> task_hash;

    int time_step;
    QString last_error;
};

class SqliteExecTask : public SqliteConsumer, public QRunnable
{
public:
    explicit SqliteExecTask(const QString &filepath, const QString &query, QObject *parent = nullptr);
    ~SqliteExecTask() override;
private:
    void run() override;
    const QString curr_query;
};

#endif // SQLITEEXECQUERY_H
