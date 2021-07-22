#pragma once

namespace Katabasis {

/// Request parameter (name-value pair) participating in authentication.
struct RequestParameter {
    RequestParameter(const QByteArray &n, const QByteArray &v): name(n), value(v) {}
    bool operator <(const RequestParameter &other) const {
        return (name == other.name)? (value < other.value): (name < other.name);
    }
    QByteArray name;
    QByteArray value;
};

}
