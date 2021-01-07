#ifndef FPGACONFIG_H
#define FPGACONFIG_H

#include "editor.h"
#include "fpga.h"

#include <QDialog>
#include <QTextStream>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QAbstractButton>
#include <QLocalSocket>

namespace Ui {
  class FpgaConfig;
}

class FpgaConfig : public QDialog {
  Q_OBJECT

public:
  explicit FpgaConfig( Editor *editor, QWidget *parent = nullptr, GraphicElement *elm = nullptr );
  ~FpgaConfig( );

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
  void onImportPinList();
  void onExportPinList();

  void onTimeRefresh();

  void onTryToConnect();
  void onEditPortMapping(int row, int column);
  void comboboxItemChanged(QString arg);

  void onApplyConfig(QAbstractButton* btn);
  void onRejectConfig();

private:
  Ui::FpgaConfig *ui;
  Fpga *elm;
  Editor *editor;
  QNetworkAccessManager *manager;
  RemoteLabOption currentOption;
  QTimer timer;
};

#endif /* FPGACONFIG_H */
