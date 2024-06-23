#pragma once

#include "scene.h"
#include "remotedevice.h"

#include <QDialog>
#include <QTextStream>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QAbstractButton>
#include <QLocalSocket>

#define MAJOR_REMOTE_VERSION 1
#define MINOR_REMOTE_VERSION 0

namespace Ui {
  class RemoteDeviceConfig;
}

class RemoteDeviceConfig : public QDialog {
  Q_OBJECT

public:
  explicit RemoteDeviceConfig(Scene *scene, QWidget *parent = nullptr, GraphicElement *elm = nullptr);
  ~RemoteDeviceConfig( );

  void start();
  void setupAuthScreen();
  void setupConfigScreen();
  void setupQueueScreen();
  void updateServiceInfo(QString str);
  bool savePortMapping();

  static QString version() {
      return QString("%1.%2").arg(QString::number(MAJOR_REMOTE_VERSION), QString::number(MINOR_REMOTE_VERSION));
  }

private slots:

  void connectionResponse(QNetworkReply* reply);

  // Actions
  void onAddPin();
  void onRemovePin();

  void onCopyToClipboard();

  void onTimeRefresh();
  void onQueueTimeRefresh();

  void onTryToConnect();
  void onEditPortMapping(int row, int column);
  void comboboxItemChanged(QString arg);

  void onApplyConfig(QAbstractButton* btn);
  void onRejectConfig();

  void on_disconnectBtn_clicked();

  void on_leaveBtn_clicked();

private:
  Ui::RemoteDeviceConfig *ui;
  RemoteDevice *elm;
  Scene *scene;
  QNetworkAccessManager *manager;
  RemoteLabOption currentOption;
  QTimer timer;
};
