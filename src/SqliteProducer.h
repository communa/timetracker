#ifndef SQLITEPRODUCER_H
#define SQLITEPRODUCER_H

#include <QObject>
#include <QMap>
#include <QVariantMap>

#include "BaseThread.h"

class ActivityRecord;
class QTimer;

typedef QMap<qint64,QString> ServerStatusMap;

class SqliteProducer : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SqliteProducer)

public:
    static constexpr char const *dataBaseFile   = "ActivityTrack.db";
    static constexpr char const *dataBaseConfig = "ConfigHistory";
    static constexpr char const *dataBaseReduce = "23:59:55";
    static constexpr char const *sqlConfigQuery = "SELECT * FROM Config";

    static constexpr char const *dataBaseColumn[] = {
        "LocalTime", "ProjectId", "TextNote", "KeyPresses", "MouseClicks", "MouseDistance", "ServerStatus"
    };
    enum DataBaseColumn {
        LocalTime, ProjectId, TextNote, KeyPresses, MouseClicks, MouseDistance, ServerStatus,
        TotalColumns
    };
    Q_ENUM(DataBaseColumn)

    enum DataBaseConfig {
        HistoryOn = 1,   // used as boolean 0=false, 1=true
        TimeStep  = 1,   // default time step in minutes
        KeepDays  = 365, // default max size in days
    };
    Q_ENUM(DataBaseConfig)

    explicit SqliteProducer(const QString &filepath, QObject *parent = nullptr);
    ~SqliteProducer();

    static QString columnName(int column);
    static int columnIndex(const QString &name);
    static QMap<QString,int> columnMap();

public slots:
    void start();
    void configure(const QVariantMap &map);
    void insertRow(const ActivityRecord &record);
    void setServerStatus(const QString &tableName, const ServerStatusMap &status);

signals:
    void configChanged(const QVariantMap &map);
    void errorOccurred(const QString &text);
    void dataChanged();

private:
    QTimer *reduceTimer();
    void reconfig();
    void reduce();

    QString conn_name;
    QString db_filepath;
    QString reduce_at;
    int keep_days;
    QTimer *reduce_timer;
    QVariantMap db_config;
};

class SqliteProducerThread : public BaseThread<SqliteProducer>
{
    Q_OBJECT
    Q_DISABLE_COPY(SqliteProducerThread)

public:
    explicit SqliteProducerThread(const QString &filepath, QObject *parent = nullptr)
        : BaseThread<SqliteProducer>(new SqliteProducer(filepath), parent) {
        connect(this, &QThread::started, worker(), &SqliteProducer::start, Qt::QueuedConnection);
    }
};

#endif // SQLITEPRODUCER_H
