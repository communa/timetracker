#ifndef ACTIVITYRECORD_H
#define ACTIVITYRECORD_H

#include <QSharedDataPointer>
#include <QMetaType>
#include <QDateTime>

class QUuid;
class ActivityRecordData;

class ActivityRecord
{
    Q_GADGET
    Q_PROPERTY(QString   tableName READ tableName     CONSTANT FINAL)
    Q_PROPERTY(QDateTime localTime READ localTime     CONSTANT FINAL)
    Q_PROPERTY(QDateTime   utcTime READ utcTime       CONSTANT FINAL)
    Q_PROPERTY(QString   projectId READ projectId     CONSTANT FINAL)
    Q_PROPERTY(QString    textNote READ textNote      CONSTANT FINAL)
    Q_PROPERTY(int      keyPresses READ keyPresses    CONSTANT FINAL)
    Q_PROPERTY(int     mouseClicks READ mouseClicks   CONSTANT FINAL)
    Q_PROPERTY(int   mouseDistance READ mouseDistance CONSTANT FINAL)

public:
    ActivityRecord();
    // the time must be specified as Local time, e.g. QDateTime::currentDateTime()
    ActivityRecord(const QString &tableName,
                   const QDateTime &localTime, const QUuid &uuid, const QString &textNote,
                   int keyPresses, int mouseClicks, int mouseDistance);
    ActivityRecord(const ActivityRecord &);
    ActivityRecord &operator=(const ActivityRecord &);
    ~ActivityRecord();

    bool operator==(const ActivityRecord &other) const;
    bool operator!=(const ActivityRecord &other) const;

    bool isValid() const;
    void clearCounters();

    QString tableName() const;
    QDateTime localTime() const;
    QDateTime utcTime() const;
    QUuid uuid() const;
    QString projectId() const; // text form of the uuid() above
    QString textNote() const;
    int keyPresses() const;
    int mouseClicks() const;
    int mouseDistance() const;

    friend inline QDebug& operator<<(QDebug &dbg, const ActivityRecord &from) {
        from.dump(dbg); return dbg; }

private:
    void dump(QDebug &dbg) const;

    QSharedDataPointer<ActivityRecordData> d;
};

Q_DECLARE_TYPEINFO(ActivityRecord, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(ActivityRecord)

#endif // ACTIVITYRECORD_H
