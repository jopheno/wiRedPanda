/*
 * Copyright 2015 - 2021, GIBIS-Unifesp and the wiRedPanda contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CLOCKDIALOG_H
#define CLOCKDIALOG_H

#include <QDialog>

namespace Ui
{
class clockDialog;
}

class clockDialog : public QDialog
{
    Q_OBJECT

public:
    explicit clockDialog(QWidget *parent = nullptr);
    int getFrequency();
    ~clockDialog() override;

private slots:
    void on_cancelPushButton_clicked();

    void on_okPushButton_clicked();

private:
    Ui::clockDialog *ui;
    bool canceled;
};

#endif /* CLOCKDIALOG_H */
