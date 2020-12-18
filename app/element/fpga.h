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
  std::string logo;
  AUTH_METHOD authMethod;

  RemoteLabOption() = default;
  RemoteLabOption(std::string name, std::string url, std::string logo, AUTH_METHOD auth_method) : name(name), url(url), logo(logo), authMethod(auth_method) {}

  std::string getName() const { return name; }
  std::string getUrl() const {
      QString url = QString::fromStdString(this->url);

      if (!url.endsWith("/"))
          url.append("/");

      return url.toStdString();
  }
  std::string getLogo() const { return logo; }
  AUTH_METHOD getAuthMethod() const { return authMethod; }
};

enum PIN_TYPE : uint8_t {
  PIN_NONE = 0,
  PIN_INPUT = 1,
  PIN_OUTPUT = 2,
  PIN_GENERAL_PURPOSE = 3,
};

struct Pin {
  uint8_t id;
  std::string name;
  PIN_TYPE type;

  Pin() = delete;
  Pin(uint8_t id, const std::string& name, PIN_TYPE type) : id(id), name(name), type(type) {}

  uint8_t getId() const { return id; }
  const std::string& getName() const { return name; }
  PIN_TYPE getType() const { return type; }

  static PIN_TYPE convertTypeString(const std::string& typeName);
};

class Fpga : public GraphicElement {
  bool lastClk;
  bool lastValue;
  uint16_t deviceId;
  uint32_t latency;
  std::string authToken;
  std::string deviceAuth;

  std::list<RemoteLabOption> options;
  std::list<Pin> availablePins;
  std::list<Pin> mappedPins;

  RemoteLabOption currentOption;
  QTcpSocket socket;

public:
  explicit Fpga( QGraphicsItem *parent = nullptr );
  virtual ~Fpga( ) override;

  bool connectTo(const std::string& host, int port, const std::string& token, uint8_t deviceTypeId);
  void sendPing();

  const std::string& getAuthToken() const { return authToken; }
  void setAuthToken(const std::string& token) {
      authToken = token;
  }

  const std::string& getDeviceAuth() const { return deviceAuth; }
  void setDeviceAuth(const std::string& token) {
      deviceAuth = token;
  }

  uint16_t getDeviceId() const { return deviceId; }
  void setDeviceId(uint16_t id) {
      deviceId = id;
  }

  uint32_t getLatency() const { return latency; }
  void setLatency(uint32_t milliseconds) {
      latency = milliseconds;
  }

  const RemoteLabOption getCurrentOption() const { return currentOption; }
  void setCurrentOption(const RemoteLabOption option) {
      currentOption = option;
  }

  const std::list<Pin>& getAvailablePins() const { return availablePins; }
  void addPin(uint8_t id, const std::string& port, uint8_t pinType) {
      availablePins.push_back(Pin(id, port, static_cast<PIN_TYPE>(pinType)));
  }

  void resetPortMapping() { mappedPins.clear(); }
  const std::list<Pin>& getMappedPins() const { return mappedPins; }
  bool mapPin(const std::string& name, uint8_t pinType) {
      const std::list<Pin>& list = getAvailablePins();
      std::list<Pin>::const_iterator it;

      for (it = list.begin(); it != list.end() ; ++it) {
          const Pin& p = (*it);

          if (p.getName().compare(name) == 0) {
              if (static_cast<int>(p.getType()) == PIN_GENERAL_PURPOSE || static_cast<int>(p.getType()) == static_cast<int>(pinType)) {
                  mappedPins.push_back(Pin(p.id, name, static_cast<PIN_TYPE>(pinType)));
                  return true;
              } else {
                  return false;
              }
          }
      }

      return false;
  }

private slots:
    void handle(qint64 bytes);
    void readIsDone();
    void close();

  // GraphicElement interface
public:
  virtual ElementType elementType( ) override {
    return( ElementType::FPGA );
  }
  virtual ElementGroup elementGroup( ) override {
    return( ElementGroup::REMOTE );
  }
  virtual void updatePorts( ) override;
  void setSkin( bool defaultSkin, QString filename ) override;
  bool loadSettings( const QDomDocument& xml );
  const std::list<RemoteLabOption>& getOptions() { return options; }
};

#endif // FPGA_H
