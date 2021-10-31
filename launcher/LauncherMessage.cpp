#include "LauncherMessage.h"

#include <QJsonDocument>
#include <QJsonObject>

void LauncherMessage::parse(const QByteArray & input) {
    auto doc = QJsonDocument::fromBinaryData(input);
    auto root = doc.object();

    command = root.value("command").toString();
    args.clear();

    auto parsedArgs = root.value("args").toObject();
    for(auto iter = parsedArgs.begin(); iter != parsedArgs.end(); iter++) {
        args[iter.key()] = iter.value().toString();
    }
}

QByteArray LauncherMessage::serialize() {
    QJsonObject root;
    root.insert("command", command);
    QJsonObject outArgs;
    for (auto iter = args.begin(); iter != args.end(); iter++) {
        outArgs[iter.key()] = iter.value();
    }
    root.insert("args", outArgs);

    QJsonDocument out;
    out.setObject(root);
    return out.toBinaryData();
}
