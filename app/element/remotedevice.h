#ifndef FPGA_H
#define FPGA_H

#include <QtXml>
#include <QFile>
#include <QTcpSocket>

#include <list>
#include <map>
#include "graphicelement.h"

enum AUTH_METHOD {
  NONE = 0,
  PLAIN = 1,
  MD5,
  SHA1,
  SHA256
};

AUTH_METHOD toAuthMethod(QString auth_type);

struct RemoteLabOption {
  std::string name;
  std::string url;
  AUTH_METHOD authMethod;

  RemoteLabOption() = default;
  RemoteLabOption(std::string name, std::string url, AUTH_METHOD auth_method) : name(name), url(url), authMethod(auth_method) {}

  std::string getName() const { return name; }
  std::string getUrl() const {
      QString url = QString::fromStdString(this->url);

      if (!url.endsWith("/"))
          url.append("/");

      return url.toStdString();
  }
  AUTH_METHOD getAuthMethod() const { return authMethod; }
};

enum PIN_TYPE : uint8_t {
  PIN_NONE = 0,
  PIN_INPUT = 1,
  PIN_OUTPUT = 2,
  PIN_GENERAL_PURPOSE = 3,
};

struct Pin {
  uint32_t id;
  std::string name;
  PIN_TYPE type;

  Pin() = delete;
  Pin(uint32_t id, const std::string& name, PIN_TYPE type) : id(id), name(name), type(type) {}

  uint32_t getId() const { return id; }
  const std::string& getName() const { return name; }
  PIN_TYPE getType() const { return type; }

  static PIN_TYPE convertTypeString(const std::string& typeName);
};

struct DeviceAuth {
    std::string name;
    std::string token;
};

class RemoteDevice : public GraphicElement {
  bool lastClk;
  bool lastValue;
  uint16_t deviceId;
  uint16_t latency;
  std::string authToken;
  std::string deviceMethod;
  DeviceAuth deviceAuth;

  static std::list<RemoteLabOption> options;
  std::list<Pin> availablePins;
  std::list<Pin> mappedPins;

  std::map<uint32_t, bool> mappedInputs;
  std::map<uint32_t, bool> mappedOutputs;

  RemoteLabOption currentOption;
  QTcpSocket socket;
  QTimer timer;

public:
  explicit RemoteDevice( QGraphicsItem *parent = nullptr );
  virtual ~RemoteDevice( ) override;

  void setupPorts( );

  bool connectTo(const std::string& host, int port, const std::string& token, uint8_t deviceTypeId, uint8_t methodId);
  void sendPing();
  void sendIOInfo();
  void sendUpdateInput(uint32_t id, uint8_t value);

  void onTimeRefresh();

  void disconnect() {
      if (socket.isOpen()) {
          socket.disconnectFromHost();
          socket.waitForDisconnected(1000);
      }

      // will no longer trigger the setup window
      setAuthToken("");

      // reset available pins, the mapped one shall still be configured (in case of incompatibility, they will not be applied)
      availablePins.clear();
  }

  const std::string& getAuthToken() const { return authToken; }
  void setAuthToken(const std::string& token) {
      authToken = token;
  }

  const std::string& getDeviceMethod() const { return deviceMethod; }
  void setDeviceMethod(const std::string& method) {
      deviceMethod = method;
  }

  const DeviceAuth& getDeviceAuth() const { return deviceAuth; }
  void setDeviceAuth(const std::string& name, const std::string& token) {
      deviceAuth = {name, token};
  }

  uint16_t getDeviceId() const { return deviceId; }
  void setDeviceId(uint16_t id) {
      deviceId = id;
  }

  uint16_t getLatency() const { return latency; }
  void setLatency(uint16_t milliseconds) {
      latency = milliseconds;
  }

  const RemoteLabOption getCurrentOption() const { return currentOption; }
  void setCurrentOption(const RemoteLabOption option) {
      currentOption = option;
  }

  const std::list<Pin>& getAvailablePins() const { return availablePins; }
  void addPin(uint32_t id, const std::string& port, uint8_t pinType) {
      availablePins.push_back(Pin(id, port, static_cast<PIN_TYPE>(pinType)));
  }

  void resetPortMapping() { mappedPins.clear(); mappedOutputs.clear(); }
  const std::list<Pin>& getMappedPins() const { return mappedPins; }

  uint32_t getPinId(const std::string& name, uint8_t pinType) {
      const std::list<Pin>& list = getAvailablePins();
      std::list<Pin>::const_iterator it;

      for (it = list.begin(); it != list.end() ; ++it) {
          const Pin& p = (*it);

          if (p.getName().compare(name) == 0) {
              if (static_cast<int>(p.getType()) == PIN_GENERAL_PURPOSE || static_cast<int>(p.getType()) == static_cast<int>(pinType)) {

                  std::list<Pin>::const_iterator mapped_it = getMappedPins().begin();

                  while (mapped_it != getMappedPins().end()) {
                      if (mapped_it->getName().compare(name) == 0)
                          return 0;

                      ++mapped_it;
                  }

                  return p.id;
              } else {
                  return 0;
              }
          }
      }

      return 0;
  }

  bool canBeMapped(const std::string& name, uint8_t pinType) {
      return (getPinId(name, pinType) != 0);
  }

  bool mapPin(const std::string& name, uint8_t pinType) {
      uint32_t id = getPinId(name, pinType);

      if (id != 0) {
          std::cerr << "Pin(" << id << ", " << name << ", " << static_cast<PIN_TYPE>(pinType) << ")" << std::endl;
          mappedPins.push_back(Pin(id, name, static_cast<PIN_TYPE>(pinType)));
          if (pinType == PIN_TYPE::PIN_INPUT) { mappedInputs[id] = false; }
          if (pinType == PIN_TYPE::PIN_OUTPUT) { mappedOutputs[id] = false; }
          return true;
      }

      return false;
  }

  const std::map<uint32_t, bool>& getInputs() { return mappedInputs; }
  void setInput(uint32_t id, bool value) {
      if (mappedInputs[id] != value) {
          mappedInputs[id] = value;
          sendUpdateInput(id, value == true ? 1 : 0);
      }
  }

  const std::map<uint32_t, bool>& getOutputs() { return mappedOutputs; }
  void setOutput(uint32_t id, bool value) { mappedOutputs[id] = value; }

  void loadAvailablePin( QDataStream &ds );
  void loadMappedPin( QDataStream &ds );
  void loadRemoteIO( QDataStream &ds, double version );

private slots:
    void readIsDone();
    void close();

  // GraphicElement interface
public:
  virtual ElementType elementType( ) override {
    return( ElementType::REMOTE );
  }
  virtual ElementGroup elementGroup( ) override {
    return( ElementGroup::REMOTE );
  }
  virtual void updatePorts( ) override;
  void setSkin( bool defaultSkin, QString filename ) override;
  static bool loadSettings( const QDomDocument& xml );
  const std::list<RemoteLabOption>& getOptions() { return options; }

  void load( QDataStream &ds, QMap< quint64, QNEPort* > &portMap, double version ) override;
  void save( QDataStream &ds ) const override;
};

#endif // FPGA_H
