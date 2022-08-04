#pragma once

#include <QObject>
#include <QSharedPointer>
#include <functional>
#include <memory>

namespace details {
[[maybe_unused]] static void do_delete_later(QObject* obj)
{
    if (obj)
        obj->deleteLater();
}
struct DeleteQObjectLater {
    void operator()(QObject* obj) const { do_delete_later(obj); }
};

}  // namespace details

/**
 * A unique pointer class with unique pointer semantics intended for derivates of QObject
 * Calls deleteLater() instead of destroying the contained object immediately
 */
template <typename T>
using unique_qobject_ptr = std::unique_ptr<T, details::DeleteQObjectLater>;

/**
 * A shared pointer class with shared pointer semantics intended for derivates of QObject
 * Calls deleteLater() instead of destroying the contained object immediately
 */
template <typename T>
class shared_qobject_ptr : public QSharedPointer<T> {
   public:
    constexpr shared_qobject_ptr() : QSharedPointer<T>() {}
    constexpr shared_qobject_ptr(T* ptr) : QSharedPointer<T>(ptr, details::do_delete_later) {}
    constexpr shared_qobject_ptr(std::nullptr_t null_ptr) : QSharedPointer<T>(null_ptr, details::do_delete_later) {}

    template <typename Derived>
    constexpr shared_qobject_ptr(const shared_qobject_ptr<Derived>& other) : QSharedPointer<T>(other)
    {}

    void reset() { QSharedPointer<T>::reset(); }
    void reset(const shared_qobject_ptr<T>& other)
    {
        shared_qobject_ptr<T> t(other);
        this->swap(t);
    }
};
