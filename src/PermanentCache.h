#ifndef PERMANENTCACHE_H
#define PERMANENTCACHE_H

#include <QFileInfo>
#include <QDataStream>
#include <QDateTime>
#include <QLockFile>
#include <QSaveFile>

template <class K, class T>
class PermanentCache
{
public:
    PermanentCache() : modified(false) {}
    PermanentCache(const QString &path, int reserve = 0)
        : file_path(path), modified(false), last_time(0) {
        if (reserve > 0) data_hash.reserve(reserve);
        cacheLoad();
    }
    PermanentCache(const PermanentCache &pc) { cacheCopy(pc); }
    PermanentCache& operator=(const PermanentCache &pc) {
        if (this != &pc) { cacheCopy(pc); }
        return *this;
    }
    virtual ~PermanentCache() { cacheSave(); }

    void setCache(const QString &path, int reserve = 0) {
        if (path != file_path) {
            file_path = path;
            modified = false;
            last_time = 0;
            data_hash.clear();
        }
        if (reserve > 0) data_hash.reserve(reserve);
    }

    int cacheLoad() {
        if (!file_path.isEmpty()) {
            QFile file(file_path);
            if (file.open(QIODevice::ReadOnly)) {
                data_hash.clear();
                QDataStream in(&file);
                in >> data_hash;
                file.close();
                modified = false;
                last_time = QDateTime::currentMSecsSinceEpoch();
                return data_hash.size();
            }
        }
        return 0;
    }

    int cacheRefresh() {
        if (!file_path.isEmpty() && QFileInfo(file_path).lastModified().toMSecsSinceEpoch() > last_time)
            return cacheLoad();
        return 0;
    }

    int cacheSave(bool use_lock = false) {
        if (!file_path.isEmpty() && modified && !data_hash.isEmpty()) {
            QHash<K, T> prev_hash;
            prev_hash.swap(data_hash);
            if (cacheRefresh()) {
                // remove old first
                for (auto it = data_hash.begin(); it != data_hash.end(); ) {
                    if (!prev_hash.contains(it.key())) {
                        it = data_hash.erase(it);
                        modified = true;
                    } else ++it;
                }
                // insert new one
                for (auto it = prev_hash.constBegin(); it != prev_hash.constEnd(); ++it) {
                    if (!data_hash.contains(it.key())) {
                        data_hash.insert(it.key(), it.value());
                        modified = true;
                    } else if (data_hash[it.key()] != prev_hash[it.key()]) {
                        data_hash[it.key()] = prev_hash[it.key()];
                        modified = true;
                    }
                }
                if (!modified) return 0;
            } else data_hash.swap(prev_hash);

            if (use_lock) {
                QLockFile lock_file(file_path + QStringLiteral("-lock"));
                lock_file.setStaleLockTime(1500);
                if (!lock_file.lock()) return 0;
            }
            QSaveFile save_file(file_path);
            if (save_file.open(QIODevice::WriteOnly)) {
                QDataStream out(&save_file);
                out << data_hash;
            }
            if (save_file.isOpen() && save_file.commit()) {
                QFile::setPermissions(save_file.fileName(), QFile::Permission(0x664));
                modified = false;
                return data_hash.size();
            }
        }
        return 0;
    }

    inline int cacheCount() const { return data_hash.size(); }
    inline bool cacheContains(const K& key) const { return data_hash.contains(key); }
    inline const T cacheValue(const K& key) const { return data_hash.value(key); }
    inline const QHash<K, T> &cacheHash() const { return data_hash; }
    inline void cacheInsert(const K &key, const T &val) {
        if (!data_hash.contains(key) || data_hash.value(key) != val) {
            data_hash.insert(key, val);
            modified = true;
        }
    }

private:
    void cacheCopy(const PermanentCache &pc) {
        file_path = pc.file_path;
        modified = pc.modified;
        last_time = pc.last_time;
        data_hash = pc.data_hash;
    }

    QString file_path;
    bool modified;
    qint64 last_time;
    QHash<K, T> data_hash;
};

#endif // PERMANENTCACHE_H
