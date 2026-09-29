#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>

#include "Result.h"

#include "cli/Commands.h"

struct ApplicationMessage {
    QString command;
    QHash<QString, QString> args;

    QByteArray serialize() const;
    Result<> parse(const QByteArray& input);

    static ApplicationMessage fromCliCommand(const Cli::Command& cmd);

    Result<Cli::Command> toCliCommand() const;
};
