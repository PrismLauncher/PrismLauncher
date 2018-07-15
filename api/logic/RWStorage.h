#pragma once
#include <QWriteLocker>
#include <QReadLocker>
template <typename K, typename V>
class RWStorage
{
public:
    void add(K key, V value)
    {
        QWriteLocker l(&lock);
        cache[key] = value;
        stale_entries.remove(key);
    }
    V get(K key)
    {
        QReadLocker l(&lock);
        if(cache.contains(key))
        {
            return cache[key];
        }
        else return V();
    }
    bool get(K key, V& value)
    {
        QReadLocker l(&lock);
        if(cache.contains(key))
        {
            value = cache[key];
            return true;
        }
        else return false;
    }
    bool has(K key)
    {
        QReadLocker l(&lock);
        return cache.contains(key);
    }
    bool stale(K key)
    {
        QReadLocker l(&lock);
        if(!cache.contains(key))
            return true;
        return stale_entries.contains(key);
    }
    void setStale(K key)
    {
        QWriteLocker l(&lock);
        if(cache.contains(key))
        {
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
