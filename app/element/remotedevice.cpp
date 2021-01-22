#include "remotedevice.h"
#include "protocol.h"

#include <QMessageBox>

AUTH_METHOD toAuthMethod(std::string auth_type) {
  static const std::map<std::string, AUTH_METHOD> optionStrings {
    { "plain", AUTH_METHOD::PLAIN },
    { "md5", AUTH_METHOD::MD5 },
    { "sha1", AUTH_METHOD::SHA1 },
    { "sha256", AUTH_METHOD::SHA256 },

    { "sha-1", AUTH_METHOD::SHA1 },
    { "sha-256", AUTH_METHOD::SHA256 }
  };

  auto itr = optionStrings.find(QString::fromStdString(auth_type).toLower().toStdString());
  if( itr != optionStrings.end() ) {
      return (*itr).second;
  }
  return AUTH_METHOD::NONE;
}

PIN_TYPE Pin::convertTypeString(const std::string& typeName) {
    PIN_TYPE t = PIN_NONE;

    if (typeName.compare("INPUT") == 0) {
        t = PIN_INPUT;
    } else if (typeName.compare("OUTPUT") == 0) {
        t = PIN_OUTPUT;
    } else if (typeName.compare("GENERAL_PURPOSE") == 0) {
        t = PIN_GENERAL_PURPOSE;
    }

    return t;
}

std::list<RemoteLabOption> RemoteDevice::options;

RemoteDevice::RemoteDevice( QGraphicsItem *parent ) : GraphicElement( 0, 55, 0, 55, parent ) {
  pixmapSkinName.append( ":/remote/fpgaBox.png" );
  setPixmap( pixmapSkinName[ 0 ] );
  setRotatable( false );
  setupPorts( );
  updatePorts( );
  setPortName( "RemoteDevice" );
  setHasCustomConfig(true);
  lastValue = false;
  lastClk = false;
  deviceId = 0;
  authToken = "";
  deviceAuth = {"", ""};

  // Try loading remote lab settings
  QDomDocument xml;
  // Load xml file as raw data

  std::cerr << QDir::currentPath().toStdString() << std::endl;

  QFile f("remotelab.xml");
  if (!f.open(QIODevice::ReadOnly ))
  {
      // Error while loading file
      std::cerr << "Error while loading remotelab.xml file" << std::endl;
      disable();
      return;
  }
  // Set data into the QDomDocument before processing
  xml.setContent(&f);
  f.close();

  if (!loadSettings(xml)) {
    disable();
    return;
  }

  socket.setSocketOption(QAbstractSocket::KeepAliveOption, 1);
  // No need for checking the amount of bytes written
  //connect(&socket, &QTcpSocket::bytesWritten, this, &RemoteDevice::handle);
  connect(&socket, &QTcpSocket::disconnected, this, &RemoteDevice::close);
  connect(&socket, &QTcpSocket::readyRead, this, &RemoteDevice::readIsDone);

  connect(&this->timer, &QTimer::timeout, this, &RemoteDevice::onTimeRefresh);
  this->timer.start(1000);
}

RemoteDevice::~RemoteDevice() {
    timer.stop();

    if(socket.isOpen())
        socket.disconnectFromHost();
}

void RemoteDevice::onTimeRefresh() {
    if(socket.isOpen()) {
        sendPing();

        if (!isAlive()) {
            disconnect();

            QMessageBox messageBox;
            messageBox.critical(0,"Disconnected","Sorry, we were unable to keep your connection up.\nPlease consider reconnecting, or checking if you are still connected to the internet.");
            messageBox.setFixedSize(500,200);
        }
    }
}

bool RemoteDevice::connectTo(const std::string& host, int port, const std::string& token, uint8_t deviceTypeId, uint8_t methodId) {
    COMMENT( "Connecting to " + host, 0 );
    setAliveSince(QDateTime::currentSecsSinceEpoch());
    socket.connectToHost(QString::fromStdString(host), port);
    if (socket.waitForConnected(5000)) {
        NetworkOutgoingMessage msg(1);
        msg.addByte<uint8_t>(deviceTypeId);
        msg.addByte<uint8_t>(methodId);
        msg.addString(QString::fromStdString(token));
        msg.addSize();

        socket.write(msg);
        socket.waitForReadyRead(5000);
        //qDebug() << socket.readLine();

        return true;
    }

    return false;
}

void RemoteDevice::sendPing() {
    NetworkOutgoingMessage msg = RemoteProtocol::sendPing();
    socket.write(msg);
    socket.waitForReadyRead(1000);
}

void RemoteDevice::sendIOInfo() {
    NetworkOutgoingMessage msg = RemoteProtocol::sendIOInfo(latency, getMappedPins());
    socket.write(msg);
    socket.waitForReadyRead(1000);
}

void RemoteDevice::sendUpdateInput(uint32_t id, uint8_t value) {
    NetworkOutgoingMessage msg = RemoteProtocol::sendUpdateInput(id, value);
    socket.write(msg);
}

void RemoteDevice::readIsDone()
{
    while(socket.bytesAvailable() > 0) {
        QByteArray headerBytes = socket.read(4);

        QDataStream hds(headerBytes);
        uint32_t size;
        hds >> size;

        QByteArray opcodeBytes = socket.read(1);
        QDataStream op_ds(opcodeBytes);

        uint8_t opcode;
        op_ds >> opcode;

        QByteArray bytes = socket.read(size-1);
        QDataStream ds(bytes);

        RemoteProtocol::parse(this, opcode, bytes);
    }
}

void RemoteDevice::close()
{
    COMMENT( "Closing connection!", 0 );
}

void RemoteDevice::setupPorts( ) {
    int inputAmount = 0;
    int outputAmount = 0;

    const std::list<Pin>& mappedPins = getMappedPins();

    if (mappedPins.size() <= 0) {
        setInputSize(0);
        setOutputSize(0);
        return;
    }


    for (const Pin& p : mappedPins) {
      if (p.getType() == PIN_TYPE::PIN_INPUT)
          inputAmount++;
      if (p.getType() == PIN_TYPE::PIN_OUTPUT)
          outputAmount++;
    }

    setInputSize(inputAmount);
    setOutputSize(outputAmount);

    inputAmount = 0;
    outputAmount = 0;
    for (const Pin& p : mappedPins) {
      if (p.getType() == PIN_TYPE::PIN_INPUT) {
        input(inputAmount)->setName(QString::fromStdString(p.getName()));
        input(inputAmount)->setRemoteId(p.getId());
        inputAmount++;
      }
      if (p.getType() == PIN_TYPE::PIN_OUTPUT) {
        output(outputAmount)->setName(QString::fromStdString(p.getName()));
        output(outputAmount)->setRemoteId(p.getId());
        outputAmount++;
      }
    }
}

void RemoteDevice::updatePorts( ) {
  GraphicElement::updatePorts();
}

void RemoteDevice::setSkin( bool defaultSkin, QString filename ) {
  if( defaultSkin )
    pixmapSkinName[ 0 ] = ":/remote/RemoteDeviceBox.png";
  else
    pixmapSkinName[ 0 ] = filename;
  setPixmap( pixmapSkinName[ 0 ] );
}

bool RemoteDevice::loadSettings(const QDomDocument& xml) {
  // Extract the root markup
  QDomElement root=xml.documentElement();

  if (root.tagName() != "endpoints") {
    std::cerr << "Malformed remotelab.xml file" << std::endl;
    return false;
  }

  options.clear();

  QDomElement optionElm=root.firstChild().toElement();

  // Loop while there is a child
  while(!optionElm.isNull())
  { 
    RemoteLabOption option;

    // Check if the child tag name is option
    if (optionElm.tagName()=="option")
    {

        // Read and display the component name
        option.name=optionElm.attribute("name","Unknown").toStdString();

        // Get the first child of the component
        QDomElement component=optionElm.firstChild().toElement();

        // Read each child of the component node
        while (!component.isNull())
        {
            // Read Name and value
            if (component.tagName()=="url") option.url=component.firstChild().toText().data().toStdString();
            if (component.tagName()=="auth") option.authMethod=toAuthMethod(component.firstChild().toText().data().toStdString());

            // Next child component
            component = component.nextSibling().toElement();
        }
    }

    options.push_back(option);
    COMMENT( "Creating option " + option.getName() + " with url: " + option.getUrl(), 0 );

    // Next component
    optionElm = optionElm.nextSibling().toElement();
  }

  return true;
}

void RemoteDevice::loadAvailablePin( QDataStream &ds ) {
  quint32 portId;
  ds >> portId;
  QString portName;
  ds >> portName;
  quint8 portType;
  ds >> portType;

  addPin(portId, portName.toStdString(), portType);
}

void RemoteDevice::loadMappedPin( QDataStream &ds ) {
  QString portName;
  ds >> portName;
  quint8 portType;
  ds >> portType;

  mapPin(portName.toStdString(), portType);
}

void RemoteDevice::loadRemoteIO( QDataStream &ds, double version ) {
  if( version >= 2.7 ) {
    COMMENT( "Loading remote IO.", 4 );

    resetPortMapping();

    quint32 availablePinsAmount;
    ds >> availablePinsAmount;
    for( size_t index = 0; index < availablePinsAmount; ++index ) {
      loadAvailablePin( ds );
    }

    quint32 mappedPinsAmount;
    ds >> mappedPinsAmount;
    for( size_t index = 0; index < mappedPinsAmount; ++index ) {
      loadMappedPin( ds );
    }
    refresh( );
  }
}

void RemoteDevice::load( QDataStream &ds, QMap< quint64, QNEPort* > &portMap, double version ) {
    GraphicElement::load(ds, portMap, version);

    loadRemoteIO(ds, version);
}

void RemoteDevice::save( QDataStream &ds ) const {
    GraphicElement::save( ds );

    /* <\Version2.7> */
    COMMENT( "Saving remote IO.", 4 );
    ds << static_cast< quint32 >( availablePins.size( ) );
    for( const Pin& pin: availablePins ) {
      ds << static_cast<quint32>(pin.getId());
      ds << QString::fromStdString(pin.getName());
      ds << static_cast<quint8>(pin.getType());
    }

    ds << static_cast< quint32 >( mappedPins.size( ) );
    for( const Pin& pin: mappedPins ) {
      ds << QString::fromStdString(pin.getName());
      ds << static_cast<quint8>(pin.getType());
    }
}