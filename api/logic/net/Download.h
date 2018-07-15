/* Copyright 2013-2018 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "NetAction.h"
#include "HttpMetaCache.h"
#include "Validator.h"
#include "Sink.h"

#include "multimc_logic_export.h"
namespace Net {
class MULTIMC_LOGIC_EXPORT Download : public NetAction
{
    Q_OBJECT

public: /* types */
    typedef std::shared_ptr<class Download> Ptr;
    enum class Option
    {
        NoOptions = 0,
        AcceptLocalFiles = 1
    };
    Q_DECLARE_FLAGS(Options, Option)

protected: /* con/des */
    explicit Download();
public:
    virtual ~Download(){};
    static Download::Ptr makeCached(QUrl url, MetaEntryPtr entry, Options options = Option::NoOptions);
    static Download::Ptr makeByteArray(QUrl url, QByteArray *output, Options options = Option::NoOptions);
    static Download::Ptr makeFile(QUrl url, QString path, Options options = Option::NoOptions);

public: /* methods */
    QString getTargetFilepath()
    {
        return m_target_path;
    }
    void addValidator(Validator * v);
    bool abort() override;
    bool canAbort() override;

private: /* methods */
    bool handleRedirect();

protected slots:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal) override;
    void downloadError(QNetworkReply::NetworkError error) override;
    void sslErrors(const QList<QSslError> & errors);
    void downloadFinished() override;
    void downloadReadyRead() override;

public slots:
    void start() override;

private: /* data */
    // FIXME: remove this, it has no business being here.
    QString m_target_path;
    std::unique_ptr<Sink> m_sink;
    Options m_options;
};
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Net::Download::Options)
