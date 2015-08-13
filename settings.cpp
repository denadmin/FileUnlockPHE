/**************************************************************************
 *  FileUnlock plug-in for FAR 3.0                                        *
 *  Copyright (C) 2010-2014 by Artem Senichev <artemsen@gmail.com>        *
 *  https://sourceforge.net/projects/farplugs/                            *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#include "settings.h"
#include "string_rc.h"

bool settings::use_driver = true;

const wchar_t* param_use_ar = L"use_driver";


void settings::load()
{
	FarSettingsCreate fsc;
	ZeroMemory(&fsc, sizeof(fsc));
	fsc.StructSize = sizeof(fsc);
	fsc.Guid = _FPG;
	fsc.Handle = INVALID_HANDLE_VALUE;
	if (!_PSI.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &fsc))
		return;

	FarSettingsItem fsi;
	ZeroMemory(&fsi, sizeof(fsi));
	fsi.StructSize = sizeof(fsi);


	fsi.Name = param_use_ar;
	fsi.Type = FST_QWORD;
	if (_PSI.SettingsControl(fsc.Handle, SCTL_GET, 0, &fsi))
		use_driver = fsi.Number != 0;


	_PSI.SettingsControl(fsc.Handle, SCTL_FREE, 0, nullptr);
}


void settings::save()
{
	FarSettingsCreate fsc;
	ZeroMemory(&fsc, sizeof(fsc));
	fsc.StructSize = sizeof(fsc);
	fsc.Guid = _FPG;
	fsc.Handle = INVALID_HANDLE_VALUE;
	if (!_PSI.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &fsc))
		return;

	FarSettingsItem fsi;
	ZeroMemory(&fsi, sizeof(fsi));
	fsi.StructSize = sizeof(fsi);


	fsi.Name = param_use_ar;
	fsi.Type = FST_QWORD;
	fsi.Number = use_driver ? 1 : 0;
	_PSI.SettingsControl(fsc.Handle, SCTL_SET, 0, &fsi);

	_PSI.SettingsControl(fsc.Handle, SCTL_FREE, 0, nullptr);
}


void settings::configure()
{
	const FarDialogItem dlg_items[] = {
		/* 0 */ { DI_DOUBLEBOX, 3, 1, 45, 7, 0, nullptr, nullptr, LIF_NONE, _PSI.GetMsg(&_FPG, ps_title) },
		/* 2 */ { DI_CHECKBOX,  5, 3, 43, 3, use_driver ? 1 : 0, nullptr, nullptr, LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_use_admr) },
		/* 5 */ { DI_TEXT,      0, 5,  0, 6, 0, nullptr, nullptr, DIF_SEPARATOR },
		/* 6 */ { DI_BUTTON,    0, 6,  0, 6, 0, nullptr, nullptr, DIF_CENTERGROUP | DIF_DEFAULTBUTTON, _PSI.GetMsg(&_FPG, ps_ok) },
		/* 7 */ { DI_BUTTON,    0, 6,  0, 6, 0, nullptr, nullptr, DIF_CENTERGROUP, _PSI.GetMsg(&_FPG, ps_cancel) }
	};

	const HANDLE dlg = _PSI.DialogInit(&_FPG, &_FPG, -1, -1, 49, 9, nullptr, dlg_items, sizeof(dlg_items) / sizeof(dlg_items[0]), 0, FDLG_NONE, nullptr, nullptr);
	const intptr_t rc = _PSI.DialogRun(dlg);
	if (rc >= 0 && rc != 7) {
		use_driver = _PSI.SendDlgMessage(dlg, DM_GETCHECK, 1, nullptr) != 0;
		save();
	}
	_PSI.DialogFree(dlg);
}
