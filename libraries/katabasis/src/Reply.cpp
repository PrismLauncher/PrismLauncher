#include <QTimer>
#include <QNetworkReply>

#include "katabasis/Reply.h"

namespace Katabasis {

Reply::Reply(QNetworkReply *r, int timeOut, QObject *parent): QTimer(parent), reply(r) {
    setSingleShot(true);
    connect(this, &Reply::timeout, this, &Reply::onTimeOut, Qt::QueuedConnection);
    start(timeOut);
}

void Reply::onTimeOut() {
    timedOut = true;
    reply->abort();
}

// ----------------------------

ReplyList::~ReplyList() {
    foreach (Reply *timedReply, replies_) {
        delete timedReply;
    }
}

void ReplyList::add(QNetworkReply *reply, int timeOut) {
    if (reply && ignoreSslErrors()) {
        reply->ignoreSslErrors();
    }
    add(new Reply(reply, timeOut));
}

void ReplyList::add(Reply *reply) {
    replies_.append(reply);
}

void ReplyList::remove(QNetworkReply *reply) {
    Reply *o2Reply = find(reply);
    if (o2Reply) {
        o2Reply->stop();
        (void)replies_.removeOne(o2Reply);
        // we took ownership, we must free
        delete o2Reply;
    }
}

Reply *ReplyList::find(QNetworkReply *reply) {
    foreach (Reply *timedReply, replies_) {
        if (timedReply->reply == reply) {
            return timedReply;
        }
    }
    return 0;
}

bool ReplyList::ignoreSslErrors()
{
    return ignoreSslErrors_;
}

void ReplyList::setIgnoreSslErrors(bool ignoreSslErrors)
{
    ignoreSslErrors_ = ignoreSslErrors;
}

}
