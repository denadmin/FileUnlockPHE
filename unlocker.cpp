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

#include "unlocker.h"
#include "string_rc.h"
#include "settings.h"
#include "progress.h"


PPH_STRING unlocker::SearchString = NULL;
bool unlocker::inited = false;

PH_QUEUED_LOCK unlocker::SearchResultsLock = PH_QUEUED_LOCK_INIT;


VOID unlocker::PhInitializeKph(
	VOID
	)
{

static PH_STRINGREF kprocesshacker = PH_STRINGREF_INIT(L"kprocesshacker.sys");
PPH_STRING kprocesshackerFileName = NULL;
KPH_PARAMETERS parameters;

wstring module_path = _PSI.ModuleName;
module_path.resize(module_path.rfind(L'\\') + 1);
module_path += kprocesshacker.Buffer;

kprocesshackerFileName = PhCreateString((PWSTR)module_path.c_str());

parameters.SecurityLevel = KphSecurityPrivilegeCheck;
parameters.CreateDynamicConfiguration = TRUE;

KphConnect2Ex(L"KProcessHacker2", kprocesshackerFileName->Buffer, &parameters);

PhDereferenceObject(kprocesshackerFileName);



}


PPH_STRING unlocker::getProcPathById(HANDLE procid){

PPH_STRING fileName = NULL;
fileName = PhCreateStringEx(NULL, 1024);


HANDLE processHandle = NULL;
PhOpenProcess(&processHandle, ProcessQueryAccess, procid);
		
// If we're dealing with System (PID 4), we need to get the
// kernel file name. Otherwise, get the image file name.
	if (procid != SYSTEM_PROCESS_ID)
	{
		if (WindowsVersion >= WINDOWS_VISTA)
		{
			if (processHandle)
			{
				PhGetProcessImageFileNameWin32(processHandle, &fileName);
			}
			else
			{
				if (NT_SUCCESS(PhGetProcessImageFileNameByProcessId(procid, &fileName)))
				{
					fileName = PhGetFileName(fileName);
				}
			}
		}
		else
		{
			if (processHandle && NT_SUCCESS(PhGetProcessImageFileName(processHandle, &fileName)))
			{
				fileName = PhGetFileName(fileName);
			}
		}
	}
	else
	{
		fileName = PhGetKernelFileName();
		if (fileName)
		{
			fileName = PhGetFileName(fileName);
		}
	}

NtClose(processHandle);
	
return fileName;

}



BOOLEAN NTAPI unlocker::EnumModulesCallback(
	_In_ PPH_MODULE_INFO Module,
	_In_opt_ PVOID Context
	)
{

	PSEARCH_HANDLE_CONTEXT context = (PSEARCH_HANDLE_CONTEXT)Context;

	WCHAR szShortPath[1024],szLongPath[1024],t[1024];
	wsprintfW(szShortPath,L"\\\\?\\%s",Module->FileName->Buffer);
	memset(szLongPath,0,2048);
	GetLongPathNameW(szShortPath,szLongPath,1024);
	if(wcsstr(szLongPath,L"\\\\?\\") != szLongPath || lstrlenW(szLongPath) <= 4)
		if(lstrlenW(szLongPath) <= 0)
			lstrcpyW(t,Module->FileName->Buffer);
		else
			lstrcpyW(t,szLongPath);
	else
		lstrcpyW(t,szLongPath+4);

	PhDereferenceObject(Module->FileName);
	Module->FileName = PhCreateStringEx(NULL, wcslen(t) * sizeof(WCHAR));
	wcscpy(Module->FileName->Buffer,t);



	if (PhEqualString(Module->FileName, SearchString,TRUE))
	{
		PPH_STRING fileName = getProcPathById(context->ProcessHandle);

		lock_info li;
		wchar_t lock_handle[25];
		wchar_t proc_id[16];

		PhPrintPointer(lock_handle, (PVOID)(HANDLE)Module->BaseAddress);
		swprintf_s(proc_id, L" [%04i] ", context->ProcessHandle);
		li.label = lock_handle;
		li.label += proc_id;
		li.label += context->ProcessName->Buffer;
		li.label += L" (";
		li.label += fileName->Buffer;
		li.label += L")";
		li.isModule = TRUE;
		li.lockHandel = (HANDLE)Module->BaseAddress;
		li.processId = context->ProcessHandle;
		li.modType = Module->Type;
		li.modName = PhDuplicateString(Module->Name);

		PhAcquireQueuedLockExclusive(&SearchResultsLock);
			context->li->push_back(li);
		PhReleaseQueuedLockExclusive(&SearchResultsLock);
	
		PhDereferenceObject(fileName);

	}


	return TRUE;
}



NTSTATUS NTAPI unlocker::SearchHandleFunction(
	_In_ PVOID Parameter
	)
{

	PSEARCH_HANDLE_CONTEXT context = (PSEARCH_HANDLE_CONTEXT)Parameter;
	PPH_STRING typeName;
	PPH_STRING bestObjectName;
	if (NT_SUCCESS(PhGetHandleInformation(context->ProcessHandle,(HANDLE)context->HandleInfo->HandleValue,context->HandleInfo->ObjectTypeIndex,	NULL,&typeName,NULL,&bestObjectName)))
	{

	WCHAR szShortPath[1024],szLongPath[1024],t[1024];
	wsprintfW(szShortPath,L"\\\\?\\%s",bestObjectName->Buffer);
	memset(szLongPath,0,2048);
	GetLongPathNameW(szShortPath,szLongPath,1024);
	if(wcsstr(szLongPath,L"\\\\?\\") != szLongPath || lstrlenW(szLongPath) <= 4)
		if(lstrlenW(szLongPath) <= 0)
			lstrcpyW(t,bestObjectName->Buffer);
		else
			lstrcpyW(t,szLongPath);
	else
		lstrcpyW(t,szLongPath+4);

	PhDereferenceObject(bestObjectName);
	bestObjectName = PhCreateStringEx(NULL, wcslen(t) * sizeof(WCHAR));
	wcscpy(bestObjectName->Buffer,t);

		
		if (PhEqualString(bestObjectName,SearchString,TRUE))
		{

		PPH_STRING fileName = getProcPathById((HANDLE)context->HandleInfo->UniqueProcessId);

		lock_info li;
		wchar_t lock_handle[25];
		wchar_t proc_id[16];

		PhPrintPointer(lock_handle, (PVOID)(HANDLE)context->HandleInfo->HandleValue);
		swprintf_s(proc_id, L" [%04i] ", context->HandleInfo->UniqueProcessId);
		li.label = lock_handle;
		li.label += proc_id;
		li.label += context->ProcessName->Buffer;
		li.label += L" (";
		li.label += fileName->Buffer;
		li.label += L")";
		li.isModule = FALSE;
		li.lockHandel = (HANDLE)context->HandleInfo->HandleValue;
		li.processId  = (HANDLE)context->HandleInfo->UniqueProcessId;
			PhAcquireQueuedLockExclusive(&SearchResultsLock);
				context->li->push_back(li);
			PhReleaseQueuedLockExclusive(&SearchResultsLock);
			
		PhDereferenceObject(fileName);
		
		}
		else
		{
			PhDereferenceObject(typeName);
			PhDereferenceObject(bestObjectName);
		}

		
	}

	if (context->NeedToFree)
		PhFree(context);
	
	return STATUS_SUCCESS;
}





VOID unlocker::PhpEnablePrivileges(
	VOID
	)
{
	HANDLE tokenHandle;

	if (NT_SUCCESS(PhOpenProcessToken(
		&tokenHandle,
		TOKEN_ADJUST_PRIVILEGES,
		NtCurrentProcess()
		)))
	{
		CHAR privilegesBuffer[FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges) + sizeof(LUID_AND_ATTRIBUTES) * 8];
		PTOKEN_PRIVILEGES privileges;
		ULONG i;

		privileges = (PTOKEN_PRIVILEGES)privilegesBuffer;
		privileges->PrivilegeCount = 8;

		for (i = 0; i < privileges->PrivilegeCount; i++)
		{
			privileges->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
			privileges->Privileges[i].Luid.HighPart = 0;
		}

		privileges->Privileges[0].Luid.LowPart = SE_DEBUG_PRIVILEGE;
		privileges->Privileges[1].Luid.LowPart = SE_INC_BASE_PRIORITY_PRIVILEGE;
		privileges->Privileges[2].Luid.LowPart = SE_INC_WORKING_SET_PRIVILEGE;
		privileges->Privileges[3].Luid.LowPart = SE_LOAD_DRIVER_PRIVILEGE;
		privileges->Privileges[4].Luid.LowPart = SE_PROF_SINGLE_PROCESS_PRIVILEGE;
		privileges->Privileges[5].Luid.LowPart = SE_RESTORE_PRIVILEGE;
		privileges->Privileges[6].Luid.LowPart = SE_SHUTDOWN_PRIVILEGE;
		privileges->Privileges[7].Luid.LowPart = SE_TAKE_OWNERSHIP_PRIVILEGE;

		NtAdjustPrivilegesToken(
			tokenHandle,
			FALSE,
			privileges,
			0,
			NULL,
			NULL
			);

		NtClose(tokenHandle);
	}

}



unlocker::unlocker()
	: _pipe(INVALID_HANDLE_VALUE), _exe_process(nullptr)
{

SearchString = NULL;

if(!inited){

if (!NT_SUCCESS(PhInitializePhLib()))
	return;

}

PhpEnablePrivileges();
if(settings::use_driver)
	PhInitializeKph();
inited = true;

}


unlocker::~unlocker()
{

	KphDisconnect();
//if(PhHeapHandle)
//		RtlDestroyHeap(PhHeapHandle);

}



void unlocker::process(const wchar_t* file_name)
{
	PSYSTEM_HANDLE_INFORMATION_EX handles = NULL;
	PPH_HASHTABLE processHandleHashtable;
	PVOID processes;
	PSYSTEM_PROCESS_INFORMATION process;
	ULONG i;

	PPH_STRING process_name = NULL;
	PPH_HASHTABLE processHashtable;

	
	
    vector<lock_info> plain_view;

	SearchString = PhCreateStringEx(NULL, wcslen(file_name) * sizeof(WCHAR));
	wcscpy(SearchString->Buffer,file_name);

	processHashtable = PhCreateSimpleHashtable(8);
	
	if (NT_SUCCESS(PhEnumProcesses(&processes)))
		{
			process = PH_FIRST_PROCESS(processes);
			PPH_STRING buf = NULL;
			PPH_STRING tbuf = NULL;
			do
			{

			if (process->ImageName.Buffer){
					tbuf = (PPH_STRING)PhFindItemSimpleHashtable(processHashtable,(PVOID)process->UniqueProcessId);
					if(!tbuf)
					{
						buf = PhCreateStringEx(process->ImageName.Buffer,process->ImageName.Length);
						PhAddItemSimpleHashtable(processHashtable,(PVOID)process->UniqueProcessId,buf);
					}

				}
				PSEARCH_HANDLE_CONTEXT searchHandleContext;

				searchHandleContext = (PSEARCH_HANDLE_CONTEXT)PhAllocate(sizeof(SEARCH_HANDLE_CONTEXT));
				searchHandleContext->NeedToFree = TRUE;
				searchHandleContext->HandleInfo = NULL;
				searchHandleContext->ProcessHandle = process->UniqueProcessId;
				searchHandleContext->ProcessName = buf;
				searchHandleContext->li = &plain_view;
				

				PhEnumGenericModules(
					process->UniqueProcessId,
					NULL,
					PH_ENUM_GENERIC_MAPPED_FILES | PH_ENUM_GENERIC_MAPPED_IMAGES,
					EnumModulesCallback,
					(PVOID)searchHandleContext
					);

				


			} while (process = PH_NEXT_PROCESS(process));

			PhFree(processes);
		}




	if (NT_SUCCESS(PhEnumHandlesEx(&handles)))
	{
	static PH_INITONCE initOnce = PH_INITONCE_INIT;
	static ULONG fileObjectTypeIndex = -1;

	BOOLEAN useWorkQueue = FALSE;
	PH_WORK_QUEUE workQueue;
	processHandleHashtable = PhCreateSimpleHashtable(8);
	
	if (PhBeginInitOnce(&initOnce))
		{
			UNICODE_STRING fileTypeName;

			RtlInitUnicodeString(&fileTypeName, L"File");
			fileObjectTypeIndex = PhGetObjectTypeNumber(&fileTypeName);
			PhEndInitOnce(&initOnce);
		}


	if (!KphIsConnected() && WindowsVersion >= WINDOWS_VISTA)
	{
		useWorkQueue = TRUE;
		PhInitializeWorkQueue(&workQueue, 1, 20, 1000);
	}

	progress prg_wnd(handles->NumberOfHandles);

	for (i = 0; i < handles->NumberOfHandles; i++)
	{
		PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handleInfo = &handles->Handles[i];
		PVOID *processHandlePtr;
		HANDLE processHandle;



			if (prg_wnd.aborted())
				break;
			if(i%1000==0)
				prg_wnd.update(i, plain_view.size());
			
		// Open a handle to the process if we don't already have one.

		processHandlePtr = PhFindItemSimpleHashtable(
			processHandleHashtable,
			(PVOID)handleInfo->UniqueProcessId
			);

		if (processHandlePtr)
		{
			processHandle = (HANDLE)*processHandlePtr;
		}
		else
		{
			if (NT_SUCCESS(PhOpenProcess(
				&processHandle,
				PROCESS_DUP_HANDLE,
				(HANDLE)handleInfo->UniqueProcessId
				)))
			{
				PhAddItemSimpleHashtable(
					processHandleHashtable,
					(PVOID)handleInfo->UniqueProcessId,
					processHandle
					);
			}
			else
			{
				continue;
			}
		}

		process_name = (PPH_STRING)*PhFindItemSimpleHashtable(processHashtable,(PVOID)(HANDLE)handleInfo->UniqueProcessId);

		if(handleInfo->ObjectTypeIndex != (USHORT)fileObjectTypeIndex){continue;}

		if (useWorkQueue && handleInfo->ObjectTypeIndex == (USHORT)fileObjectTypeIndex)
		{
			PSEARCH_HANDLE_CONTEXT searchHandleContext;

			searchHandleContext = (PSEARCH_HANDLE_CONTEXT)PhAllocate(sizeof(SEARCH_HANDLE_CONTEXT));
			searchHandleContext->NeedToFree = TRUE;
			searchHandleContext->HandleInfo = handleInfo;
			searchHandleContext->ProcessHandle = processHandle;
			searchHandleContext->ProcessName = process_name;
			searchHandleContext->li = &plain_view;
			PhQueueItemWorkQueue(&workQueue, SearchHandleFunction, searchHandleContext);
		}
		else
		{
			SEARCH_HANDLE_CONTEXT searchHandleContext;

			searchHandleContext.NeedToFree = FALSE;
			searchHandleContext.HandleInfo = handleInfo;
			searchHandleContext.ProcessHandle = processHandle;
			searchHandleContext.ProcessName = process_name;
			searchHandleContext.li = &plain_view;
			SearchHandleFunction(&searchHandleContext);
		}
	}

	prg_wnd.update(i, plain_view.size());

	if (useWorkQueue)
	{
		PhWaitForWorkQueue(&workQueue);
		PhDeleteWorkQueue(&workQueue);
	}

		{
			PPH_KEY_VALUE_PAIR entry;

			i = 0;

			while (PhEnumHashtable(processHandleHashtable, (PVOID *)&entry, &i))
				NtClose((HANDLE)entry->Value);
		}

		PhDereferenceObject(processHandleHashtable);
		PhFree(handles);

			PPH_KEY_VALUE_PAIR entry;

			i = 0;

			while (PhEnumHashtable(processHashtable, (PVOID *)&entry, &i)){
			if (entry)
			{
				PPH_STRING buf = (PPH_STRING)entry->Value;
				if(buf){
					RtlSecureZeroMemory(buf->Buffer, buf->Length);
					PhDereferenceObject(buf);
				}
			}
			}
			

			PhDereferenceObject(processHashtable);
	prg_wnd.hide();

	}
	
	

	//assert(file_name && file_name[0] && GetFileAttributes(file_name) != INVALID_FILE_ATTRIBUTES);
	
	_file_name = file_name;
	
	  if (plain_view.size()==0) {
			const wchar_t* err_msg[] = { _PSI.GetMsg(&_FPG, ps_title), _PSI.GetMsg(&_FPG, ps_handles_not_found), _file_name };
			_PSI.Message(&_FPG, &_FPG, FMSG_MB_OK, nullptr, err_msg, sizeof(err_msg) / sizeof(err_msg[0]), 0);
			//return;
			goto clean;
		}

		uint16_t idx = 0;
		if (!show_list(plain_view, idx))
			goto clean;


		ULONG j;

		if (idx!=UINT16_MAX)
		{
			j = idx;	
		}else
		{
			j = 0;
		}

			for(i=j; i < plain_view.size(); i++){
				if(i>j && idx!=UINT16_MAX) break;
				if(!plain_view[i].isModule){
						NTSTATUS status;
                        HANDLE processHandle;
						if (NT_SUCCESS(status = PhOpenProcess(&processHandle,PROCESS_DUP_HANDLE,plain_view[i].processId)))
                        {
                                if (NT_SUCCESS(status = PhDuplicateObject(processHandle,plain_view[i].lockHandel,NULL,NULL,0,0,DUPLICATE_CLOSE_SOURCE)))
                                {
									
								}
								else 
								{
									show_error();
								}
						}
						NtClose(processHandle);
				}else{
			NTSTATUS status;	
			HANDLE processHandle;
			switch (plain_view[i].modType)
			{
				case PH_MODULE_TYPE_MODULE:
				case PH_MODULE_TYPE_WOW64_MODULE:

				if (NT_SUCCESS(status = PhOpenProcess(&processHandle,ProcessQueryAccess | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |PROCESS_VM_READ | PROCESS_VM_WRITE,plain_view[i].processId)))
				{
					LARGE_INTEGER timeout;
					timeout.QuadPart = -5 * PH_TIMEOUT_SEC;
					status = PhUnloadDllProcess(processHandle,plain_view[i].lockHandel,&timeout);
					NtClose(processHandle);
				}
				if (status == STATUS_DLL_NOT_FOUND)
				{
					//err
		        }

				if (!NT_SUCCESS(status))
				{
					show_error();
				}

        break;

    case PH_MODULE_TYPE_KERNEL_MODULE:
        status = PhUnloadDriver(plain_view[i].lockHandel, plain_view[i].modName->Buffer);

        if (!NT_SUCCESS(status))
        {
			show_error();
        }

        break;

    case PH_MODULE_TYPE_MAPPED_FILE:
    case PH_MODULE_TYPE_MAPPED_IMAGE:
        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_VM_OPERATION,
            plain_view[i].processId
            )))
        {
            status = NtUnmapViewOfSection(processHandle, plain_view[i].lockHandel);
            NtClose(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
		show_error();
		}

        break;
    }


				}
			}

clean:
for(i=0; i < plain_view.size(); i++){
	if(plain_view[i].isModule && plain_view[i].modName){
		PhDereferenceObject(plain_view[i].modName);
	}
}

PhDereferenceObject(SearchString);

}


bool unlocker::show_list(const vector<lock_info>& plain_view, uint16_t& index) const
{
	assert(!plain_view.empty());

	wstring title;
	//title += L": ";
	
	const wstring pin(_file_name);
	const size_t pos = pin.rfind(L'\\');
	if (pos != string::npos)
		title = pin.substr(pos + 1);
	else
		title = _PSI.GetMsg(&_FPG, ps_title);

	size_t max_len = 0;

	
	vector<FarListItem> far_items;
	far_items.resize(plain_view.size());
	ZeroMemory(&far_items.front(), sizeof(FarListItem) * far_items.size());
	for (size_t i = 0; i < plain_view.size(); ++i) {
		far_items[i].Text = plain_view[i].label.c_str();
		
		//Calculate max length (for window size)
		if (plain_view[i].label.length() > max_len)
			max_len = plain_view[i].label.length();
	}
	far_items[0].Flags |= LIF_SELECTED;
	FarList far_list;
	ZeroMemory(&far_list, sizeof(far_list));
	far_list.StructSize = sizeof(far_list);
	far_list.ItemsNumber = far_items.size();
	far_list.Items = &far_items.front();

	//Calculate window size
	SMALL_RECT rc_far_wnd;
	_PSI.AdvControl(&_FPG, ACTL_GETFARRECT, 0, &rc_far_wnd);
	intptr_t wnd_width = static_cast<intptr_t>(max_len) + 12 /* borders */;
	const intptr_t max_wnd_width = (rc_far_wnd.Right + 1) - 2;
	const intptr_t min_wnd_width = 60;
	if (wnd_width > max_wnd_width)
		wnd_width = max_wnd_width;
	if (wnd_width < min_wnd_width)
		wnd_width = min_wnd_width;
	intptr_t wnd_height = static_cast<intptr_t>(far_items.size() + 6);
	const intptr_t max_wnd_height = (rc_far_wnd.Bottom - rc_far_wnd.Top + 1) - 2;
	if (wnd_height > max_wnd_height)
		wnd_height = max_wnd_height;

	//Build dialog window
	const FarDialogItem dlg_items[] = {
		/* 0 */ { DI_DOUBLEBOX, 3, 1, wnd_width - 4, wnd_height - 2, 0, nullptr, nullptr, LIF_NONE, title.c_str() },
		/* 1 */ { DI_LISTBOX,   5, 2, wnd_width - 6, wnd_height - 5, reinterpret_cast<intptr_t>(&far_list), nullptr, nullptr, DIF_LISTNOBOX | DIF_LISTNOAMPERSAND | DIF_LISTWRAPMODE },
		/* 2 */ { DI_TEXT,      0, wnd_height - 4, 0, wnd_height - 4, 0, nullptr, nullptr, DIF_SEPARATOR },
		/* 3 */ { DI_BUTTON,    0, wnd_height - 3, 0, wnd_height - 3, 0, nullptr, nullptr, DIF_CENTERGROUP | 0, _PSI.GetMsg(&_FPG, ps_close_all) },
		/* 4 */ { DI_BUTTON,    0, wnd_height - 3, 0, wnd_height - 3, 0, nullptr, nullptr, DIF_CENTERGROUP | DIF_DEFAULTBUTTON, _PSI.GetMsg(&_FPG, ps_close_one) },
		/* 5 */ { DI_BUTTON,    0, wnd_height - 3, 0, wnd_height - 3, 0, nullptr, nullptr, DIF_CENTERGROUP, _PSI.GetMsg(&_FPG, ps_cancel) }
	};

	const HANDLE dlg = _PSI.DialogInit(&_FPG, &_FPG, -1, -1, wnd_width, wnd_height, nullptr, dlg_items, sizeof(dlg_items) / sizeof(dlg_items[0]), 0, FDLG_NONE, nullptr, nullptr);
	const intptr_t rcd = _PSI.DialogRun(dlg);
	const bool rc = rcd >= 0 && rcd != 5 /* cancel */;
	if (rc) {
		if (rcd == 3)
			index = UINT16_MAX;
		else
			index = static_cast<uint16_t>(_PSI.SendDlgMessage(dlg, DM_LISTGETCURPOS, 1, nullptr));
	}
	_PSI.DialogFree(dlg);

	return rc;
}


void unlocker::show_error(const DWORD err_code /*= 0*/) const
{
	if (err_code)
		SetLastError(err_code);
	const wchar_t* err_msg[] = { _PSI.GetMsg(&_FPG, ps_title), _PSI.GetMsg(&_FPG, ps_some_error) };
	_PSI.Message(&_FPG, &_FPG, FMSG_WARNING | FMSG_ERRORTYPE | FMSG_MB_OK, nullptr, err_msg, sizeof(err_msg) / sizeof(err_msg[0]), 0);
}
