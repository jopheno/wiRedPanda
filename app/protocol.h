#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QDataStream>

#include <map>

#include "remotedevice.h"
#include "network.h"

typedef void (*parse_function)(RemoteDevice*, NetworkIncomingMessage&);

enum Opcodes : uint8_t {
    OPCODE_NONE = 0,
    OPCODE_START_SESSION,
    OPCODE_PONG,
    OPCODE_UPDATE_OUTPUT
};

class RemoteProtocol {
public:
    static void init();
    static void parse(RemoteDevice* remoteDevice, uint8_t opcode, QByteArray ds);

    static void parse_session_start(RemoteDevice* remoteDevice, NetworkIncomingMessage& imsg);
    static void parse_pong(RemoteDevice* remoteDevice, NetworkIncomingMessage& imsg);
    static void parse_output(RemoteDevice* remoteDevice, NetworkIncomingMessage& imsg);

    static NetworkOutgoingMessage sendPing();
    static NetworkOutgoingMessage sendIOInfo(uint16_t latency, const std::list<Pin>& mappedPins);
    static NetworkOutgoingMessage sendUpdateInput(uint32_t id, uint8_t value);
private:
    static std::map<uint8_t, parse_function> parseMapping;
    static bool initialized;
};


#endif // PROTOCOL_H
