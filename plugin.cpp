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

#include "common.h"
#include "string_rc.h"
#include "settings.h"
#include "version.h"
#include "unlocker.h"


//! Plugin GUID {F39EFDBF-A2E3-44F5-B4DF-519548A62BFA}
const GUID _FPG = { 0xf39efdbf, 0xa2e3, 0x44f5, { 0xb4, 0xdf, 0x51, 0x95, 0x48, 0xa6, 0x2b, 0xfa } };

PluginStartupInfo    _PSI;
FarStandardFunctions _FSF;


void WINAPI SetStartupInfoW(const PluginStartupInfo* psi)
{
	_PSI = *psi;
	_FSF = *psi->FSF;
	_PSI.FSF = &_FSF;

	settings::load();
}


void WINAPI GetGlobalInfoW(GlobalInfo* info)
{
	info->StructSize = sizeof(GlobalInfo);
	info->MinFarVersion = FARMANAGERVERSION;
	info->Version = MAKEFARVERSION(PLUGIN_VERSION_NUM, VS_RELEASE);
	info->Guid = _FPG;
	info->Title = TEXT(PLUGIN_NAME);
	info->Description = TEXT(PLUGIN_DESCR);
	info->Author = TEXT(PLUGIN_AUTHOR);
}


void WINAPI GetPluginInfoW(PluginInfo* info)
{
	assert(info);

	info->StructSize = sizeof(PluginInfo);

	static const wchar_t* menu_strings[1];
	menu_strings[0] = _PSI.GetMsg(&_FPG, ps_title);

	info->PluginConfig.Guids = &_FPG;
	info->PluginConfig.Strings = menu_strings;
	info->PluginConfig.Count = sizeof(menu_strings) / sizeof(menu_strings[0]);
	info->PluginMenu.Guids = &_FPG;
	info->PluginMenu.Strings = menu_strings;
	info->PluginMenu.Count = sizeof(menu_strings) / sizeof(menu_strings[0]);




#ifdef _DEBUG
	info->Flags |= PF_PRELOAD;
#endif // _DEBUG
}


HANDLE WINAPI OpenW(const OpenInfo* info)
{
	if (!info || info->StructSize < sizeof(OpenInfo))
		return nullptr;

	//Determine file name to analyze
	wstring file_name;
	if (info->OpenFrom == OPEN_COMMANDLINE && info->Data) {
		const OpenCommandLineInfo* ocli = reinterpret_cast<const OpenCommandLineInfo*>(info->Data);
		if (!ocli || ocli->StructSize < sizeof(OpenCommandLineInfo) || !ocli->CommandLine || !ocli->CommandLine[0])
			return nullptr;
		//Get command line
		wstring cmd_line = ocli->CommandLine;
		size_t pos = 0;
		while ((pos = cmd_line.find(L'\"', pos)) != string::npos)
			cmd_line.erase(pos, 1);
		while (!cmd_line.empty() && iswspace(cmd_line[0]))
			cmd_line.erase(0, 1);
		while (!cmd_line.empty() && iswspace(cmd_line[cmd_line.length() - 1]))
			cmd_line.erase(cmd_line.length() - 1, 1);
		if (cmd_line.empty())
			return nullptr;
		//Expand environment variables in path string
		wstring exp_path(2048, 0);
		if (ExpandEnvironmentStrings(cmd_line.c_str(), &exp_path.front(), static_cast<DWORD>(exp_path.size() - 1)))
			exp_path.resize(lstrlen(exp_path.c_str()));
		else
			exp_path = cmd_line;
		const size_t path_len = _FSF.ConvertPath(CPM_FULL, exp_path.c_str(), nullptr, 0);
		if (path_len) {
			file_name.resize(path_len);
			_FSF.ConvertPath(CPM_FULL, exp_path.c_str(), &file_name[0], path_len);
		}
	}
	else if (info->OpenFrom == OPEN_PLUGINSMENU) {
		PanelInfo pi;
		ZeroMemory(&pi, sizeof(pi));
		pi.StructSize = sizeof(pi);
		if (!_PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &pi))
			return nullptr;
		const intptr_t ppi_len = _PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, static_cast<int>(pi.CurrentItem), nullptr);
		if (ppi_len == 0)
			return nullptr;
		vector<unsigned char> buffer(ppi_len);
		PluginPanelItem* ppi = reinterpret_cast<PluginPanelItem*>(&buffer.front());
		FarGetPluginPanelItem fgppi;
		ZeroMemory(&fgppi, sizeof(fgppi));
		fgppi.StructSize = sizeof(fgppi);
		fgppi.Size = buffer.size();
		fgppi.Item = ppi;
		if (!_PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, static_cast<int>(pi.CurrentItem), &fgppi))
			return nullptr;
		const size_t file_name_len = _FSF.ConvertPath(CPM_FULL, ppi->FileName, nullptr, 0);
		if (file_name_len) {
			file_name.resize(file_name_len);
			_FSF.ConvertPath(CPM_FULL, ppi->FileName, &file_name[0], file_name_len);
		}
	}

	if (!file_name.empty()) {
		unlocker u;
		u.process(file_name.c_str());
	}

	return nullptr;
}


intptr_t WINAPI ConfigureW(const ConfigureInfo* /*info*/)
{
	settings::configure();
	return 0;
}
