#pragma once

enum class InstanceShortcutTarget : std::uint8_t { Desktop, Applications, Other, Count };

struct InstanceShortcut {
    QString name;
    QString filePath;
    InstanceShortcutTarget target;

    bool operator==(const InstanceShortcut&) const = default;
};
