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

#include "HotkeyManager.h"
#include "Editor/FilterTable.h"
#include "helpers/RegistryHelper.h"
#include <QSettings>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QMutexLocker>

// ==================== Logging ====================

static void logHotkey(const QString& msg)
{
    QFile f(QDir::tempPath() + "/equalizerapo_hotkey.log");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Append))
        return;
    QTextStream s(&f);
    s << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << " " << msg << "\n";
}

#define HOTKEY_REGPATH USER_REGPATH L"\\Hotkeys"

// ==================== HotkeyWorker ====================

HotkeyWorker::HotkeyWorker(QObject* parent)
    : QThread(parent), workerThreadId(0), shouldStop(false)
{
}

HotkeyWorker::~HotkeyWorker()
{
    logHotkey("HotkeyWorker::~HotkeyWorker() destructor");
    stop();
    wait(3000);
}

void HotkeyWorker::run()
{
    workerThreadId = GetCurrentThreadId();
    logHotkey(QString("Worker thread started, tid=%1").arg(workerThreadId));

    // Process any pending ops first (e.g. RegisterHotKey calls)
    processPendingOps();

    // Classic GetMessage loop - blocks until a message arrives
    // This is THE correct way to receive WM_HOTKEY on a worker thread
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if (msg.message == WM_HOTKEY)
        {
            int id = (int)msg.wParam;
            logHotkey(QString("Worker WM_HOTKEY id=%1").arg(id));
            emit hotkeyPressed(id);
        }
        else if (msg.message == WM_NULL)
        {
            // Wakeup from PostThreadMessage(WM_NULL) - check pending ops
            processPendingOps();
        }
        // Other messages are ignored (no DefWindowProc needed - no window)
    }

    // Unregister all hotkeys (must be done in this thread)
    for (auto it = registeredIds.begin(); it != registeredIds.end(); ++it)
    {
        UnregisterHotKey(NULL, it.key());
    }
    registeredIds.clear();

    logHotkey("Worker thread exiting");
}

void HotkeyWorker::processPendingOps()
{
    QMutexLocker locker(&mutex);
    while (!pendingOps.isEmpty())
    {
        PendingReg op = pendingOps.takeFirst();
        locker.unlock();

        if (op.unregister)
        {
            UnregisterHotKey(NULL, op.id);
            registeredIds.remove(op.id);
            logHotkey(QString("Worker unregistered: id=%1").arg(op.id));
        }
        else
        {
            UINT mod = op.modifiers | MOD_NOREPEAT;
            BOOL ok = RegisterHotKey(NULL, op.id, mod, op.vk);
            if (ok)
            {
                registeredIds[op.id] = true;
                logHotkey(QString("Worker RegisterHotKey OK: id=%1 mod=%2 vk=%3").arg(op.id).arg(mod).arg(op.vk));
            }
            else
            {
                DWORD err = GetLastError();
                logHotkey(QString("Worker RegisterHotKey FAILED: id=%1 err=%2").arg(op.id).arg(err));
            }
        }

        locker.relock();
    }
    cond.wakeAll();
}

void HotkeyWorker::stop()
{
    logHotkey("HotkeyWorker::stop() called");
    shouldStop = true;
    if (workerThreadId)
        PostThreadMessage(workerThreadId, WM_QUIT, 0, 0);
}

bool HotkeyWorker::registerHotkey(int id, UINT modifiers, UINT vk)
{
    QMutexLocker locker(&mutex);
    PendingReg op;
    op.id = id;
    op.modifiers = modifiers;
    op.vk = vk;
    op.unregister = false;
    pendingOps.append(op);

    // Wake up the worker thread
    if (workerThreadId)
        PostThreadMessage(workerThreadId, WM_NULL, 0, 0);

    // Wait for processing
    cond.wait(&mutex, 2000);

    return registeredIds.contains(id);
}

void HotkeyWorker::unregisterHotkey(int id)
{
    QMutexLocker locker(&mutex);
    PendingReg op;
    op.id = id;
    op.modifiers = 0;
    op.vk = 0;
    op.unregister = true;
    pendingOps.append(op);

    if (workerThreadId)
        PostThreadMessage(workerThreadId, WM_NULL, 0, 0);

    cond.wait(&mutex, 2000);
}

void HotkeyWorker::unregisterAll()
{
    QMutexLocker locker(&mutex);
    for (auto it = registeredIds.begin(); it != registeredIds.end(); ++it)
    {
        PendingReg op;
        op.id = it.key();
        op.modifiers = 0;
        op.vk = 0;
        op.unregister = true;
        pendingOps.append(op);
    }

    if (workerThreadId)
        PostThreadMessage(workerThreadId, WM_NULL, 0, 0);

    cond.wait(&mutex, 2000);
}

// ==================== HotkeyManager ====================

HotkeyManager::HotkeyManager(FilterTable* parent)
    : QObject(nullptr), filterTable(parent), worker(nullptr), nextHotkeyId(HOTKEY_BASE_ID)
{
    worker = new HotkeyWorker(this);
    worker->start();

    connect(worker, &HotkeyWorker::hotkeyPressed, this, &HotkeyManager::onWorkerHotkey);

    logHotkey("HotkeyManager created, worker started");
}

HotkeyManager::~HotkeyManager()
{
    logHotkey("HotkeyManager::~HotkeyManager() destructor");
    if (worker)
    {
        worker->unregisterAll();
        worker->stop();
        worker->wait(3000);
        delete worker;
    }
}

bool HotkeyManager::registerHotkey(const QString& filterId, const QKeySequence& keySequence)
{
    if (keySequence.isEmpty())
    {
        unregisterHotkey(filterId);
        return true;
    }

    UINT modifiers, vk;
    if (!keySequenceToWinHotkey(keySequence, modifiers, vk))
    {
        return false;
    }

    // Unregister existing hotkey for this filter
    unregisterHotkey(filterId);

    // Check for conflicts
    for (auto it = hotkeys.begin(); it != hotkeys.end(); ++it)
    {
        if (it.value().modifiers == modifiers && it.value().vk == vk)
        {
            return false;
        }
    }

    int id = nextHotkeyId++;
    if (nextHotkeyId > HOTKEY_MAX_ID)
        nextHotkeyId = HOTKEY_BASE_ID;

    // Register in worker thread
    if (!worker->registerHotkey(id, modifiers, vk))
    {
        logHotkey(QString("RegisterHotKey FAILED: filter=%1 key=%2").arg(filterId).arg(keySequence.toString()));
        return false;
    }

    logHotkey(QString("RegisterHotKey OK: filter=%1 id=%2 key=%3 mod=%4 vk=%5")
        .arg(filterId).arg(id).arg(keySequence.toString()).arg(modifiers).arg(vk));

    HotkeyInfo info;
    info.id = id;
    info.modifiers = modifiers;
    info.vk = vk;
    info.filterId = filterId;
    info.enabled = true;
    hotkeys[filterId] = info;

    saveSettings();
    return true;
}

void HotkeyManager::unregisterHotkey(const QString& filterId)
{
    if (!hotkeys.contains(filterId))
        return;

    const HotkeyInfo& info = hotkeys[filterId];
    worker->unregisterHotkey(info.id);
    hotkeys.remove(filterId);

    saveSettings();
}

void HotkeyManager::clearAll()
{
    worker->unregisterAll();
    hotkeys.clear();
}

QKeySequence HotkeyManager::getHotkey(const QString& filterId) const
{
    if (!hotkeys.contains(filterId))
        return QKeySequence();

    const HotkeyInfo& info = hotkeys[filterId];
    return winHotkeyToKeySequence(info.modifiers, info.vk);
}

bool HotkeyManager::hasHotkey(const QString& filterId) const
{
    return hotkeys.contains(filterId);
}

bool HotkeyManager::handleHotkey(int id)
{
    for (const auto& info : hotkeys)
    {
        if (info.id == id)
        {
            logHotkey(QString("handleHotkey matched: id=%1 filter=%2").arg(id).arg(info.filterId));
            emit hotkeyPressed(info.filterId);
            return true;
        }
    }
    logHotkey(QString("handleHotkey NO MATCH: id=%1").arg(id));
    return false;
}

void HotkeyManager::onWorkerHotkey(int id)
{
    logHotkey(QString("onWorkerHotkey: id=%1").arg(id));
    handleHotkey(id);
}

void HotkeyManager::loadSettings()
{
    QSettings settings(QString::fromWCharArray(HOTKEY_REGPATH), QSettings::NativeFormat);

    worker->unregisterAll();
    hotkeys.clear();

    QStringList filterIds = settings.childGroups();
    for (const QString& filterId : filterIds)
    {
        settings.beginGroup(filterId);
        QString keySeqStr = settings.value("keySequence").toString();
        settings.endGroup();

        if (!keySeqStr.isEmpty())
        {
            QKeySequence keySeq(keySeqStr);
            if (!keySeq.isEmpty())
            {
                // Store but don't register - registration happens when filter is created
            }
        }
    }
}

void HotkeyManager::saveSettings()
{
    QSettings settings(QString::fromWCharArray(HOTKEY_REGPATH), QSettings::NativeFormat);
    settings.clear();

    for (auto it = hotkeys.begin(); it != hotkeys.end(); ++it)
    {
        const QString& filterId = it.key();
        const HotkeyInfo& info = it.value();

        settings.beginGroup(filterId);
        QKeySequence keySeq = winHotkeyToKeySequence(info.modifiers, info.vk);
        settings.setValue("keySequence", keySeq.toString());
        settings.endGroup();
    }
}

bool HotkeyManager::keySequenceToWinHotkey(const QKeySequence& keySeq, UINT& modifiers, UINT& vk)
{
    if (keySeq.isEmpty())
        return false;

    int key = keySeq[0].toCombined();
    modifiers = 0;

    if (key & Qt::ShiftModifier)
        modifiers |= MOD_SHIFT;
    if (key & Qt::ControlModifier)
        modifiers |= MOD_CONTROL;
    if (key & Qt::AltModifier)
        modifiers |= MOD_ALT;
    if (key & Qt::MetaModifier)
        modifiers |= MOD_WIN;

    int keyCode = key & ~Qt::KeyboardModifierMask;

    if (keyCode >= Qt::Key_A && keyCode <= Qt::Key_Z)
    {
        vk = 'A' + (keyCode - Qt::Key_A);
    }
    else if (keyCode >= Qt::Key_0 && keyCode <= Qt::Key_9)
    {
        vk = '0' + (keyCode - Qt::Key_0);
    }
    else if (keyCode >= Qt::Key_F1 && keyCode <= Qt::Key_F24)
    {
        vk = VK_F1 + (keyCode - Qt::Key_F1);
    }
    else
    {
        switch (keyCode)
        {
        case Qt::Key_Space: vk = VK_SPACE; break;
        case Qt::Key_Return: vk = VK_RETURN; break;
        case Qt::Key_Enter: vk = VK_RETURN; break;
        case Qt::Key_Escape: vk = VK_ESCAPE; break;
        case Qt::Key_Tab: vk = VK_TAB; break;
        case Qt::Key_Backspace: vk = VK_BACK; break;
        case Qt::Key_Insert: vk = VK_INSERT; break;
        case Qt::Key_Delete: vk = VK_DELETE; break;
        case Qt::Key_Home: vk = VK_HOME; break;
        case Qt::Key_End: vk = VK_END; break;
        case Qt::Key_PageUp: vk = VK_PRIOR; break;
        case Qt::Key_PageDown: vk = VK_NEXT; break;
        case Qt::Key_Left: vk = VK_LEFT; break;
        case Qt::Key_Right: vk = VK_RIGHT; break;
        case Qt::Key_Up: vk = VK_UP; break;
        case Qt::Key_Down: vk = VK_DOWN; break;
        case Qt::Key_Print: vk = VK_PRINT; break;
        case Qt::Key_ScrollLock: vk = VK_SCROLL; break;
        case Qt::Key_Pause: vk = VK_PAUSE; break;
        default:
            vk = MapVirtualKey(keyCode, MAPVK_VK_TO_VSC);
            if (vk == 0)
                return false;
        }
    }

    return true;
}

QKeySequence HotkeyManager::winHotkeyToKeySequence(UINT modifiers, UINT vk)
{
    int key = 0;

    if (vk >= 'A' && vk <= 'Z')
    {
        key = Qt::Key_A + (vk - 'A');
    }
    else if (vk >= '0' && vk <= '9')
    {
        key = Qt::Key_0 + (vk - '0');
    }
    else if (vk >= VK_F1 && vk <= VK_F24)
    {
        key = Qt::Key_F1 + (vk - VK_F1);
    }
    else
    {
        switch (vk)
        {
        case VK_SPACE: key = Qt::Key_Space; break;
        case VK_RETURN: key = Qt::Key_Return; break;
        case VK_ESCAPE: key = Qt::Key_Escape; break;
        case VK_TAB: key = Qt::Key_Tab; break;
        case VK_BACK: key = Qt::Key_Backspace; break;
        case VK_INSERT: key = Qt::Key_Insert; break;
        case VK_DELETE: key = Qt::Key_Delete; break;
        case VK_HOME: key = Qt::Key_Home; break;
        case VK_END: key = Qt::Key_End; break;
        case VK_PRIOR: key = Qt::Key_PageUp; break;
        case VK_NEXT: key = Qt::Key_PageDown; break;
        case VK_LEFT: key = Qt::Key_Left; break;
        case VK_RIGHT: key = Qt::Key_Right; break;
        case VK_UP: key = Qt::Key_Up; break;
        case VK_DOWN: key = Qt::Key_Down; break;
        case VK_PRINT: key = Qt::Key_Print; break;
        case VK_SCROLL: key = Qt::Key_ScrollLock; break;
        case VK_PAUSE: key = Qt::Key_Pause; break;
        default:
            key = vk;
        }
    }

    if (modifiers & MOD_SHIFT)
        key |= Qt::ShiftModifier;
    if (modifiers & MOD_CONTROL)
        key |= Qt::ControlModifier;
    if (modifiers & MOD_ALT)
        key |= Qt::AltModifier;
    if (modifiers & MOD_WIN)
        key |= Qt::MetaModifier;

    return QKeySequence(key);
}

QString HotkeyManager::formatKeySequence(const QKeySequence& keySeq)
{
    if (keySeq.isEmpty())
        return QObject::tr("None");
    return keySeq.toString(QKeySequence::NativeText);
}
