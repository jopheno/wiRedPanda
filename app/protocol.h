#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QDataStream>

#include <map>

#include "fpga.h"
#include "network.h"

typedef void (*parse_function)(Fpga*, NetworkIncomingMessage&);

enum Opcodes : uint8_t {
    OPCODE_NONE = 0,
    OPCODE_START_SESSION,
    OPCODE_PONG,
};

class RemoteProtocol {
public:
    static void init();
    static void parse(Fpga* fpga, uint8_t opcode, QByteArray ds);

    static void parse_session_start(Fpga* fpga, NetworkIncomingMessage& imsg);
    static void parse_pong(Fpga* fpga, NetworkIncomingMessage& imsg);

    static NetworkOutgoingMessage sendPing();
    static void sendUpdatePorts();
private:
    static std::map<uint8_t, parse_function> parseMapping;
    static bool initialized;
};


#endif // PROTOCOL_H
