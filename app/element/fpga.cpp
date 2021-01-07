#include "fpga.h"
#include "protocol.h"

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

Fpga::Fpga( QGraphicsItem *parent ) : GraphicElement( 0, 55, 0, 55, parent ) {
  pixmapSkinName.append( ":/remote/fpgaBox.png" );
  setPixmap( pixmapSkinName[ 0 ] );
  setRotatable( false );
  setupPorts( );
  updatePorts( );
  setPortName( "FPGA" );
  setHasCustomConfig(true);
  lastValue = false;
  lastClk = false;
  deviceId = 0;
  authToken = "";
  deviceAuth = "";

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
  connect(&socket, &QTcpSocket::bytesWritten, this, &Fpga::handle);
  connect(&socket, &QTcpSocket::disconnected, this, &Fpga::close);
  connect(&socket, &QTcpSocket::readyRead, this, &Fpga::readIsDone);
}

Fpga::~Fpga() {
    if(socket.isOpen())
        socket.disconnectFromHost();
}

bool Fpga::connectTo(const std::string& host, int port, const std::string& token, uint8_t deviceTypeId) {
    COMMENT( "Connecting to " + host, 0 );
    qDebug() << deviceTypeId;
    socket.connectToHost(QString::fromStdString(host), port);
    if (socket.waitForConnected(5000)) {
        NetworkOutgoingMessage msg(1);
        msg.addByte<uint8_t>(deviceTypeId);
        msg.addString(QString::fromStdString(token));
        msg.addSize();

        socket.write(msg);
        socket.waitForReadyRead(5000);
        //qDebug() << socket.readLine();

        return true;
    }

    return false;
}

void Fpga::sendPing() {
    NetworkOutgoingMessage msg = RemoteProtocol::sendPing();
    socket.write(msg);
    socket.waitForReadyRead(1000);
}

void Fpga::sendIOInfo() {
    NetworkOutgoingMessage msg = RemoteProtocol::sendIOInfo(latency, getMappedPins());
    socket.write(msg);
    socket.waitForReadyRead(1000);
}

void Fpga::sendUpdateInput(uint32_t id, uint8_t value) {
    NetworkOutgoingMessage msg = RemoteProtocol::sendUpdateInput(id, value);
    socket.write(msg);
}

void Fpga::readIsDone()
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
void Fpga::handle(qint64 bytesAmount)
{
    std::cerr << "bytesAmount: " << bytesAmount << std::endl;
}
void Fpga::close()
{
    COMMENT( "Closing connection!", 0 );
}

void Fpga::setupPorts( ) {
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

void Fpga::updatePorts( ) {
  GraphicElement::updatePorts();
}

void Fpga::setSkin( bool defaultSkin, QString filename ) {
  if( defaultSkin )
    pixmapSkinName[ 0 ] = ":/remote/fpgaBox.png";
  else
    pixmapSkinName[ 0 ] = filename;
  setPixmap( pixmapSkinName[ 0 ] );
}

bool Fpga::loadSettings(const QDomDocument& xml) {
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
