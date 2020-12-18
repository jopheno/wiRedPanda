#include "protocol.h"

#include <iostream>

#include <QMessageBox>

std::map<uint8_t, parse_function> RemoteProtocol::parseMapping;
bool RemoteProtocol::initialized;

#define REGISTER_PARSE_OPCODE(opcode, parse_func_id) parseMapping.insert(it, std::pair<Opcodes, parse_function>(Opcodes::opcode, parse_func_id))

void RemoteProtocol::init() {
    auto it = parseMapping.begin();

    REGISTER_PARSE_OPCODE(OPCODE_START_SESSION, parse_session_start);
    REGISTER_PARSE_OPCODE(OPCODE_PONG, parse_pong);
}

void RemoteProtocol::parse_session_start(Fpga* fpga, NetworkIncomingMessage& imsg) {
    std::cerr << "siz: " << imsg.getSize() << std::endl;
    QString token = imsg.popString();
    uint16_t device_id = imsg.pop<uint16_t>();
    QString device_auth = imsg.popString();
    uint8_t pin_amount = imsg.pop<uint8_t>();

    // error occured
    if (pin_amount == 0) {
        QString errorMsg = imsg.popString();
        QMessageBox messageBox;
        messageBox.critical(0,"Error",errorMsg);
        messageBox.setFixedSize(500,200);
        return;
    }

    for (int i = 0; i<pin_amount; i++) {
        uint8_t id = imsg.pop<uint8_t>();
        QString pinName = imsg.popString();
        uint8_t type = imsg.pop<uint8_t>();

        std::cerr << "> pin " << i << ": " << static_cast<int>(id) << ", " << pinName.toStdString() << ", " << static_cast<int>(type) << std::endl;

        fpga->addPin(id, pinName.toStdString(), type);
    }

    COMMENT(token.toStdString(), 0);

    fpga->setDeviceId(device_id);
    fpga->setDeviceAuth(device_auth.toStdString());
    fpga->setAuthToken(token.toStdString());

    QMessageBox messageBox;
    messageBox.information(0,"Info","Connection estabilished!");
    messageBox.setFixedSize(500,200);
}

void RemoteProtocol::parse_pong(Fpga* fpga, NetworkIncomingMessage& imsg) {
    COMMENT("PONG", 0);
    uint64_t timestamp = imsg.pop<uint64_t>();

    uint64_t current = QDateTime::currentMSecsSinceEpoch();
    fpga->setLatency(current-timestamp);
}

void RemoteProtocol::parse(Fpga* fpga, uint8_t opcode, QByteArray byteArray) {
    if (!initialized)
        init();

    NetworkIncomingMessage imsg(opcode, byteArray);

    auto it = parseMapping.find(opcode);

    if (it != parseMapping.end()) {
        it->second(fpga, imsg);

        // all bytes should be read
        assert(imsg.getRemainingBytes() <= 0);
    }
}

NetworkOutgoingMessage RemoteProtocol::sendPing() {
    NetworkOutgoingMessage msg(2);

    uint64_t timestamp = QDateTime::currentMSecsSinceEpoch();
    msg.addByte<uint64_t>(timestamp);
    msg.addSize();

    return msg;
}

void RemoteProtocol::sendUpdatePorts() {

}
