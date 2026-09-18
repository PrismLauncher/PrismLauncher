#include "McClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QTcpSocket>
#include <expected>
#include <utility>

#include "Json.h"
#include "Result.h"

namespace {

// From https://wiki.vg/Protocol#VarInt_and_VarLong
constexpr auto g_varIntValueMask = 0x7F;
constexpr auto g_varIntContinue = 0x80;

void writeVarInt(QByteArray& data, int value)
{
    while ((value & ~g_varIntValueMask) != 0) {  // check if the value is too big to fit in 7 bits
        // Write 7 bits
        data.append(static_cast<uint8_t>((value & ~g_varIntValueMask) | g_varIntContinue));

        // Erase theses 7 bits from the value to write
        // Note: >>> means that the sign bit is shifted with the rest of the number rather than being left alone
        value >>= 7;
    }
    data.append(static_cast<uint8_t>(value));
}

Result<uint8_t> readByte(QByteArray& data)
{
    if (data.isEmpty()) {
        return std::unexpected("No more bytes to read");
    }

    const uint8_t byte = data.at(0);
    data.remove(0, 1);
    return byte;
}

// From https://wiki.vg/Protocol#VarInt_and_VarLong
Result<int> readVarInt(QByteArray& data)
{
    int value = 0;
    int position = 0;

    while (position < 32) {
        TRY_INTO(const auto& currentByte, readByte(data))
        value |= (currentByte & g_varIntValueMask) << position;

        if ((currentByte & g_varIntContinue) == 0) {
            break;
        }

        position += 7;
    }

    if (position >= 32) {
        return std::unexpected("VarInt is too big");
    }

    return value;
}

void writeUInt16(QByteArray& data, const uint16_t value)
{
    QDataStream stream(&data, QIODeviceBase::Append);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << value;
}

void writeString(QByteArray& data, const QString& value)
{
    writeVarInt(data, static_cast<int32_t>(value.size()));
    data.append(value.toUtf8());
}

}  // namespace

McClient::McClient(QObject* parent, QString domain, QString ip, const uint16_t port)
    : QObject(parent), m_domain(std::move(domain)), m_ip(std::move(ip)), m_port(port)
{}

void McClient::getStatusData()
{
    qDebug() << "Connecting to socket..";

    connect(&m_socket, &QTcpSocket::connected, this, [this]() {
        qDebug() << "Connected to socket successfully";
        sendRequest();

        connect(&m_socket, &QTcpSocket::readyRead, this, &McClient::readRawResponse);
    });

    connect(&m_socket, &QTcpSocket::errorOccurred, this, [this]() { emitFail("Socket disconnected: " + m_socket.errorString()); });

    m_socket.connectToHost(m_ip, m_port);
}

void McClient::sendRequest()
{
    QByteArray data;
    writeVarInt(data, 0x00);      // packet ID
    writeVarInt(data, 763);       // hardcoded protocol version (763 = 1.20.1)
    writeString(data, m_domain);  // server address
    writeUInt16(data, m_port);    // server port
    writeVarInt(data, 0x01);      // next state
    writePacketToSocket(data);    // send handshake packet

    writeVarInt(data, 0x00);    // packet ID
    writePacketToSocket(data);  // send status packet
}

void McClient::readRawResponse()
{
    if (m_responseReadState == ResponseReadState::Finished) {
        return;
    }

    m_resp.append(m_socket.readAll());
    if (m_responseReadState == ResponseReadState::Waiting && m_resp.size() >= 5) {
        auto res = readVarInt(m_resp);
        if (!res) {
            emitFail(res.error());
            return;
        }
        m_wantedRespLength = *res;
        m_responseReadState = ResponseReadState::GotLength;
    }

    if (m_responseReadState == ResponseReadState::GotLength && m_resp.size() >= m_wantedRespLength) {
        if (m_resp.size() > m_wantedRespLength) {
            qDebug().nospace() << "Warning: Packet length doesn't match actual packet size (" << m_wantedRespLength << " expected vs "
                               << m_resp.size() << " received)";
        }
        parseResponse();
        m_responseReadState = ResponseReadState::Finished;
    }
}

void McClient::parseResponse()
{
    qDebug() << "Received response successfully";

    const auto packetID = readVarInt(m_resp);
    if (!packetID) {
        emitFail(packetID.error());
        return;
    }
    if (*packetID != 0x00) {
        emitFail(QString("Packet ID doesn't match expected value (0x00 vs 0x%1)").arg(*packetID, 0, 16));
        return;
    }

    {
        auto result = readVarInt(m_resp);  // json length
        if (!result) {
            emitFail(result.error());
            return;
        }
    }

    // 'resp' should now be the JSON string
    QJsonParseError parseError;
    const QJsonDocument doc = Json::parseUntilGarbage(m_resp, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse JSON:" << parseError.errorString();
        emitFail(parseError.errorString());
        return;
    }
    emitSucceed(doc.object());
}

void McClient::writePacketToSocket(QByteArray& data)
{
    // we prefix the packet with its length
    QByteArray dataWithSize;
    writeVarInt(dataWithSize, static_cast<int32_t>(data.size()));
    dataWithSize.append(data);

    // write it to the socket
    m_socket.write(dataWithSize);
    m_socket.flush();

    data.clear();
}

void McClient::emitFail(const QString& error)
{
    qDebug() << "Minecraft server ping for status error:" << error;
    emit failed(error);
    emit finished();
}

void McClient::emitSucceed(QJsonObject data)
{
    emit succeeded(std::move(data));
    emit finished();
}
