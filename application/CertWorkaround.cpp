#include <stdexcept>
#include <iostream>

#include <QByteArray>
#include <QSslSocket>
#include <QDebug>

#include <Security/Security.h>

// CFRelease will crash if passed NULL
#define SafeCFRelease(ref)                                                                     \
	if (ref)                                                                                   \
		CFRelease(ref);

/*!
 * \brief LoadCertificatesFromKeyChain  Load all certificates from the KeyChain path provided
 * and return them as
 *                                      QSslCertificates.
 * \param keyChainPath                  The KeyChain path. Pass an empty string to use the
 * user's keychain.
 * \return                              A list of new QSslCertificates generated from the
 * KeyChain DER data.
 */
static QList<QSslCertificate>
LoadCertificatesFromKeyChain(const std::string &keyChainPath = std::string())
{
	QList<QSslCertificate> qtCerts;

	SecKeychainRef certsKeyChain = NULL;
	SecKeychainSearchRef searchItem = NULL;
	SecKeychainItemRef itemRef = NULL;
	CSSM_DATA certData = {0, 0};

	try
	{
		OSStatus status = errSecSuccess;

		// if a keychain path was provided, obtain a pointer
		if (!keyChainPath.empty())
		{
			status = SecKeychainOpen(keyChainPath.c_str(), &certsKeyChain);
			if (status != errSecSuccess)
			{
				throw status;
			}
		}

		// build a search query reference for certificates
		status = SecKeychainSearchCreateFromAttributes(certsKeyChain, kSecCertificateItemClass,
													   NULL, &searchItem);
		if (status != errSecSuccess)
		{
			throw status;
		}

		// loop through the certificates
		while (SecKeychainSearchCopyNext(searchItem, &itemRef) != errSecItemNotFound)
		{
			// copy the KeyChain item data into a CSSM_DATA struct - this will be the certs Der
			// data
			status = SecKeychainItemCopyContent(itemRef, NULL, NULL,
												reinterpret_cast<UInt32 *>(&certData.Length),
												reinterpret_cast<void **>(&certData.Data));

			if (status != errSecSuccess)
			{
				throw status;
			}

			// create a Qt byte array from the data - the data is NOT copied
			const QByteArray byteArray = QByteArray::fromRawData(
				reinterpret_cast<const char *>(certData.Data), certData.Length);

			// create a Qt certificate from the data and add it to the list
			QSslCertificate qtCert(byteArray, QSsl::Der);
			qDebug() << "COMMON NAME: "
					 << qtCert.issuerInfo(QSslCertificate::CommonName).join('\n')
					 << " ORG NAME: "
					 << qtCert.issuerInfo(QSslCertificate::Organization).join('\n');

			qtCerts << qtCert;
		}
	}
	catch (OSStatus status)
	{
		CFStringRef errorMessage = SecCopyErrorMessageString(status, NULL);
		std::cerr << CFStringGetCStringPtr(errorMessage, kCFStringEncodingMacRoman)
				  << std::endl;
		SafeCFRelease(errorMessage);
	}

	SecKeychainItemFreeContent(NULL, certData.Data);
	SafeCFRelease(itemRef);
	SafeCFRelease(searchItem);
	SafeCFRelease(certsKeyChain);

	return qtCerts;
}

void RebuildQtCertificates()
{
	const QList<QSslCertificate> existingCerts = QSslSocket::defaultCaCertificates();
	QList<QSslCertificate> certs = LoadCertificatesFromKeyChain();
	certs += LoadCertificatesFromKeyChain(
		"/System/Library/Keychains/SystemRootCertificates.keychain");

	Q_FOREACH (const QSslCertificate qtCert, certs)
	{
		if (!existingCerts.contains(qtCert))
		{
			qDebug() << "cert not known to Qt - adding";
			qDebug() << "COMMON NAME: "
					 << qtCert.issuerInfo(QSslCertificate::CommonName).join('\n')
					 << " ORG NAME: "
					 << qtCert.issuerInfo(QSslCertificate::Organization).join('\n');

			QSslSocket::addDefaultCaCertificate(qtCert);
		}
	}
}
