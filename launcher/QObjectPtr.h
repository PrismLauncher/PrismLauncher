#pragma once

#include <QObject>
#include <QSharedPointer>

#include <functional>
#include <memory>

/**
 * A unique pointer class with unique pointer semantics intended for derivates of QObject
 * Calls deleteLater() instead of destroying the contained object immediately
 */
template <typename T>
using unique_qobject_ptr = QScopedPointer<T, QScopedPointerDeleteLater>;

/**
 * A shared pointer class with shared pointer semantics intended for derivates of QObject
 * Calls deleteLater() instead of destroying the contained object immediately
 */
template <typename T>
class shared_qobject_ptr : public QSharedPointer<T> {
   public:
    constexpr explicit shared_qobject_ptr() : QSharedPointer<T>() {}
    constexpr explicit shared_qobject_ptr(T* ptr) : QSharedPointer<T>(ptr, &QObject::deleteLater) {}
    constexpr shared_qobject_ptr(std::nullptr_t null_ptr) : QSharedPointer<T>(null_ptr, &QObject::deleteLater) {}

    template <typename Derived>
    constexpr shared_qobject_ptr(const shared_qobject_ptr<Derived>& other) : QSharedPointer<T>(other)
    {}

    template <typename Derived>
    constexpr shared_qobject_ptr(const QSharedPointer<Derived>& other) : QSharedPointer<T>(other)
    {}

    void reset() { QSharedPointer<T>::reset(); }
    void reset(T* other)
    {
        shared_qobject_ptr<T> t(other);
        this->swap(t);
    }
    void reset(const shared_qobject_ptr<T>& other)
    {
        shared_qobject_ptr<T> t(other);
        this->swap(t);
    }
};

template <typename T, typename... Args>
shared_qobject_ptr<T> makeShared(Args... args)
{
    auto obj = new T(args...);
    return shared_qobject_ptr<T>(obj);
}
