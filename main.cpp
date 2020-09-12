#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <filesystem>

#include <strsafe.h>
#include <windows.h>
#include <Dbghelp.h>

namespace fs=std::filesystem;

typedef std::map<std::string, std::string> DepMapType;

const DepMapType getDependencies(const HMODULE hMod)
{
	DepMapType depsMap;

	IMAGE_DOS_HEADER* pDosHeader = (IMAGE_DOS_HEADER*)hMod;
	IMAGE_OPTIONAL_HEADER* pOptHeader = (IMAGE_OPTIONAL_HEADER*)((BYTE*)hMod + pDosHeader->e_lfanew + 24);
	IMAGE_IMPORT_DESCRIPTOR* pImportDesc = (IMAGE_IMPORT_DESCRIPTOR*)((BYTE*)hMod + pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	while (pImportDesc->FirstThunk) {
		char* dllName = (char*)((BYTE*)hMod + pImportDesc->Name);

		std::string dllPath = "Can't Find!!!";
		HMODULE hModDep = ::LoadLibraryEx(dllName, NULL, DONT_RESOLVE_DLL_REFERENCES);
		if (hModDep != 0) {
			LPTSTR  strDLLPath = new TCHAR[_MAX_PATH];
			::GetModuleFileName(hModDep, strDLLPath, _MAX_PATH);
			dllPath = std::string(strDLLPath);
			delete strDLLPath;
		}
		FreeLibrary(hModDep);
		depsMap[std::string(dllName)] = dllPath;
		pImportDesc++;
	}
	return depsMap;
}

int printDependencies(const std::string libPath)
{	
	fs::path dll_path = fs::absolute(libPath);
	std::string dir = dll_path.parent_path().string();
	SetCurrentDirectory(dir.c_str());

	HMODULE hMod = ::LoadLibraryEx(libPath.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (hMod == 0) {		 
		// Retrieves the last error message.
		DWORD lastError = GetLastError();
		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
					  nullptr, lastError, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, nullptr);
		lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + 40) * sizeof(TCHAR));
		StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
						TEXT("failed with error code:[%d]: %s"), lastError, lpMsgBuf);
		std::cerr << (LPTSTR)lpDisplayBuf << std::endl;
		return -1;
	}
	const DepMapType& depMap = getDependencies(hMod);
	FreeLibrary(hMod);
	DepMapType::const_iterator iter;
	for (iter = depMap.begin(); iter != depMap.end(); ++iter) {
		std::cout << "\t" << iter->first << " => " << iter->second << std::endl;
	}

	return 0;
}

int printUsage()
{
	std::cout << "ldd for Windows, Version 1.0" << std::endl;
	std::cout << "usage:\n ldd FILE..." << std::endl;
	return -1;
}

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		return printUsage();
	}
	int res = 0;
	for (int i = 1; i < argc; ++i) {
		std::cout << argv[i] << std::endl;
		res = printDependencies(argv[i]);
	}
	return res;
}
