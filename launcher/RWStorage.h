#pragma once
#include <QMap>
#include <QReadLocker>
#include <QSet>
#include <QWriteLocker>
#include <utility>

template <typename K, typename V>
class RWStorage {
   public:
    void add(const K& key, V value)
    {
        QWriteLocker l(&lock);
        cache[key] = std::move(value);
        stale_entries.remove(key);
    }
    V get(const K& key)
    {
        QReadLocker l(&lock);
        if (cache.contains(key)) {
            return cache[key];
        }
        return V();
    }
    bool get(const K& key, V& value)
    {
        QReadLocker l(&lock);
        if (cache.contains(key)) {
            value = cache[key];
            return true;
        }
        return false;
    }
    bool has(K key)
    {
        QReadLocker l(&lock);
        return cache.contains(key);
    }
    bool stale(const K& key)
    {
        QReadLocker l(&lock);
        if (!cache.contains(key)) {
            return true;
        }
        return stale_entries.contains(key);
    }
    void setStale(const K& key)
    {
        QWriteLocker l(&lock);
        if (cache.contains(key)) {
            stale_entries.insert(key);
        }
    }
    void clear()
    {
        QWriteLocker l(&lock);
        cache.clear();
        stale_entries.clear();
    }

   private:
    QReadWriteLock lock;
    QMap<K, V> cache;
    QSet<K> stale_entries;
};
