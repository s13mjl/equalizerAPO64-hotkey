/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2026  EqualizerAPO Contributors

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include <QDialog>
#include <QKeySequence>

namespace Ui {
class HotkeyEditDialog;
}

class HotkeyEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HotkeyEditDialog(QWidget* parent = nullptr);
    ~HotkeyEditDialog();

    void setKeySequence(const QKeySequence& keySeq);
    QKeySequence getKeySequence() const;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void on_clearButton_clicked();
    void on_okButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::HotkeyEditDialog* ui;
    QKeySequence currentKeySequence;
    bool capturing;
    int currentKey;
    Qt::KeyboardModifiers currentModifiers;

    void updateDisplay();
    QString keyEventToString(QKeyEvent* event);
};
