/*
cppcryptfs : user-mode cryptographic virtual overlay filesystem.

Copyright (C) 2016-2018 Bailey Brown (github.com/bailey27/cppcryptfs)

cppcryptfs is based on the design of gocryptfs (github.com/rfjakob/gocryptfs)

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "stdafx.h"

#include "winutil.h"

#include "ui/MountMangerDialog.h"
#include  <tlhelp32.h>

DWORD
getppid()
{
	HANDLE hSnapshot;
	PROCESSENTRY32 pe32;
	DWORD ppid = 0, pid = GetCurrentProcessId();

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	__try {
		if (hSnapshot == INVALID_HANDLE_VALUE) __leave;

		ZeroMemory(&pe32, sizeof(pe32));
		pe32.dwSize = sizeof(pe32);
		if (!Process32First(hSnapshot, &pe32)) __leave;

		do {
			if (pe32.th32ProcessID == pid) {
				ppid = pe32.th32ParentProcessID;
				break;
			}
		} while (Process32Next(hSnapshot, &pe32));

	} __finally {
		if (hSnapshot != INVALID_HANDLE_VALUE) CloseHandle(hSnapshot);
	}
	return ppid;
}


bool
have_args()
{
	int argc = 1;

	LPCWSTR cmdLine = GetCommandLineW();

	LPTSTR *argv = NULL;

	if (cmdLine)
		argv = CommandLineToArgvW(cmdLine, &argc);

	if (argv)
		LocalFree(argv);
	else
		argc = 1;

	return argc > 1;
}

void
OpenConsole(DWORD pid)
{
	FreeConsole();

	if (AttachConsole(pid ? pid : ATTACH_PARENT_PROCESS)) {
#pragma warning( push )
#pragma warning(disable : 4996)
		freopen("CONOUT$", "wt", stdout);
		freopen("CONOUT$", "wt", stderr);
#pragma warning( pop )
	}
}

void
CloseConsole()
{
	fclose(stderr);
	fclose(stdout);

	FreeConsole();
}

void
ConsoleErrMes(LPCWSTR err, DWORD pid)
{
	OpenConsole(pid);
	fwprintf(stderr, L"cppcryptfs: %s\n", err);
	CloseConsole();
}


static bool
GetProductVersionInfo(CString& strProductName, CString& strProductVersion,
	CString& strLegalCopyright, HMODULE hMod)
{

	TCHAR fullPath[MAX_PATH + 1];
	*fullPath = L'\0';
	if (!GetModuleFileName(hMod, fullPath, MAX_PATH)) {
		return false;
	}
	DWORD dummy = 0;
	DWORD vSize = GetFileVersionInfoSize(fullPath, &dummy);
	if (vSize < 1) {
		return false;
	}

	void *pVersionResource = NULL;

	pVersionResource = malloc(vSize);

	if (pVersionResource == NULL) {
		return false;
	}

	if (!GetFileVersionInfo(fullPath, NULL, vSize, pVersionResource)) {
		free(pVersionResource);
		return false;
	}

	// get the name and version strings
	LPVOID pvProductName = NULL;
	unsigned int iProductNameLen = 0;
	LPVOID pvProductVersion = NULL;
	unsigned int iProductVersionLen = 0;
	LPVOID pvLegalCopyright = NULL;
	unsigned int iLegalCopyrightLen = 0;

	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate;

	// Read the list of languages and code pages.
	unsigned int cbTranslate;
	if (!VerQueryValue(pVersionResource,
		TEXT("\\VarFileInfo\\Translation"),
		(LPVOID*)&lpTranslate,
		&cbTranslate)) {

		return false;
	}

	if (cbTranslate / sizeof(struct LANGANDCODEPAGE) < 1) {
		return false;
	}

	CString lang;

	WCHAR buf[16];

	// use the first language/codepage;

	wsprintf(buf, L"%04x%04x", lpTranslate->wLanguage, lpTranslate->wCodePage);

	lang = buf;

	// replace "040904e4" with the language ID of your resources
	if (!VerQueryValue(pVersionResource, L"\\StringFileInfo\\" + lang + L"\\ProductName", &pvProductName, &iProductNameLen) ||
		!VerQueryValue(pVersionResource, L"\\StringFileInfo\\" + lang + "\\ProductVersion", &pvProductVersion, &iProductVersionLen) ||
		!VerQueryValue(pVersionResource, L"\\StringFileInfo\\" + lang + "\\LegalCopyright", &pvLegalCopyright, &iLegalCopyrightLen)) {
		free(pVersionResource);
		return false;
	}

	if (iProductNameLen < 1 || iProductVersionLen < 1 || iLegalCopyrightLen < 1) {
		free(pVersionResource);
		return false;
	}

	strProductName.SetString((LPCTSTR)pvProductName, iProductNameLen - 1);
	strProductVersion.SetString((LPCTSTR)pvProductVersion, iProductVersionLen - 1);
	strLegalCopyright.SetString((LPCTSTR)pvLegalCopyright, iLegalCopyrightLen - 1);

	free(pVersionResource);

	return true;
}

bool
GetProductVersionInfo(wstring& strProductName, wstring& strProductVersion,
	wstring& strLegalCopyright, HMODULE hMod)
{
	CString cName, cVer, cCop;

	if (GetProductVersionInfo(cName, cVer, cCop, hMod)) {
		strProductName = cName;
		strProductVersion = cVer;
		strLegalCopyright = cCop;
		return true;
	} else {
		return false;
	}
}

bool mountmanager_continue_mounting()
{
	CMountMangerDialog mdlg;

	mdlg.DoModal();

	return mdlg.m_bOkPressed != 0;
}
