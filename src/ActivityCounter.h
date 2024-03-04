#ifndef ACTIVITYCOUNTER_H
#define ACTIVITYCOUNTER_H

#include <QObject>
#include <QUuid>
#include <QHash>
#include <QJsonArray>

#include "PermanentCache.h"

class QTimer;
class SqliteProducer;
class ActivityRecord;
class UiohookCounterThread;

typedef PermanentCache<QUuid,QString> TitleIdCache;

class ActivityCounter : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ActivityCounter)

    Q_PROPERTY(QString tableName READ tableName     WRITE setTableName NOTIFY tableNameChanged FINAL)
    Q_PROPERTY(QString projectId READ projectId     WRITE setProjectId NOTIFY projectIdChanged FINAL)
    Q_PROPERTY(QString  textNote READ textNote      WRITE setTextNote  NOTIFY textNoteChanged FINAL)
    Q_PROPERTY(int      idleTime READ idleTime      WRITE setIdleTime  NOTIFY idleTimeChanged FINAL)
    Q_PROPERTY(bool      running READ running       WRITE setRunning   NOTIFY runningChanged FINAL)
    Q_PROPERTY(bool    available READ available     NOTIFY availableChanged FINAL)
    Q_PROPERTY(int    keyPresses READ keyPresses    NOTIFY keyPressesChanged FINAL)
    Q_PROPERTY(int   mouseClicks READ mouseClicks   NOTIFY mouseClicksChanged FINAL)
    Q_PROPERTY(int mouseDistance READ mouseDistance NOTIFY mouseDistanceChanged FINAL)
    Q_PROPERTY(int      timeStep READ timeStep      NOTIFY timeStepChanged FINAL)
    Q_PROPERTY(QString timeCount READ timeCount     NOTIFY timeCountChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError     NOTIFY lastErrorChanged FINAL)

public:
    static constexpr int const startUpDelay    = 750; // milliseconds
    static constexpr char const *titleIdCache  = "titleid.cache";
    static constexpr char const *settingsGroup = "ActivityCounter";
    static constexpr char const *idleTimeMinut = "idleTimeMinut";
    static constexpr char const *lastProjectId = "lastProjectId";
    static constexpr char const *lastWorkTime  = "lastWorkTime";
    static constexpr char const *lastTimeCount = "lastTimeCount";

    explicit ActivityCounter(QObject *parent = nullptr);
    ~ActivityCounter();

    enum IdleTimeRange { // in minutes
        IdleTimeMin = 10,
        IdleTimeDef = 20,
        IdleTimeMax = 60
    };
    Q_ENUM(IdleTimeRange)

    static ActivityCounter *instance();

    QString tableName() const;
    void setTableName(const QString &name);

    QString projectId() const;
    void setProjectId(const QString &uuid);

    QString textNote() const;
    void setTextNote(const QString &note);

    int idleTime() const; // in minutes
    void setIdleTime(int minutes);

    bool running() const;
    void setRunning(bool enable);

    bool available() const;
    int keyPresses() const;
    int mouseClicks() const;
    int mouseDistance() const;
    int timeStep() const; // in seconds
    QString timeCount() const;
    QString lastError() const;

    Q_INVOKABLE void setTitleCache(const QString &id, const QString &title);
    Q_INVOKABLE bool isTitleCache(const QString &id) const;
    Q_INVOKABLE QString titleCache(const QString &id) const;
    QString uuidCache(const QUuid &uuid) const;

public slots:
    void start();
    void setSqlDbStatus(const QJsonArray &array);

signals:
    void tableNameChanged();
    void projectIdChanged();
    void textNoteChanged();
    void idleTimeChanged();
    void runningChanged();
    void availableChanged(bool ok);
    void keyPressesChanged();
    void mouseClicksChanged();
    void mouseDistanceChanged();
    void timeStepChanged();
    void timeCountChanged();
    void lastErrorChanged(const QString &text);
    void sqlDbChanged();
    void notification(const QString &text);

private:
    friend class UiohookCounterThread;
    void startUp();
    void cleanUp();
    void onSqlConfigChanged(const QVariantMap &map);
    void onDailyTimer();
    void onSecondTimer();
    void setLastError(const QString &text);
    void saveWorkTime();

    UiohookCounterThread *uiohook_thread;
    SqliteProducer *sql_db;
    QString last_error;

    TitleIdCache title_cache;
    QString table_name;
    QString project_id;
    QString text_note;
    int idle_time; // in seconds

    int key_presses;
    int mouse_clicks;
    int mouse_distance;
    int last_keys;
    int last_clicks;
    int last_distance;

    QTimer *daily_timer;
    QTimer *second_timer;

    bool history_on;
    int second_count, second_step;
    int time_count; // in seconds
    int idle_count;
};

#endif // ACTIVITYCOUNTER_H
