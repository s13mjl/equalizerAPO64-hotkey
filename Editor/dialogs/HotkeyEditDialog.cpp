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

#include "HotkeyEditDialog.h"
#include "ui_HotkeyEditDialog.h"
#include <QKeyEvent>
#include <QDebug>

HotkeyEditDialog::HotkeyEditDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::HotkeyEditDialog)
    , capturing(false)
    , currentKey(0)
    , currentModifiers(Qt::NoModifier)
{
    ui->setupUi(this);
    
    setWindowTitle(tr("Set Hotkey"));
    
    // Install event filter to capture keys
    ui->keyLabel->installEventFilter(this);
    
    connect(ui->clearButton, &QPushButton::clicked, this, &HotkeyEditDialog::on_clearButton_clicked);
    connect(ui->okButton, &QPushButton::clicked, this, &HotkeyEditDialog::on_okButton_clicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &HotkeyEditDialog::on_cancelButton_clicked);
    
    updateDisplay();
}

HotkeyEditDialog::~HotkeyEditDialog()
{
    delete ui;
}

void HotkeyEditDialog::setKeySequence(const QKeySequence& keySeq)
{
    currentKeySequence = keySeq;
    if (!keySeq.isEmpty())
    {
        // Qt6: use QKeyCombination instead of toCombined() bitmask
        QKeyCombination comb = keySeq[0];
        currentKey = comb.key();
        currentModifiers = comb.keyboardModifiers();
    }
    else
    {
        currentModifiers = Qt::NoModifier;
        currentKey = 0;
    }
    updateDisplay();
}

QKeySequence HotkeyEditDialog::getKeySequence() const
{
    return currentKeySequence;
}

bool HotkeyEditDialog::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui->keyLabel && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        
        // Ignore modifier-only keys
        int key = keyEvent->key();
        if (key == Qt::Key_Shift || key == Qt::Key_Control || 
            key == Qt::Key_Alt || key == Qt::Key_Meta ||
            key == Qt::Key_AltGr)
        {
            return true;
        }
        
        currentModifiers = keyEvent->modifiers();
        currentKey = keyEvent->key();
        
        // Build the key sequence (Qt6 style: use QKeyCombination)
        QKeyCombination comb(currentModifiers, static_cast<Qt::Key>(currentKey));
        currentKeySequence = QKeySequence(comb);
        
        updateDisplay();
        return true;
    }
    
    return QDialog::eventFilter(obj, event);
}

void HotkeyEditDialog::keyPressEvent(QKeyEvent* event)
{
    // Handle escape to close dialog
    if (event->key() == Qt::Key_Escape)
    {
        reject();
        return;
    }
    
    // Ignore modifier-only keys
    int key = event->key();
    if (key == Qt::Key_Shift || key == Qt::Key_Control || 
        key == Qt::Key_Alt || key == Qt::Key_Meta ||
        key == Qt::Key_AltGr)
    {
        return;
    }
    
    currentModifiers = event->modifiers();
    currentKey = key;
    
    // Build the key sequence (Qt6 style using QKeyCombination)
    QKeyCombination comb(currentModifiers, static_cast<Qt::Key>(currentKey));
    currentKeySequence = QKeySequence(comb);
    
    updateDisplay();
}

void HotkeyEditDialog::keyReleaseEvent(QKeyEvent* event)
{
    // Not needed for this implementation
    QDialog::keyReleaseEvent(event);
}

void HotkeyEditDialog::on_clearButton_clicked()
{
    currentKeySequence = QKeySequence();
    currentKey = 0;
    currentModifiers = Qt::NoModifier;
    updateDisplay();
}

void HotkeyEditDialog::on_okButton_clicked()
{
    accept();
}

void HotkeyEditDialog::on_cancelButton_clicked()
{
    reject();
}

void HotkeyEditDialog::updateDisplay()
{
    if (currentKeySequence.isEmpty())
    {
        ui->keyLabel->setText(tr("Press a key combination..."));
        ui->keyLabel->setStyleSheet("QLabel { color: gray; font-size: 14px; padding: 10px; border: 2px dashed gray; border-radius: 5px; }");
    }
    else
    {
        QString text = currentKeySequence.toString(QKeySequence::NativeText);
        ui->keyLabel->setText(text);
        ui->keyLabel->setStyleSheet("QLabel { color: black; font-size: 14px; padding: 10px; border: 2px solid green; border-radius: 5px; background-color: #e8f5e9; }");
    }
}

QString HotkeyEditDialog::keyEventToString(QKeyEvent* event)
{
    QStringList parts;
    
    if (event->modifiers() & Qt::ControlModifier)
        parts << "Ctrl";
    if (event->modifiers() & Qt::AltModifier)
        parts << "Alt";
    if (event->modifiers() & Qt::ShiftModifier)
        parts << "Shift";
    if (event->modifiers() & Qt::MetaModifier)
        parts << "Meta";
    
    QString keyText = event->text();
    if (keyText.isEmpty() || keyText == " ")
    {
        switch (event->key())
        {
        case Qt::Key_Space: keyText = "Space"; break;
        case Qt::Key_Return: keyText = "Enter"; break;
        case Qt::Key_Enter: keyText = "Enter"; break;
        case Qt::Key_Escape: keyText = "Esc"; break;
        case Qt::Key_Tab: keyText = "Tab"; break;
        case Qt::Key_Backspace: keyText = "Backspace"; break;
        case Qt::Key_Delete: keyText = "Delete"; break;
        case Qt::Key_Insert: keyText = "Insert"; break;
        case Qt::Key_Home: keyText = "Home"; break;
        case Qt::Key_End: keyText = "End"; break;
        case Qt::Key_PageUp: keyText = "PageUp"; break;
        case Qt::Key_PageDown: keyText = "PageDown"; break;
        case Qt::Key_Left: keyText = "Left"; break;
        case Qt::Key_Right: keyText = "Right"; break;
        case Qt::Key_Up: keyText = "Up"; break;
        case Qt::Key_Down: keyText = "Down"; break;
        case Qt::Key_F1: keyText = "F1"; break;
        case Qt::Key_F2: keyText = "F2"; break;
        case Qt::Key_F3: keyText = "F3"; break;
        case Qt::Key_F4: keyText = "F4"; break;
        case Qt::Key_F5: keyText = "F5"; break;
        case Qt::Key_F6: keyText = "F6"; break;
        case Qt::Key_F7: keyText = "F7"; break;
        case Qt::Key_F8: keyText = "F8"; break;
        case Qt::Key_F9: keyText = "F9"; break;
        case Qt::Key_F10: keyText = "F10"; break;
        case Qt::Key_F11: keyText = "F11"; break;
        case Qt::Key_F12: keyText = "F12"; break;
        default:
            keyText = QKeySequence(event->key()).toString();
        }
    }
    
    parts << keyText;
    return parts.join("+");
}
