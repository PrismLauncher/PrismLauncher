/* Copyright 2013 MultiMC Contributors
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

#include <logic/auth/YggdrasilTask.h>

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QByteArray>

#include <MultiMC.h>
#include <logic/auth/MojangAccount.h>

YggdrasilTask::YggdrasilTask(MojangAccountPtr account, QObject *parent) : Task(parent)
{
	m_error = nullptr;
	m_account = account;
}

YggdrasilTask::~YggdrasilTask()
{
	if (m_error)
		delete m_error;
}

void YggdrasilTask::executeTask()
{
	setStatus(getStateMessage(STATE_SENDING_REQUEST));

	// Get the content of the request we're going to send to the server.
	QJsonDocument doc(getRequestContent());

	auto worker = MMC->qnam();
	connect(worker.get(), SIGNAL(finished(QNetworkReply *)), this,
			SLOT(processReply(QNetworkReply *)));

	QUrl reqUrl("https://authserver.mojang.com/" + getEndpoint());
	QNetworkRequest netRequest(reqUrl);
	netRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QByteArray requestData = doc.toJson();
	m_netReply = worker->post(netRequest, requestData);
}

void YggdrasilTask::processReply(QNetworkReply *reply)
{
	setStatus(getStateMessage(STATE_PROCESSING_RESPONSE));

	if (m_netReply != reply)
		// Wrong reply for some reason...
		return;

	if (reply->error() == QNetworkReply::OperationCanceledError)
	{
		emitFailed("Yggdrasil task cancelled.");
		return;
	}
	else
	{
		// Try to parse the response regardless of the response code.
		// Sometimes the auth server will give more information and an error code.
		QJsonParseError jsonError;
		QByteArray replyData = reply->readAll();
		QJsonDocument doc = QJsonDocument::fromJson(replyData, &jsonError);
		// Check the response code.
		int responseCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

		if (responseCode == 200)
		{
			// If the response code was 200, then there shouldn't be an error. Make sure anyways.
			// Also, sometimes an empty reply indicates success. If there was no data received, 
			// pass an empty json object to the processResponse function.
			if (jsonError.error == QJsonParseError::NoError || replyData.size() == 0)
			{
				if (!processResponse(replyData.size() > 0 ? doc.object() : QJsonObject()))
				{
					YggdrasilTask::Error *err = getError();
					if (err)
						emitFailed(err->getErrorMessage());
					else
						emitFailed(tr("An unknown error occurred when processing the response "
									  "from the authentication server."));
				}
				else
				{
					emitSucceeded();
				}
			}
			else
			{
				emitFailed(tr("Failed to parse Yggdrasil JSON response: %1 at offset %2.").arg(jsonError.errorString()).arg(jsonError.offset));
			}
		}
		else
		{
			// If the response code was not 200, then Yggdrasil may have given us information about the error.
			// If we can parse the response, then get information from it. Otherwise just say there was an unknown error.
			if (jsonError.error == QJsonParseError::NoError)
			{
				// We were able to parse the server's response. Woo!
				// Call processError. If a subclass has overridden it then they'll handle their stuff there.
				QLOG_DEBUG() << "The request failed, but the server gave us an error message. Processing error.";
				emitFailed(processError(doc.object()));
			}
			else
			{
				// The server didn't say anything regarding the error. Give the user an unknown error.
				QLOG_DEBUG() << "The request failed and the server gave no error message. Unknown error.";
				emitFailed(tr("An unknown error occurred when trying to communicate with the authentication server: %1").arg(reply->errorString()));
			}
		}
	}
}

QString YggdrasilTask::processError(QJsonObject responseData)
{
	QJsonValue errorVal = responseData.value("error");
	QJsonValue msgVal = responseData.value("errorMessage");
	QJsonValue causeVal = responseData.value("cause");

	if (errorVal.isString() && msgVal.isString())
	{
		m_error = new Error(errorVal.toString(""), msgVal.toString(""), causeVal.toString(""));
		return m_error->getDisplayMessage();
	}
	else
	{
		// Error is not in standard format. Don't set m_error and return unknown error.
		return tr("An unknown Yggdrasil error occurred.");
	}
}

QString YggdrasilTask::getStateMessage(const YggdrasilTask::State state) const
{
	switch (state)
	{
	case STATE_SENDING_REQUEST:
		return tr("Sending request to auth servers.");
	case STATE_PROCESSING_RESPONSE:
		return tr("Processing response from servers.");
	default:
		return tr("Processing. Please wait.");
	}
}

YggdrasilTask::Error *YggdrasilTask::getError() const
{
	return this->m_error;
}

MojangAccountPtr YggdrasilTask::getMojangAccount() const
{
	return this->m_account;
}
