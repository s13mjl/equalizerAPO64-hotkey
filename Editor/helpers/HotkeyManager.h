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

#include <QObject>
#include <QHash>
#include <QKeySequence>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <windows.h>

class FilterTable;

struct HotkeyInfo
{
    int id;
    UINT modifiers;
    UINT vk;
    QString filterId;
    bool enabled;
};

// Worker thread that runs a GetMessage loop to receive WM_HOTKEY
class HotkeyWorker : public QThread
{
    Q_OBJECT

public:
    explicit HotkeyWorker(QObject* parent = nullptr);
    ~HotkeyWorker();

    void run() override;

    // These are called from the main thread (thread-safe via mutex)
    bool registerHotkey(int id, UINT modifiers, UINT vk);
    void unregisterHotkey(int id);
    void unregisterAll();
    void stop();

signals:
    void hotkeyPressed(int id);

private:
    struct PendingReg
    {
        int id;
        UINT modifiers;
        UINT vk;
        bool unregister;  // true = unregister, false = register
    };
    
    QMutex mutex;
    QWaitCondition cond;
    QList<PendingReg> pendingOps;
    QHash<int, bool> registeredIds;
    DWORD workerThreadId;
    volatile bool shouldStop;
    
    void processPendingOps();
};

class HotkeyManager : public QObject
{
    Q_OBJECT

public:
    explicit HotkeyManager(FilterTable* parent);
    ~HotkeyManager();

    bool registerHotkey(const QString& filterId, const QKeySequence& keySequence);
    void unregisterHotkey(const QString& filterId);
    void clearAll();
    
    QKeySequence getHotkey(const QString& filterId) const;
    bool hasHotkey(const QString& filterId) const;
    bool handleHotkey(int id);
    
    void loadSettings();
    void saveSettings();

    static bool keySequenceToWinHotkey(const QKeySequence& keySeq, UINT& modifiers, UINT& vk);
    static QKeySequence winHotkeyToKeySequence(UINT modifiers, UINT vk);
    static QString formatKeySequence(const QKeySequence& keySeq);

signals:
    void hotkeyPressed(const QString& filterId);

private slots:
    void onWorkerHotkey(int id);

private:
    FilterTable* filterTable;
    HotkeyWorker* worker;
    QHash<QString, HotkeyInfo> hotkeys;
    int nextHotkeyId;
    
    static const int HOTKEY_BASE_ID = 0x0100;
    static const int HOTKEY_MAX_ID = 0xBFFF;
};
