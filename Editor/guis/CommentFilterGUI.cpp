/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2014  Jonas Thedering

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

#include "Editor/helpers/GUIHelper.h"
#include "Editor/helpers/HotkeyManager.h"
#include "Editor/dialogs/HotkeyEditDialog.h"
#include "CommentFilterGUI.h"
#include "ui_CommentFilterGUI.h"
#include <QMessageBox>
#include <QDebug>

using namespace std;

CommentFilterGUI::CommentFilterGUI(IFilterGUI* child, bool isComment)
{
	ui = new Ui::CommentFilterGUI;
	ui->setupUi(this);

	this->child = child;
	connect(child, SIGNAL(updateModel()), this, SIGNAL(updateModel()));
	connect(child, SIGNAL(updateChannels()), this, SIGNAL(updateChannels()));

	ui->horizontalLayout->removeItem(ui->horizontalSpacer);
	ui->horizontalLayout->addWidget(child);

	ui->actionPowerOn->setChecked(!isComment);
	ui->toolBar->setIconSize(GUIHelper::scale(QSize(24, 24)));
	ui->toolBar->addAction(ui->actionPowerOn);
	
	// Add hotkey action - use text since we don't have a keyboard icon
	ui->actionSetHotkey->setText("🔑");
	ui->toolBar->addAction(ui->actionSetHotkey);
	
	ui->toolBar->updateMaximumHeight();

	child->setEnabled(!isComment);
	
	// Generate unique filter ID based on object address
	filterId = QString::number(reinterpret_cast<quintptr>(this), 16);
	
	updateHotkeyTooltip();
}

CommentFilterGUI::~CommentFilterGUI()
{
	delete ui;
}

void CommentFilterGUI::configureChannels(vector<wstring>& channelNames)
{
	if (ui->actionPowerOn->isChecked())
	{
		child->configureChannels(channelNames);
	}
	else
	{
		// prevent modification of channel names
		vector<wstring> copy = channelNames;
		child->configureChannels(copy);
	}
}

void CommentFilterGUI::store(QString& command, QString& parameters)
{
	child->store(command, parameters);

	if (!ui->actionPowerOn->isChecked())
		command = "# " + command;
}

void CommentFilterGUI::loadPreferences(const QVariantMap& prefs)
{
	child->loadPreferences(prefs);
	
	// Load hotkey if stored
	if (prefs.contains("hotkey"))
	{
		hotkey = QKeySequence(prefs.value("hotkey").toString());
		updateHotkeyTooltip();
	}
}

void CommentFilterGUI::storePreferences(QVariantMap& prefs)
{
	child->storePreferences(prefs);
	
	// Store hotkey
	if (!hotkey.isEmpty())
	{
		prefs["hotkey"] = hotkey.toString();
	}
}

QKeySequence CommentFilterGUI::getHotkey() const
{
	return hotkey;
}

void CommentFilterGUI::setHotkey(const QKeySequence& keySeq)
{
	hotkey = keySeq;
	updateHotkeyTooltip();
}

QString CommentFilterGUI::getFilterId() const
{
	return filterId;
}

void CommentFilterGUI::toggleEnabled()
{
	ui->actionPowerOn->toggle();
}

void CommentFilterGUI::updateHotkeyTooltip()
{
	QString tooltip;
	if (hotkey.isEmpty())
	{
		tooltip = tr("Click to set hotkey");
	}
	else
	{
		tooltip = tr("Hotkey: %1 (click to change)").arg(hotkey.toString(QKeySequence::NativeText));
	}
	ui->actionSetHotkey->setToolTip(tooltip);
	ui->actionSetHotkey->setStatusTip(tooltip);
}

void CommentFilterGUI::on_actionPowerOn_toggled(bool checked)
{
	child->setEnabled(checked);

	emit updateModel();
}

void CommentFilterGUI::on_actionSetHotkey_triggered()
{
	HotkeyEditDialog dialog(this);
	dialog.setKeySequence(hotkey);
	
	if (dialog.exec() == QDialog::Accepted)
	{
		QKeySequence newHotkey = dialog.getKeySequence();
		
		// If hotkey changed, update it
		if (newHotkey != hotkey)
		{
			setHotkey(newHotkey);
			
			// Notify FilterTable to re-register the hotkey
			emit hotkeyChanged();
			
			emit updateModel();  // Trigger save of preferences
		}
	}
}
