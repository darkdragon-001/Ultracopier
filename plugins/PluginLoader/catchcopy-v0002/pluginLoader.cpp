/** \file pluginLoader.cpp
\brief Define the session plugin loader test
\author alpha_one_x86
\version 0.3
\date 2010 */

#include "pluginLoader.h"
#include "PlatformMacro.h"

#ifdef ULTRACOPIER_PLUGIN_DEBUG
	#define NORMAL_EXT "d.dll"
	#define SECOND_EXT ".dll"
#else
	#define NORMAL_EXT ".dll"
	#define SECOND_EXT "d.dll"
#endif
#define CATCHCOPY_DLL_32 "catchcopy32"
#define CATCHCOPY_DLL_64 "catchcopy64"

PluginLoader::PluginLoader()
{
	//set the startup value into the variable
	dllChecked=false;
	
	needBeRegistred=false;
}

PluginLoader::~PluginLoader()
{
        ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"destructor");
        setEnabled(false);
}

void PluginLoader::setEnabled(bool needBeRegistred)
{
	if(!checkExistsDll())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("No dll exists"));
		emit newState(Uncaught);
		return;
	}
	if(this->needBeRegistred==needBeRegistred)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Double event dropped"));
		return;
	}
	this->needBeRegistred=needBeRegistred;
	int index=0;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, needBeRegistred: "+QString::number(needBeRegistred));

	bool oneHaveFound=false;
	index=0;
	while(index<importantDll.size())
	{
		if(QFile::exists(pluginPath+importantDll.at(index)))
		{
			oneHaveFound=true;
			break;
		}
		index++;
	}
	if(!oneHaveFound)
	{
		index=0;
		while(index<secondDll.size())
		{
			if(QFile::exists(pluginPath+secondDll.at(index)))
			{
				oneHaveFound=true;
				break;
			}
			index++;
		}
	}
	if(!oneHaveFound)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("No dll have found"));
		emit newState(Uncaught);
		return;
	}

	index=0;
	bool importantDll_is_loaded=false,secondDll_is_loaded=false;
	bool importantDll_have_bug=false,secondDll_have_bug=false;
	int importantDll_count=0,secondDll_count=0;
	while(index<importantDll.size())
	{
		if(!RegisterShellExtDll(pluginPath+importantDll.at(index),needBeRegistred,false))
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"the important dll have failed: "+importantDll.at(index));
			importantDll_have_bug=true;
		}
		else
		{
			importantDll_is_loaded=true;
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"the important dll have been loaded: "+importantDll.at(index));
		}
		importantDll_count++;
		index++;
	}
	index=0;
	while(index<secondDll.size())
	{
		if(!RegisterShellExtDll(pluginPath+secondDll.at(index),needBeRegistred,true))
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"the second dll have failed: "+secondDll.at(index));
			secondDll_have_bug=true;
		}
		else
		{
			secondDll_is_loaded=true;
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"the second dll have been loaded: "+secondDll.at(index));
		}
		secondDll_count++;
		index++;
	}

	CatchState importantDll_state,secondDll_state;
	if(importantDll_count==0)
	{
		if(needBeRegistred)
			importantDll_state=Caught;
		else
			importantDll_state=Uncaught;
	}
	else
	{
		if(importantDll_is_loaded)
		{
			if(!importantDll_have_bug)
				importantDll_state=Caught;
			else
				importantDll_state=Semiuncaught;
		}
		else
			importantDll_state=Uncaught;
	}
	if(secondDll_count==0)
		if(needBeRegistred)
			secondDll_state=Caught;
		else
			secondDll_state=Uncaught;
	else
	{
		if(secondDll_is_loaded)
		{
			if(!secondDll_have_bug)
				secondDll_state=Caught;
			else
				secondDll_state=Semiuncaught;
		}
		else
			secondDll_state=Uncaught;
	}

	if((importantDll_state==Uncaught && secondDll_state==Uncaught) || !needBeRegistred || (importantDll_count==0 && secondDll_count==0))
		emit newState(Uncaught);
	else if(importantDll_state==Caught)
		emit newState(Caught);
	else
		emit newState(Semiuncaught);
}

bool PluginLoader::checkExistsDll()
{
	if(dllChecked)
	{
		if(importantDll.size()>0 || secondDll.size()>0)
			return true;
		else
			return false;
	}
	dllChecked=true;
	
	#if defined(ULTRACOPIER_VERSION_PORTABLE) || ! defined(_M_X64)
	bool is64Bits=false;
	char *arch=getenv("windir");
	if(arch!=NULL)
	{
		QDir dir;
		if(dir.exists(QString(arch)+"\\SysWOW64\\"))
		{
			is64Bits=true;
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"OS seam 64Bits, "+QString(arch)+"\\SysWOW64\\");
		}
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"OS seam not 64Bits, "+QString(arch)+"\\SysWOW64\\");
		/// \note commented because it do a crash at the startup
		//delete arch;
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to get env var");
	
	if(!is64Bits)
	{
		if((importantDll.size()+secondDll.size())>1)
		{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Not load 64Bits dll");
		importantDll.removeOne(CATCHCOPY_DLL_64);
		secondDll.removeOne(CATCHCOPY_DLL_64);
		}
	}
	else
	{
		QStringList tempList=importantDll;
		importantDll=secondDll;
		secondDll=tempList;
	}
	#endif

	int index=0;
	while(index<importantDll.size())
	{
		if(!QFile::exists(pluginPath+importantDll.at(index)+NORMAL_EXT))
		{
			if(!QFile::exists(pluginPath+importantDll.at(index)+SECOND_EXT))
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"file not found, drop to the list: "+importantDll.at(index));
				importantDll.removeAt(index);
				index--;
			}
			else
				importantDll[index]+=SECOND_EXT;
		}
		else
			importantDll[index]+=NORMAL_EXT;
		index++;
	}
	index=0;
	while(index<secondDll.size())
	{
		if(!QFile::exists(pluginPath+secondDll.at(index)+NORMAL_EXT))
		{
			if(!QFile::exists(pluginPath+secondDll.at(index)+SECOND_EXT))
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"file not found, drop to the list: "+secondDll.at(index));
				secondDll.removeAt(index);
				index--;
			}
			else
				secondDll[index]+=SECOND_EXT;
		}
		else
			secondDll[index]+=NORMAL_EXT;
		index++;
	}
	if(importantDll.size()>0 || secondDll.size()>0)
		return true;
	else
		return false;
}

void PluginLoader::setResources(OptionInterface * options,QString writePath,QString pluginPath,bool portableVersion)
{
	Q_UNUSED(options);
        this->pluginPath=pluginPath;
	if(portableVersion)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("version portable detected"));
		secondDll << CATCHCOPY_DLL_32 << CATCHCOPY_DLL_64;
	}
	else
	{
		#if defined(_M_X64)//64Bits
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("64Bits version detected"));
			importantDll << CATCHCOPY_DLL_64;
			secondDll << CATCHCOPY_DLL_32;
		#else//32Bits
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("32Bits version detected"));
			importantDll << CATCHCOPY_DLL_32;
			secondDll << CATCHCOPY_DLL_64;
		#endif
	}
}

bool PluginLoader::RegisterShellExtDll(QString dllPath, bool bRegister,bool quiet)
{
	////////////////////////////// First way to load //////////////////////////////
	
	wchar_t arrayArg[65535];
	int size_lenght;
	// first try - load dll and register it manually.
	HRESULT hResult = S_OK;
	// if failed - try by loading extension manually (would fail on vista when running as user)
	hResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if(SUCCEEDED(hResult))
	{
		HRESULT (STDAPICALLTYPE *pfn)(void);
		size_lenght=dllPath.toWCharArray(arrayArg);
		HINSTANCE hMod = LoadLibrary(arrayArg);	// load the dll
		if(hMod == NULL)
			hResult = HRESULT_FROM_WIN32(GetLastError());
		if(SUCCEEDED(hResult) && !hMod)
			hResult = E_FAIL;
		if(SUCCEEDED(hResult))
		{
			(FARPROC&)pfn = GetProcAddress(hMod, (bRegister ? "DllRegisterServer" : "DllUnregisterServer"));
			if(pfn == NULL)
				hResult = E_FAIL;
			if(SUCCEEDED(hResult))
				hResult = (*pfn)();

			CoFreeLibrary(hMod);
		}
		CoUninitialize();
		if(SUCCEEDED(hResult) && SCODE_CODE(hResult) != ERROR_ACCESS_DENIED)
			return true;
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("fail by LoadLibrary: %1, error code: %2").arg(dllPath).arg((quint32)hResult));
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("fail by CoInitializeEx: %1, error code: %2").arg(dllPath).arg((quint32)hResult));
	
	////////////////////////////// Second way to load //////////////////////////////
	QStringList arguments;
	arguments.append("/s");
	if(!bRegister)
		arguments.append("/u");
	arguments.append(dllPath);
	QString argumentsString;
	for (int i = 0; i < arguments.size(); ++i) {
		if(argumentsString.isEmpty())
			argumentsString+=arguments.at(i);
		else
			if(i == arguments.size())
				argumentsString+=" \""+arguments.at(i)+"\"";
			else
				argumentsString+=' '+arguments.at(i);
	}
        ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: regsvr32 "+argumentsString);
	int result=QProcess::execute("regsvr32",arguments);
	bool ok=false;
	if(result==0)
		ok=true;
	if(result==5)
	{
		if(!quiet)
		{
			////////////////////////////// Last way to load //////////////////////////////
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"try it in win32");
			// try with regsvr32, win32 because for admin dialog

			size_lenght=argumentsString.toWCharArray(arrayArg);
			//size_lenght*sizeof(wchar_t)
			wcscpy(arrayArg+size_lenght*sizeof(wchar_t),TEXT("\0"));
			SHELLEXECUTEINFO sei;
			memset(&sei, 0, sizeof(sei));
			sei.cbSize = sizeof(sei);
			sei.fMask = SEE_MASK_UNICODE;
			sei.lpVerb = TEXT("runas");
			sei.lpFile = TEXT("regsvr32.exe");
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"in win32 mode: arrayArg: "+QString::fromWCharArray(arrayArg,size_lenght));
			sei.lpParameters = arrayArg;
			sei.nShow = SW_SHOW;
			ok=ShellExecuteEx(&sei);
		}
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"not try because need be quiet: "+dllPath);
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("regsvr32 terminated with: %1").arg(result));
	if(!bRegister)
		HardUnloadDLL(dllPath);
	return ok;
}

Q_EXPORT_PLUGIN2(pluginLoader, PluginLoader);

bool WINAPI PluginLoader::DLLEjecteurW(DWORD dwPid,PWSTR szDLLPath)
{
	/* Search address of module */
	MODULEENTRY32W meModule;
	meModule.dwSize = sizeof(meModule);
	HANDLE hSnapshot = NULL;
	
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);
	if(hSnapshot == NULL)
		return false;
	
	/* Search the right modules of the process */
	Module32FirstW(hSnapshot, &meModule);
	do{
		if((lstrcmpiW(meModule.szModule,szDLLPath) == 0) || (lstrcmpiW(meModule.szExePath,szDLLPath) == 0))break;
	}while(Module32NextW(hSnapshot, &meModule));
	
	/* Get handle of the process */
	HANDLE hProcess;
	
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, false,dwPid);
	if(hProcess == NULL)
	{
		CloseHandle(hSnapshot);
		return false;
	}
	
	LPTHREAD_START_ROUTINE lpthThreadFunction;
	/* Get addresse of FreeLibrary in kernel32.dll */
	lpthThreadFunction = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "FreeLibrary");
	if(lpthThreadFunction == NULL)
	{
		CloseHandle(hProcess);
		CloseHandle(hSnapshot);
		return false;
	}	
	
	/* Creation the remote thread */
	DWORD dwThreadID = 0;
	HANDLE hThread = NULL;
	hThread = CreateRemoteThread(hProcess, NULL, 0, lpthThreadFunction,meModule.modBaseAddr, 0, &dwThreadID);
	if(hThread == NULL)
	{
		CloseHandle(hSnapshot);
		CloseHandle(hProcess);
		return false;
	}
	
	WaitForSingleObject(hThread,INFINITE);
	
	CloseHandle(hProcess);
	CloseHandle(hThread);
	
	return true;
}

void PluginLoader::HardUnloadDLL(QString myDllName)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+myDllName);
	HANDLE hSnapShot1;
	PROCESSENTRY32 uProcess;
	HANDLE hSnapShot2;
	MODULEENTRY32 me32;
	QString DllLoaded = "";
	QString DllLoadedName = "";
	QString DllLoadedPath = "";
	bool bResult;
	bool r;
	short NbProcess;
	NbProcess=0;

	hSnapShot1 = CreateToolhelp32Snapshot(TH32CS_SNAPALL,0);

	uProcess.dwSize = (DWORD) sizeof(PROCESSENTRY32);

	r = Process32First(hSnapShot1, &uProcess);

	while ( r )
	{
		r = Process32Next(hSnapShot1, &uProcess);
		QString myProcessName;
		myProcessName=QString::fromWCharArray(uProcess.szExeFile);
		if (uProcess.th32ProcessID < 99999)
		{
			hSnapShot2 = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, uProcess.th32ProcessID );
			if (hSnapShot2 != INVALID_HANDLE_VALUE)
			{
				me32.dwSize = sizeof(me32);
				bResult = Module32First( hSnapShot2, &me32 );
				while( bResult )
				{
						DllLoaded=QString::fromWCharArray(me32.szExePath);
						DllLoadedName=QString::fromWCharArray(me32.szModule);
						if (DllLoaded == myDllName)
						{
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"The path: "+DllLoaded);
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,myProcessName+" ("+QString::number(uProcess.th32ProcessID)+")");
							DLLEjecteurW(uProcess.th32ProcessID,me32.szExePath);
						}
						bResult = Module32Next( hSnapShot2, &me32 );
				}
			}
			else
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"(int)hSnapShot2 != -1 for "+myProcessName+" ("+QString::number(uProcess.th32ProcessID)+")");
			if(hSnapShot2)
				CloseHandle(hSnapShot2);
		}
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"uProcess.th32ProcessID > 99999 for "+myProcessName+" ("+QString::number(uProcess.th32ProcessID)+")");
	}
	CloseHandle(hSnapShot1);
}
