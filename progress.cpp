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

#include "progress.h"
#include "string_rc.h"
#include <shlobj.h>

#define PROGRESS_WIDTH 50


progress::progress(const uint32_t max_value)
:	_visible(true),
	_max_value(max_value)
{
	_title = _PSI.GetMsg(&_FPG, ps_title);
	_bar.resize(PROGRESS_WIDTH);
	update(0, 0);
}


progress::~progress()
{
	hide();
}


void progress::hide()
{
	if (_visible) {
		_PSI.AdvControl(&_FPG, ACTL_PROGRESSNOTIFY, 0, nullptr);
		_PSI.AdvControl(&_FPG, ACTL_SETPROGRESSSTATE, TBPF_NOPROGRESS, nullptr);
		_PSI.PanelControl(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, nullptr);
		_PSI.PanelControl(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, nullptr);
		_visible = false;
	}
}


void progress::update(const uint32_t progr, const uint16_t found)
{
	if (!_max_value)
		return;

	const size_t percent = static_cast<size_t>((progr * 100) / _max_value);
	assert(percent <= 100);

	ProgressValue pv;
	ZeroMemory(&pv, sizeof(pv));
	pv.StructSize = sizeof(pv);
	pv.Completed = percent;
	pv.Total = 100;
	_PSI.AdvControl(&_FPG, ACTL_SETPROGRESSVALUE, 0, &pv);

	const size_t fill_length = percent * _bar.size() / 100;
	fill(_bar.begin() + fill_length, _bar.end(), L'\x2591');
	fill(_bar.begin(), _bar.begin() + fill_length, L'\x2588');

	wchar_t ft[64];
	swprintf_s(ft, _PSI.GetMsg(&_FPG, ps_found), found);

	const wchar_t* msg[] = { _title, _bar.c_str(), ft };
	_PSI.Message(&_FPG, &_FPG, FMSG_NONE, nullptr, msg, sizeof(msg) / sizeof(msg[0]), 0);
}


bool progress::aborted()
{
	HANDLE std_in = GetStdHandle(STD_INPUT_HANDLE);
	INPUT_RECORD rec;
	DWORD read_count = 0;
	while (PeekConsoleInput(std_in, &rec, 1, &read_count) && read_count != 0) {
		ReadConsoleInput(std_in, &rec, 1, &read_count);
		if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE && rec.Event.KeyEvent.bKeyDown)
			return true;
	}
	return false;
}
