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

#pragma once

#include "common.h"
#include "pipeinteraction.h"

#include <ph.h>


#ifdef __cplusplus
extern "C" {
#endif

#include <kphuser.h>

#ifdef __cplusplus
}
#endif


#pragma comment(lib, "phlib.lib")


class unlocker
{
public:
	unlocker();
	~unlocker();

	/**
	 * Process file.
	 * \param file_name path
	 */
	void process(const wchar_t* file_name);

	static PPH_STRING SearchString;
	static bool inited;
	static PH_QUEUED_LOCK SearchResultsLock;

private:
	struct lock_info {
		wstring label;
		bool isModule;
		ULONG modType;
		PPH_STRING modName;
		HANDLE processId;
		HANDLE lockHandel;
	};

	typedef struct _SEARCH_HANDLE_CONTEXT
{
	BOOLEAN NeedToFree;
	PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleInfo;
	HANDLE ProcessHandle;
	PPH_STRING ProcessName;
	vector<lock_info> * li;
} SEARCH_HANDLE_CONTEXT, *PSEARCH_HANDLE_CONTEXT;

typedef enum _PHP_OBJECT_RESULT_TYPE
{
	HandleSearchResult,
	ModuleSearchResult,
	MappedFileSearchResult
} PHP_OBJECT_RESULT_TYPE;

typedef struct _PHP_OBJECT_SEARCH_RESULT
{
	HANDLE ProcessId;
	PHP_OBJECT_RESULT_TYPE ResultType;

	HANDLE Handle;
	PPH_STRING TypeName;
	PPH_STRING Name;
	PPH_STRING ProcessName;

	WCHAR HandleString[PH_PTR_STR_LEN_1];

	SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Info;
} PHP_OBJECT_SEARCH_RESULT, *PPHP_OBJECT_SEARCH_RESULT;

private:
	/**
	 * Create pipe.
	 * \return false on error
	 */
	bool create_pipe();

	/**
	 * Start exe module.
	 * \return false on error
	 */
	bool start_exe_module();

	/**
	 * Read lock info list.
	 * \param count items count
	 * \param plain_view output array with items description
	 * \return false on error
	 */
	bool read_item_list(const uint16_t count, vector<lock_info>& plain_view) const;

	/**
	 * Show lock info list.
	 * \param plain_view array with items description
	 * \param index chosen index (UINT16_MAX for all)
	 * \return false if user cancel operation
	 */
	bool show_list(const vector<lock_info>& plain_view, uint16_t& index) const;

	/**
	 * Show error message.
	 * \param err_code error code (0 to use last system error)
	 */
	void show_error(const DWORD err_code = 0) const;


	VOID PhpEnablePrivileges(VOID);
	VOID PhInitializeKph(VOID);

	static NTSTATUS NTAPI SearchHandleFunction(_In_ PVOID Parameter);
	static BOOLEAN  NTAPI EnumModulesCallback(_In_ PPH_MODULE_INFO Module,_In_opt_ PVOID Context);

	static PPH_STRING unlocker::getProcPathById(HANDLE procid);


private:
	const wchar_t*	_file_name;
	wstring	_pipe_name;
	HANDLE	_pipe;
	HANDLE	_exe_process;

	

};
