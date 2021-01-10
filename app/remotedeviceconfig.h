#ifndef FPGACONFIG_H
#define FPGACONFIG_H

#include "editor.h"
#include "remotedevice.h"

#include <QDialog>
#include <QTextStream>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QAbstractButton>
#include <QLocalSocket>

namespace Ui {
  class RemoteDeviceConfig;
}

class RemoteDeviceConfig : public QDialog {
  Q_OBJECT

public:
  explicit RemoteDeviceConfig( Editor *editor, QWidget *parent = nullptr, GraphicElement *elm = nullptr );
  ~RemoteDeviceConfig( );

  void start();
  void setupAuthScreen();
  void setupConfigScreen();
  void updateServiceInfo(QString str);
  bool savePortMapping();

private slots:

  void connectionResponse(QNetworkReply* reply);

  // Actions
  void onAddPin();
  void onRemovePin();

  void onCopyToClipboard();

  void onTimeRefresh();

  void onTryToConnect();
  void onEditPortMapping(int row, int column);
  void comboboxItemChanged(QString arg);

  void onApplyConfig(QAbstractButton* btn);
  void onRejectConfig();

  void on_disconnectBtn_clicked();

private:
  Ui::RemoteDeviceConfig *ui;
  RemoteDevice *elm;
  Editor *editor;
  QNetworkAccessManager *manager;
  RemoteLabOption currentOption;
  QTimer timer;
};

#endif /* FPGACONFIG_H */
