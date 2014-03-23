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
#include <logic/net/URLConstants.h>

YggdrasilTask::YggdrasilTask(MojangAccount *account, QObject *parent)
	: Task(parent), m_account(account)
{
}

void YggdrasilTask::executeTask()
{
	setStatus(getStateMessage(STATE_SENDING_REQUEST));

	// Get the content of the request we're going to send to the server.
	QJsonDocument doc(getRequestContent());

	auto worker = MMC->qnam();
	QUrl reqUrl("https://" + URLConstants::AUTH_BASE + getEndpoint());
	QNetworkRequest netRequest(reqUrl);
	netRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QByteArray requestData = doc.toJson();
	m_netReply = worker->post(netRequest, requestData);
	connect(m_netReply, &QNetworkReply::finished, this, &YggdrasilTask::processReply);
	connect(m_netReply, &QNetworkReply::uploadProgress, this, &YggdrasilTask::refreshTimers);
	connect(m_netReply, &QNetworkReply::downloadProgress, this, &YggdrasilTask::refreshTimers);
	connect(m_netReply, &QNetworkReply::sslErrors, this, &YggdrasilTask::sslErrors);
	timeout_keeper.setSingleShot(true);
	timeout_keeper.start(timeout_max);
	counter.setSingleShot(false);
	counter.start(time_step);
	progress(0, timeout_max);
	connect(&timeout_keeper, &QTimer::timeout, this, &YggdrasilTask::abortByTimeout);
	connect(&counter, &QTimer::timeout, this, &YggdrasilTask::heartbeat);
}

void YggdrasilTask::refreshTimers(qint64, qint64)
{
	timeout_keeper.stop();
	timeout_keeper.start(timeout_max);
	progress(count = 0, timeout_max);
}
void YggdrasilTask::heartbeat()
{
	count += time_step;
	progress(count, timeout_max);
}

void YggdrasilTask::abort()
{
	progress(timeout_max, timeout_max);
	m_netReply->abort();
}

void YggdrasilTask::abortByTimeout()
{
	progress(timeout_max, timeout_max);
	m_netReply->abort();
}

void YggdrasilTask::sslErrors(QList<QSslError> errors)
{
	int i = 1;
	for (auto error : errors)
	{
		QLOG_ERROR() << "LOGIN SSL Error #" << i << " : " << error.errorString();
		auto cert = error.certificate();
		QLOG_ERROR() << "Certificate in question:\n" << cert.toText();
		i++;
	}
}

void YggdrasilTask::processReply()
{
	setStatus(getStateMessage(STATE_PROCESSING_RESPONSE));

	if (m_netReply->error() == QNetworkReply::SslHandshakeFailedError)
	{
		emitFailed(
			tr("<b>SSL Handshake failed.</b><br/>There might be a few causes for it:<br/>"
			   "<ul>"
			   "<li>You use Windows XP and need to <a "
			   "href=\"http://www.microsoft.com/en-us/download/details.aspx?id=38918\">update "
			   "your root certificates</a></li>"
			   "<li>Some device on your network is interfering with SSL traffic. In that case, "
			   "you have bigger worries than Minecraft not starting.</li>"
			   "<li>Possibly something else. Check the MultiMC log file for details</li>"
			   "</ul>"));
		return;
	}

	// any network errors lead to offline mode right now
	if (m_netReply->error() >= QNetworkReply::ConnectionRefusedError &&
		m_netReply->error() <= QNetworkReply::UnknownNetworkError)
	{
		// WARNING/FIXME: the value here is used in MojangAccount to detect the cancel/timeout
		emitFailed("Yggdrasil task cancelled.");
		QLOG_ERROR() << "Yggdrasil task cancelled because of: " << m_netReply->error() << " : "
					 << m_netReply->errorString();
		return;
	}

	// Try to parse the response regardless of the response code.
	// Sometimes the auth server will give more information and an error code.
	QJsonParseError jsonError;
	QByteArray replyData = m_netReply->readAll();
	QJsonDocument doc = QJsonDocument::fromJson(replyData, &jsonError);
	// Check the response code.
	int responseCode = m_netReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (responseCode == 200)
	{
		// If the response code was 200, then there shouldn't be an error. Make sure
		// anyways.
		// Also, sometimes an empty reply indicates success. If there was no data received,
		// pass an empty json object to the processResponse function.
		if (jsonError.error == QJsonParseError::NoError || replyData.size() == 0)
		{
			if (processResponse(replyData.size() > 0 ? doc.object() : QJsonObject()))
			{
				emitSucceeded();
				return;
			}

			// errors happened anyway?
			emitFailed(m_error ? m_error->m_errorMessageVerbose
							   : tr("An unknown error occurred when processing the response "
									"from the authentication server."));
		}
		else
		{
			emitFailed(tr("Failed to parse Yggdrasil JSON response: %1 at offset %2.")
						   .arg(jsonError.errorString())
						   .arg(jsonError.offset));
		}
		return;
	}

	// If the response code was not 200, then Yggdrasil may have given us information
	// about the error.
	// If we can parse the response, then get information from it. Otherwise just say
	// there was an unknown error.
	if (jsonError.error == QJsonParseError::NoError)
	{
		// We were able to parse the server's response. Woo!
		// Call processError. If a subclass has overridden it then they'll handle their
		// stuff there.
		QLOG_DEBUG() << "The request failed, but the server gave us an error message. "
						"Processing error.";
		emitFailed(processError(doc.object()));
	}
	else
	{
		// The server didn't say anything regarding the error. Give the user an unknown
		// error.
		QLOG_DEBUG() << "The request failed and the server gave no error message. "
						"Unknown error.";
		emitFailed(tr("An unknown error occurred when trying to communicate with the "
					  "authentication server: %1").arg(m_netReply->errorString()));
	}
}

QString YggdrasilTask::processError(QJsonObject responseData)
{
	QJsonValue errorVal = responseData.value("error");
	QJsonValue errorMessageValue = responseData.value("errorMessage");
	QJsonValue causeVal = responseData.value("cause");

	if (errorVal.isString() && errorMessageValue.isString())
	{
		m_error = std::shared_ptr<Error>(new Error{
			errorVal.toString(""), errorMessageValue.toString(""), causeVal.toString("")});
		return m_error->m_errorMessageVerbose;
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
		return tr("Sending request to auth servers...");
	case STATE_PROCESSING_RESPONSE:
		return tr("Processing response from servers...");
	default:
		return tr("Processing. Please wait...");
	}
}
