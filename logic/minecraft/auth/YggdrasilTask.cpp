/* Copyright 2013-2015 MultiMC Contributors
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

#include "YggdrasilTask.h"
#include "MojangAccount.h"

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QByteArray>

#include <Env.h>

#include <net/URLConstants.h>

#include <QDebug>

YggdrasilTask::YggdrasilTask(MojangAccount *account, QObject *parent)
	: Task(parent), m_account(account)
{
	changeState(STATE_CREATED);
}

void YggdrasilTask::executeTask()
{
	changeState(STATE_SENDING_REQUEST);

	// Get the content of the request we're going to send to the server.
	QJsonDocument doc(getRequestContent());

	auto worker = ENV.qnam();
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

bool YggdrasilTask::abort()
{
	progress(timeout_max, timeout_max);
	// TODO: actually use this in a meaningful way
	m_aborted = YggdrasilTask::BY_USER;
	m_netReply->abort();
	return true;
}

void YggdrasilTask::abortByTimeout()
{
	progress(timeout_max, timeout_max);
	// TODO: actually use this in a meaningful way
	m_aborted = YggdrasilTask::BY_TIMEOUT;
	m_netReply->abort();
}

void YggdrasilTask::sslErrors(QList<QSslError> errors)
{
	int i = 1;
	for (auto error : errors)
	{
		qCritical() << "LOGIN SSL Error #" << i << " : " << error.errorString();
		auto cert = error.certificate();
		qCritical() << "Certificate in question:\n" << cert.toText();
		i++;
	}
}

void YggdrasilTask::processReply()
{
	changeState(STATE_PROCESSING_RESPONSE);

	switch (m_netReply->error())
	{
	case QNetworkReply::NoError:
		break;
	case QNetworkReply::TimeoutError:
		changeState(STATE_FAILED_SOFT, tr("Authentication operation timed out."));
		return;
	case QNetworkReply::OperationCanceledError:
		changeState(STATE_FAILED_SOFT, tr("Authentication operation cancelled."));
		return;
	case QNetworkReply::SslHandshakeFailedError:
		changeState(
			STATE_FAILED_SOFT,
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
	// used for invalid credentials and similar errors. Fall through.
	case QNetworkReply::ContentOperationNotPermittedError:
		break;
	default:
		changeState(STATE_FAILED_SOFT,
					tr("Authentication operation failed due to a network error: %1 (%2)")
						.arg(m_netReply->errorString()).arg(m_netReply->error()));
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
			processResponse(replyData.size() > 0 ? doc.object() : QJsonObject());
			return;
		}
		else
		{
			changeState(STATE_FAILED_SOFT, tr("Failed to parse authentication server response "
											  "JSON response: %1 at offset %2.")
											   .arg(jsonError.errorString())
											   .arg(jsonError.offset));
			qCritical() << replyData;
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
		qDebug() << "The request failed, but the server gave us an error message. "
						"Processing error.";
		processError(doc.object());
	}
	else
	{
		// The server didn't say anything regarding the error. Give the user an unknown
		// error.
		qDebug()
			<< "The request failed and the server gave no error message. Unknown error.";
		changeState(STATE_FAILED_SOFT,
					tr("An unknown error occurred when trying to communicate with the "
					   "authentication server: %1").arg(m_netReply->errorString()));
	}
}

void YggdrasilTask::processError(QJsonObject responseData)
{
	QJsonValue errorVal = responseData.value("error");
	QJsonValue errorMessageValue = responseData.value("errorMessage");
	QJsonValue causeVal = responseData.value("cause");

	if (errorVal.isString() && errorMessageValue.isString())
	{
		m_error = std::shared_ptr<Error>(new Error{
			errorVal.toString(""), errorMessageValue.toString(""), causeVal.toString("")});
		changeState(STATE_FAILED_HARD, m_error->m_errorMessageVerbose);
	}
	else
	{
		// Error is not in standard format. Don't set m_error and return unknown error.
		changeState(STATE_FAILED_HARD, tr("An unknown Yggdrasil error occurred."));
	}
}

QString YggdrasilTask::getStateMessage() const
{
	switch (m_state)
	{
	case STATE_CREATED:
		return "Waiting...";
	case STATE_SENDING_REQUEST:
		return tr("Sending request to auth servers...");
	case STATE_PROCESSING_RESPONSE:
		return tr("Processing response from servers...");
	case STATE_SUCCEEDED:
		return tr("Authentication task succeeded.");
	case STATE_FAILED_SOFT:
		return tr("Failed to contact the authentication server.");
	case STATE_FAILED_HARD:
		return tr("Failed to authenticate.");
	default:
		return tr("...");
	}
}

void YggdrasilTask::changeState(YggdrasilTask::State newState, QString reason)
{
	m_state = newState;
	setStatus(getStateMessage());
	if (newState == STATE_SUCCEEDED)
	{
		emitSucceeded();
	}
	else if (newState == STATE_FAILED_HARD || newState == STATE_FAILED_SOFT)
	{
		emitFailed(reason);
	}
}

YggdrasilTask::State YggdrasilTask::state()
{
	return m_state;
}
