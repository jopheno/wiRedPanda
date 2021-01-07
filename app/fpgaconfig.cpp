#include "fpgaconfig.h"
#include "ui_fpgaconfig.h"

#include <QClipboard>
#include <QSettings>
#include <QInputDialog>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QHostInfo>
#include <QPixmap>
#include <QMenu>
#include <QAction>

FpgaConfig::FpgaConfig( Editor *editor, QWidget *parent, GraphicElement *elm ) : QDialog( parent ), ui( new Ui::FpgaConfig ), editor( editor ), manager(new QNetworkAccessManager(this)) {
  ui->setupUi( this );

  setWindowTitle( "FPGA Configuration" );
  setWindowFlags( Qt::Window );
  setModal( true );
  QSettings settings( QSettings::IniFormat, QSettings::UserScope,
                      QApplication::organizationName( ), QApplication::applicationName( ) );

  ui->stackedWidget->setCurrentIndex(0);

  QPixmap unavailable( ":/remote/unavailable.png" );
  ui->logo->setPixmap(unavailable.scaled(ui->logo->width(), ui->logo->height(), Qt::KeepAspectRatio));

  manager->setTransferTimeout(2000);

  connect(manager, &QNetworkAccessManager::finished,
          this, &FpgaConfig::connectionResponse);

  if (Fpga* fpga = dynamic_cast<Fpga*>(elm)) {
    this->elm = fpga;
  }

  if (this->elm->getAuthToken() == "") {
    setupAuthScreen();
  } else {
    setupConfigScreen();
  }
}

FpgaConfig::~FpgaConfig() {
    delete manager;
    delete ui;
}

void FpgaConfig::setupAuthScreen() {
    for (auto option : this->elm->getOptions()) {
      ui->serviceSelector->addItem(QString::fromUtf8(option.getName().c_str()));
    }

    QString currentSelected = ui->serviceSelector->currentText();

    connect(ui->serviceSelector, SIGNAL(currentTextChanged(QString)), this, SLOT(comboboxItemChanged(QString)));
    connect(ui->connectButton, SIGNAL(clicked()), this, SLOT(onTryToConnect()));

    updateServiceInfo(currentSelected);
}

void FpgaConfig::onEditPortMapping(int row, int column) {
    QStringList items;

    if (column == 0) {
        const std::list<Pin>& availablePins = elm->getAvailablePins();
        std::list<Pin>::const_iterator it;
        for (it = availablePins.begin(); it != availablePins.end(); ++it) {
            items << QString::fromStdString(it->getName());
        }
    } else if (column == 1) {
        items << "INPUT";
        items << "OUTPUT";
    }

    QInputDialog input;
    input.setOptions(QInputDialog::UseListViewForComboBoxItems);
    input.setComboBoxItems(items);
    input.setWindowTitle("Choose action");

    if (input.exec())
    {
        QString inputValue = input.textValue();
        QTableWidgetItem* item = ui->tableWidget->item(row, column);

        if (!item) {
            item = new QTableWidgetItem();
            ui->tableWidget->setItem(row, column, item);
        }

        item->setText(inputValue);
    }
}

void FpgaConfig::onAddPin() {
    ui->tableWidget->insertRow(ui->tableWidget->currentRow()+1);
}

void FpgaConfig::onRemovePin() {
    ui->tableWidget->removeRow(ui->tableWidget->currentRow());
}

void FpgaConfig::onImportPinList() {
    QMessageBox messageBox;
    messageBox.information(0,"Info","Not implemented yet!");
    messageBox.setFixedSize(500,200);
}

void FpgaConfig::onExportPinList() {
    QMessageBox messageBox;
    messageBox.information(0,"Info","Not implemented yet!");
    messageBox.setFixedSize(500,200);
}

bool FpgaConfig::savePortMapping() {
    elm->resetPortMapping();

    int rowsAmount = ui->tableWidget->rowCount();

    for (int row = 0; row<rowsAmount; row++) {
        QTableWidgetItem* portWidget = ui->tableWidget->item(row, 0);

        if (!portWidget) {
            QMessageBox messageBox;
            messageBox.information(0,"Error","Found rows with empty port name");
            messageBox.setFixedSize(500,200);
            return false;
        }

        QString portName = portWidget->text();

        QTableWidgetItem* typeWidget = ui->tableWidget->item(row, 1);

        if (!typeWidget) {
            QMessageBox messageBox;
            messageBox.information(0,"Error","Found rows with invalid type");
            messageBox.setFixedSize(500,200);
            return false;
        }

        QString typeName = typeWidget->text();

        PIN_TYPE type = Pin::convertTypeString(typeName.toStdString());

        if (!elm->mapPin(portName.toStdString(), type)) {
            QMessageBox messageBox;
            messageBox.information(0,"Error","Unable to map pin " + portName + " as " + typeName);
            messageBox.setFixedSize(500,200);
            return false;
        }

    }

    elm->sendIOInfo();
    elm->setupPorts();

    return true;
}

void FpgaConfig::onTimeRefresh() {
    QTime time = QTime::currentTime();
    QString text = time.toString("hh:mm");
    if ((time.second() % 2) == 0)
        text[2] = ' ';
    ui->timeRemaining->display(text);

    ui->ping->setText("Ping: " + QString::number(elm->getLatency()) + "ms");
}

void FpgaConfig::setupConfigScreen() {
    ui->stackedWidget->setCurrentIndex(1);
    QString url = QString::fromStdString(this->elm->getCurrentOption().getUrl());

    COMMENT(this->elm->getAuthToken(), 0);
    COMMENT(url.toStdString(), 0);

    // Connect buttons
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &FpgaConfig::onApplyConfig);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &FpgaConfig::onRejectConfig);

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QAction* addAction = new QAction("Add", this);
    QAction* removeAction = new QAction("Remove", this);
    QAction* importAction = new QAction("Import", this);
    QAction* exportAction = new QAction("Export", this);

    connect(addAction, &QAction::triggered, this, &FpgaConfig::onAddPin);
    connect(removeAction, &QAction::triggered, this, &FpgaConfig::onRemovePin);
    connect(importAction, &QAction::triggered, this, &FpgaConfig::onImportPinList);
    connect(exportAction, &QAction::triggered, this, &FpgaConfig::onExportPinList);

    addAction->setIcon(QIcon(":/toolbar/zoomIn.png"));
    removeAction->setIcon(QIcon(":/toolbar/zoomOut.png"));
    importAction->setIcon(QIcon(":/toolbar/folder.png"));
    exportAction->setIcon(QIcon(":/toolbar/save.png"));

    QMenu* alignMenu = new QMenu;
    alignMenu->addAction(addAction);
    alignMenu->addAction(removeAction);
    alignMenu->addAction(importAction);
    alignMenu->addAction(exportAction);

    ui->pinOptions->setPopupMode(QToolButton::MenuButtonPopup);
    ui->pinOptions->setMenu(alignMenu);
    ui->pinOptions->setDefaultAction(addAction);

    connect(ui->tableWidget, &QTableWidget::cellDoubleClicked, this, &FpgaConfig::onEditPortMapping);

    ui->ping->setText("Ping: ...");
    elm->sendPing();

    onTimeRefresh();

    connect(&this->timer, &QTimer::timeout, this, &FpgaConfig::onTimeRefresh);
    this->timer.start(1000);

    ui->methodLabel->setText(QString::fromStdString(elm->getDeviceAuth()));

    // pins

    ui->tableWidget->clearContents();

    const std::list<Pin>& mappedPins = elm->getMappedPins();
    std::list<Pin>::const_iterator it;
    int row = 0;
    for (it = mappedPins.begin(); it != mappedPins.end(); ++it) {
        ui->tableWidget->insertRow(row);
        QTableWidgetItem* portWidget = ui->tableWidget->item(row, 0);

        if (!portWidget) {
            portWidget = new QTableWidgetItem();
            ui->tableWidget->setItem(row, 0, portWidget);
        }

        portWidget->setText(QString::fromStdString(it->getName()));

        QTableWidgetItem* typeWidget = ui->tableWidget->item(row, 1);

        if (!typeWidget) {
            typeWidget = new QTableWidgetItem();
            ui->tableWidget->setItem(row, 1, typeWidget);
        }

        QString type = "";
        switch(it->getType()) {
            case 1:
                type = "INPUT";
                break;
            case 2:
                type = "OUTPUT";
                break;
            default:
                type = "ERROR";
                break;
        }

        typeWidget->setText(type);

        row++;
    }

    // retrieve service image
    {
        QNetworkRequest request;
        request.setUrl(QUrl(url+"logo"));
        request.setRawHeader("User-Agent", "WiredPanda 1.0");

        QNetworkReply* reply = manager->get(request);
        reply->setProperty("type", "image");
        reply->setProperty("req", "setInfoLogo");
    }

    // retrieve method image
    {
        QNetworkRequest request;
        request.setUrl(QUrl(url+"method"));
        request.setRawHeader("User-Agent", "WiredPanda 1.0");
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        QByteArray postData;
        postData.append("token=" + QString::fromStdString(elm->getAuthToken()) + "&");
        postData.append("deviceId=" + QString::number(elm->getDeviceId()) + "");

        QNetworkReply* reply = manager->post(request, postData);
        reply->setProperty("type", "image");
        reply->setProperty("req", "setMethod");
    }

    if (elm->getDeviceMethod() == "VirtualHere") {
        #ifdef _WIN32
        // start virtualhere
        {
            QStringList args;
            QProcess* process = new QProcess();
            process->setWorkingDirectory(QDir::currentPath());

            args << "-cvhui.ini";
            process->start("vhui32.exe", args);
        }
        #endif
        #ifdef _WIN64
        // start virtualhere
        {
            QStringList args;
            QProcess* process = new QProcess();
            process->setWorkingDirectory(QDir::currentPath());

            args << "-cvhui.ini";
            process->start("vhui64.exe", args);
        }
        #endif
    }
}

void FpgaConfig::start() {
    exec();
}

void FpgaConfig::connectionResponse(QNetworkReply* reply) {
    QByteArray repliedData;
    repliedData = reply->readAll();

    if (reply->property("type") == "json") {
        QJsonDocument loadDoc(QJsonDocument::fromJson(repliedData));
        COMMENT( "JSON " + static_cast<QString>(repliedData).toStdString( ), 0 );

        QJsonObject json = loadDoc.object();
        QVariantMap json_map = json.toVariantMap();

        if (reply->property("req") == "updateService") {
            QPixmap online( ":/remote/online.png" );
            QPixmap offline( ":/remote/offline.png" );

            if (json["status"].toString() == "available") {
                ui->status->setPixmap(online.scaled(ui->status->width(), ui->status->height(), Qt::KeepAspectRatio));
            } else {
                ui->status->setPixmap(offline.scaled(ui->status->width(), ui->status->height(), Qt::KeepAspectRatio));
            }

            ui->statusLabel->setText(json["status"].toString() != "" ? json["status"].toString() : "offline");

            ui->deviceSelector->clear();
            int totalSlots = 0;

            QVariantMap devices = json_map["devices"].toMap();
            QStringList key_list = devices.keys();
            for(int i=0; i < key_list.count(); ++i) {
                QString key = key_list.at(i);
                int amount = devices[key].toInt();
                ui->deviceSelector->addItem(QString ("%1 [%2]").arg(key).arg(amount));
                totalSlots += amount;
            }

            ui->lcd->display(totalSlots);
        } else if (reply->property("req") == "connect") {
            if (json["reply"] != "ok") {
                QMessageBox messageBox;
                messageBox.critical(0,"Error",json["msg"].toString());
                messageBox.setFixedSize(500,200);
                return;
            }

            QString host = json["host"].toString();

            if (host == "0.0.0.0")
                host = reply->url().toString();

            QHostInfo info = QHostInfo::fromName(host);

            if (info.error() != QHostInfo::NoError ){
                COMMENT(info.errorString().toStdString(), 0);
                QMessageBox messageBox;
                messageBox.critical(0,"Error","Unable to resolve hostname !");
                messageBox.setFixedSize(500,200);
                this->close();
                return;
            }

            host = info.addresses().first().toString();

            if (!elm->connectTo(host.toStdString(), json["port"].toInt(), json["token"].toString().toStdString(), ui->deviceSelector->currentIndex()+1)) {
                QMessageBox messageBox;
                messageBox.critical(0,"Error","Connection failure !");
                messageBox.setFixedSize(500,200);
                this->close();
                return;
            }

            this->close();
        }
    }

    if(reply->property("type") == "image") {
        QString req = reply->property("req").toString();
        COMMENT( "IMAGE req=" + req.toStdString(), 0 );

        if (req == "setLogo") {
            QPixmap pic;
            pic.loadFromData(repliedData);

            if (!pic.isNull()) {
                ui->logo->setPixmap(pic.scaled(ui->logo->width(), ui->logo->height(), Qt::KeepAspectRatio));
            } else {
                QPixmap unavailable( ":/remote/unavailable.png" );
                ui->logo->setPixmap(unavailable.scaled(ui->logo->width(), ui->logo->height(), Qt::KeepAspectRatio));
            }
        } else if (req == "setInfoLogo") {
            QPixmap pic;
            pic.loadFromData(repliedData);

            if (!pic.isNull()) {
                ui->infoLogo->setPixmap(pic.scaled(ui->infoLogo->width(), ui->infoLogo->height(), Qt::KeepAspectRatio));
            } else {
                QPixmap unavailable( ":/remote/unavailable.png" );
                ui->infoLogo->setPixmap(unavailable.scaled(ui->infoLogo->width(), ui->infoLogo->height(), Qt::KeepAspectRatio));
            }
        } else if (req == "setMethod") {
            QPixmap pic;
            pic.loadFromData(repliedData);

            if (!pic.isNull()) {
                ui->methodImg->setPixmap(pic.scaled(ui->methodImg->width(), ui->methodImg->height(), Qt::KeepAspectRatio));
            } else {
                QPixmap unavailable( ":/remote/unavailable.png" );
                ui->methodImg->setPixmap(unavailable.scaled(ui->methodImg->width(), ui->methodImg->height(), Qt::KeepAspectRatio));
            }
        }
    }
}

void FpgaConfig::updateServiceInfo(QString str) {
    auto& options = this->elm->getOptions();
    auto it = options.begin();

    while(it != options.end()) {
        if (it->getName() != str.toStdString())
            it++;
        else
            break;
    }

    if (it != options.end()) {
        currentOption = (*it);
        QString url = QString::fromStdString(it->getUrl());

        // retrieve service information
        {
            QNetworkRequest request;
            request.setUrl(QUrl(url));
            request.setRawHeader("User-Agent", "WiredPanda 1.0");

            QNetworkReply* reply = manager->get(request);
            reply->setProperty("type", "json");
            reply->setProperty("req", "updateService");
        }

        // retrieve service image
        {
            QNetworkRequest request;
            request.setUrl(QUrl(url+"logo"));
            request.setRawHeader("User-Agent", "WiredPanda 1.0");

            QNetworkReply* reply = manager->get(request);
            reply->setProperty("type", "image");
            reply->setProperty("req", "setLogo");
        }

        // change to loading
        QPixmap unavailable( ":/remote/unavailable.png" );
        QPixmap searching( ":/remote/searching.png" );

        ui->logo->setPixmap(unavailable.scaled(ui->logo->width(), ui->logo->height(), Qt::KeepAspectRatio));
        ui->status->setPixmap(searching.scaled(ui->status->width(), ui->status->height(), Qt::KeepAspectRatio));
        ui->deviceSelector->clear();
        ui->statusLabel->setText("searching...");
        ui->lcd->display(0);
    } else {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","There are not available services");
        messageBox.setFixedSize(500,200);
        this->close();
    }
}

void FpgaConfig::onTryToConnect() {
    QNetworkRequest request;
    request.setUrl(QUrl(QString::fromStdString(currentOption.getUrl())+"login"));
    request.setRawHeader("User-Agent", "WiredPanda 1.0");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QString encodedPasswd = ui->passwdInput->text();
    AUTH_METHOD currentAuthMethod = currentOption.getAuthMethod();
    if (currentAuthMethod != AUTH_METHOD::PLAIN) {
        QCryptographicHash::Algorithm algorithm;
        switch(currentAuthMethod) {
            case AUTH_METHOD::MD5:
                algorithm = QCryptographicHash::Algorithm::Md5;
                break;
            case AUTH_METHOD::SHA1:
                algorithm = QCryptographicHash::Algorithm::Sha1;
                break;
            case AUTH_METHOD::SHA256:
                algorithm = QCryptographicHash::Algorithm::Sha256;
                break;
            default:
                algorithm = QCryptographicHash::Algorithm::Sha256;
                std::cerr << "ERROR: Unable to evaluate the correct encoding method, sending as SHA256." << std::endl;
                break;
        }

        QCryptographicHash hashAlgorithm(algorithm);
        hashAlgorithm.addData(encodedPasswd.toStdString().c_str(), encodedPasswd.toStdString().length());
        encodedPasswd = hashAlgorithm.result().toHex();
    }

    elm->setCurrentOption(currentOption);

    QByteArray postData;
    postData.append("login=" + ui->loginInput->text() + "&");
    postData.append("passwd=" + encodedPasswd + "");

    QNetworkReply* reply = manager->post(request, postData);
    reply->setProperty("type", "json");
    reply->setProperty("req", "connect");
}

void FpgaConfig::comboboxItemChanged(QString currentSelected) {
    updateServiceInfo(currentSelected);
}

void FpgaConfig::onApplyConfig(QAbstractButton* btn)
{
    if (btn->text() == "Apply") {
        COMMENT("APPLIED!", 0);
        if (savePortMapping()) {
            this->close();
        }
    }
}

void FpgaConfig::onRejectConfig()
{
    COMMENT("REJECTED!", 0);
}
