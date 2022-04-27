#pragma once

#include "net/NetAction.h"

#include "Validator.h"

namespace Net {
class Sink {
   public:
    Sink() = default;
    virtual ~Sink() = default;

   public:
    virtual auto init(QNetworkRequest& request) -> Task::State = 0;
    virtual auto write(QByteArray& data) -> Task::State = 0;
    virtual auto abort() -> Task::State = 0;
    virtual auto finalize(QNetworkReply& reply) -> Task::State = 0;

    virtual auto hasLocalData() -> bool = 0;

    void addValidator(Validator* validator)
    {
        if (validator) {
            validators.push_back(std::shared_ptr<Validator>(validator));
        }
    }

   protected:
    bool initAllValidators(QNetworkRequest& request)
    {
        for (auto& validator : validators) {
            if (!validator->init(request))
                return false;
        }
        return true;
    }
    bool finalizeAllValidators(QNetworkReply& reply)
    {
        for (auto& validator : validators) {
            if (!validator->validate(reply))
                return false;
        }
        return true;
    }
    bool failAllValidators()
    {
        bool success = true;
        for (auto& validator : validators) {
            success &= validator->abort();
        }
        return success;
    }
    bool writeAllValidators(QByteArray& data)
    {
        for (auto& validator : validators) {
            if (!validator->write(data))
                return false;
        }
        return true;
    }

   protected:
    std::vector<std::shared_ptr<Validator>> validators;
};
}  // namespace Net
