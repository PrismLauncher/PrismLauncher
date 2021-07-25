#pragma once

#include "net/NetAction.h"

#include "Validator.h"

namespace Net {
class Sink
{
public: /* con/des */
    Sink() {};
    virtual ~Sink() {};

public: /* methods */
    virtual JobStatus init(QNetworkRequest & request) = 0;
    virtual JobStatus write(QByteArray & data) = 0;
    virtual JobStatus abort() = 0;
    virtual JobStatus finalize(QNetworkReply & reply) = 0;
    virtual bool hasLocalData() = 0;

    void addValidator(Validator * validator)
    {
        if(validator)
        {
            validators.push_back(std::shared_ptr<Validator>(validator));
        }
    }

protected: /* methods */
    bool finalizeAllValidators(QNetworkReply & reply)
    {
        for(auto & validator: validators)
        {
            if(!validator->validate(reply))
                return false;
        }
        return true;
    }
    bool failAllValidators()
    {
        bool success = true;
        for(auto & validator: validators)
        {
            success &= validator->abort();
        }
        return success;
    }
    bool initAllValidators(QNetworkRequest & request)
    {
        for(auto & validator: validators)
        {
            if(!validator->init(request))
                return false;
        }
        return true;
    }
    bool writeAllValidators(QByteArray & data)
    {
        for(auto & validator: validators)
        {
            if(!validator->write(data))
                return false;
        }
        return true;
    }

protected: /* data */
    std::vector<std::shared_ptr<Validator>> validators;
};
}
