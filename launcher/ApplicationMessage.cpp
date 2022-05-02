#include "ApplicationMessage.h"

#include <QJsonDocument>
#include <QJsonObject>
#include "Json.h"

void ApplicationMessage::parse(const QByteArray & input) {
    auto doc = Json::requireDocument(input, "ApplicationMessage");
    auto root = Json::requireObject(doc, "ApplicationMessage");

    command = root.value("command").toString();
    args.clear();

    auto parsedArgs = root.value("args").toObject();
    for(auto iter = parsedArgs.begin(); iter != parsedArgs.end(); iter++) {
        args[iter.key()] = iter.value().toString();
    }
}

QByteArray ApplicationMessage::serialize() {
    QJsonObject root;
    root.insert("command", command);
    QJsonObject outArgs;
    for (auto iter = args.begin(); iter != args.end(); iter++) {
        outArgs[iter.key()] = iter.value();
    }
    root.insert("args", outArgs);

    return Json::toText(root);
}
