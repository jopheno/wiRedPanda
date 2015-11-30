#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "editor.h"
#include "label.h"
#include "scene.h"

#include <QDir>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QMainWindow>
#include <QUndoView>

namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow( QWidget *parent = 0 );
  ~MainWindow( );

  bool save( );
  void show( );
  void clear( );
  QFileInfo getCurrentFile( ) const;
  void setCurrentFile( const QFileInfo &value );

  void open( const QString &fname );
  void createUndoView( );
  int confirmSave( );
  void updateRecentBoxes( );
  QString getOpenBoxFile( );

  bool closeFile( );

private slots:
  void scrollView( int dx, int dy );

  void on_actionExit_triggered( );

  void on_actionNew_triggered( );

  void on_actionWires_triggered( bool checked );

  void on_actionRotate_right_triggered( );

  void on_actionRotate_left_triggered( );

  void on_actionOpen_triggered( );

  void on_actionSave_triggered( );

  void on_actionAbout_triggered( );

  void on_actionAbout_Qt_triggered( );

  void on_actionDelete_triggered( );

  void on_lineEdit_textEdited( const QString & );

  void on_actionCopy_triggered( );

  void on_actionPaste_triggered( );

  void on_actionSave_As_triggered( );

  void on_actionCut_triggered( );

  void on_actionSelect_all_triggered( );

  void on_actionOpen_Box_triggered( );

  void on_lineEdit_textChanged( const QString &arg1 );

  void on_lineEdit_returnPressed( );

  void on_actionReload_File_triggered( );

  void on_actionGates_triggered( bool checked );

private:
  Ui::MainWindow *ui;
  Editor *editor;
  QFileInfo currentFile;
  QDir defaultDirectory;
  QUndoView *undoView;
  Label *firstResult;

  QAction *undoAction;
  QAction *redoAction;

  /* QWidget interface */
protected:
  void closeEvent( QCloseEvent *e );
  void resizeEvent( QResizeEvent* );

  /* QWidget interface */
protected:
  void keyPressEvent( QKeyEvent* );
  void keyReleaseEvent( QKeyEvent* );
};

#endif /* MAINWINDOW_H */
