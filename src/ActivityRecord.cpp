#include <QUuid>
#include <QDebug>

#include "ActivityRecord.h"

class ActivityRecordData : public QSharedData
{
public:
    ActivityRecordData()
        : key_presses(0)
        , mouse_clicks(0)
        , mouse_distance(0)
    {}
    ActivityRecordData(const ActivityRecordData &other)
        : QSharedData(other)
        , table_name(other.table_name)
        , local_time(other.local_time)
        , uuid(other.uuid)
        , text_note(other.text_note)
        , key_presses(other.key_presses)
        , mouse_clicks(other.mouse_clicks)
        , mouse_distance(other.mouse_distance)
    {}
    ~ActivityRecordData() {}

    QString table_name;
    QDateTime local_time;
    QUuid uuid;
    QString text_note;
    int key_presses;
    int mouse_clicks;
    int mouse_distance;
};

ActivityRecord::ActivityRecord()
    : d(new ActivityRecordData)
{
}

ActivityRecord::ActivityRecord(const QString &table_name,
                               const QDateTime &local_time, const QUuid &uuid, const QString &text_note,
                               int key_presses, int mouse_clicks, int mouse_distance)
    : d(new ActivityRecordData)
{
    d->table_name = !table_name.isEmpty() ? table_name : QStringLiteral("ActivityTable");
    d->local_time = local_time.isValid() ? local_time : QDateTime::currentDateTime();
    d->uuid = uuid;
    d->text_note = text_note;
    d->key_presses = key_presses > 0 ? key_presses : 0;
    d->mouse_clicks = mouse_clicks > 0 ? mouse_clicks : 0;
    d->mouse_distance = mouse_distance > 0 ? mouse_distance : 0;
}

ActivityRecord::ActivityRecord(const ActivityRecord &other)
    : d(other.d)
{
}

ActivityRecord &ActivityRecord::operator=(const ActivityRecord &other)
{
    if (this != &other) d.operator=(other.d);
    return *this;
}

ActivityRecord::~ActivityRecord()
{
}

bool ActivityRecord::operator==(const ActivityRecord &other) const
{
    return (d->table_name == other.d->table_name &&
            d->local_time == other.d->local_time &&
            d->uuid == other.d->uuid &&
            d->text_note == other.d->text_note &&
            d->key_presses == other.d->key_presses &&
            d->mouse_clicks == other.d->mouse_clicks &&
            d->mouse_distance == other.d->mouse_distance);
}

bool ActivityRecord::operator!=(const ActivityRecord &other) const
{
    return (d->table_name != other.d->table_name ||
            d->local_time != other.d->local_time ||
            d->uuid != other.d->uuid ||
            d->text_note != other.d->text_note ||
            d->key_presses != other.d->key_presses ||
            d->mouse_clicks != other.d->mouse_clicks ||
            d->mouse_distance != other.d->mouse_distance);
}

bool ActivityRecord::isValid() const
{
    return (!d->table_name.isEmpty() && d->local_time.isValid() && !d->uuid.isNull() &&
            (d->key_presses > 0 || d->mouse_clicks > 0 || d->mouse_distance > 0));
}

void ActivityRecord::clearCounters()
{
    d->key_presses = 0;
    d->mouse_clicks = 0;
    d->mouse_distance = 0;
}

QString ActivityRecord::tableName() const
{
    return d->table_name;
}

QDateTime ActivityRecord::localTime() const
{
    return d->local_time;
}

QDateTime ActivityRecord::utcTime() const
{
    return d->local_time.toUTC();
}

QUuid ActivityRecord::uuid() const
{
    return d->uuid;
}

QString ActivityRecord::projectId() const
{
    return d->uuid.toString(QUuid::WithoutBraces);
}

QString ActivityRecord::textNote() const
{
    return d->text_note;
}

int ActivityRecord::keyPresses() const
{
    return d->key_presses;
}

int ActivityRecord::mouseClicks() const
{
    return d->mouse_clicks;
}

int ActivityRecord::mouseDistance() const
{
    return d->mouse_distance;
}

void ActivityRecord::dump(QDebug &dbg) const
{
    QDebugStateSaver saver(dbg);
    dbg.noquote();
    dbg << "\n\t tableName"     << '"' << d->table_name << '"'
        << "\n\t localTime"     << d->local_time.toString(Qt::ISODate)
        << "\n\t projectId"     << d->uuid.toString(QUuid::WithoutBraces)
        << "\n\t textNote"      << '"' << d->text_note << '"'
        << "\n\t keyPresses"    << d->key_presses
        << "\n\t mouseClicks"   << d->mouse_clicks
        << "\n\t mouseDistance" << d->mouse_distance;
}
