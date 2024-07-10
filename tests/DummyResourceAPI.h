#pragma once

#include <QJsonDocument>

#include <modplatform/ResourceAPI.h>

class SearchTask : public TaskV2 {
    Q_OBJECT

   public:
    void executeTask() override { emitSucceeded(); }
};

class DummyResourceAPI : public ResourceAPI {
   public:
    static auto searchRequestResult()
    {
        static QByteArray json_response =
            "{\"hits\":["
            "{"
            "\"author\":\"flowln\","
            "\"description\":\"the bestest mod\","
            "\"project_id\":\"something\","
            "\"project_type\":\"mod\","
            "\"slug\":\"bip_bop\","
            "\"title\":\"AAAAAAAA\","
            "\"versions\":[\"2.71\"]"
            "}"
            "]}";

        return QJsonDocument::fromJson(json_response);
    }

    DummyResourceAPI() : ResourceAPI() {}
    [[nodiscard]] auto getSortingMethods() const -> QList<SortingMethod> override { return {}; }

    [[nodiscard]] TaskV2::Ptr searchProjects(SearchArgs&&, SearchCallbacks&& callbacks) const override
    {
        auto task = makeShared<SearchTask>();
        QObject::connect(task.get(), &TaskV2::finished, [=] {
            auto json = searchRequestResult();
            callbacks.on_succeed(json);
        });
        return task;
    }
};
