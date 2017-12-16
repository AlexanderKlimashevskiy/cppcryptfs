/*
cppcryptfs : user-mode cryptographic virtual overlay filesystem.

Copyright (C) 2016-2017 Bailey Brown (github.com/bailey27/cppcryptfs)

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

// SettingsPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "cppcryptfs.h"
#include "SettingsPropertyPage.h"
#include "afxdialogex.h"
#include "cppcryptfs.h"
#include "ui/cryptdefaults.h"
#include "util/savedpasswords.h"

// CSettingsPropertyPage dialog

IMPLEMENT_DYNAMIC(CSettingsPropertyPage, CCryptPropertyPage)

CSettingsPropertyPage::CSettingsPropertyPage()
	: CCryptPropertyPage(IDD_SETTINGS)
{
	m_bCaseInsensitive = false;
	m_bMountManager = false;
	m_bEnableSavingPasswords = false;
	m_bUseImpersonation = false;
}

CSettingsPropertyPage::~CSettingsPropertyPage()
{
}

void CSettingsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSettingsPropertyPage, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_THREADS, &CSettingsPropertyPage::OnSelchangeThreads)
	ON_CBN_SELCHANGE(IDC_BUFFERSIZE, &CSettingsPropertyPage::OnSelchangeBuffersize)
	ON_BN_CLICKED(IDC_CASEINSENSITIVE, &CSettingsPropertyPage::OnBnClickedCaseinsensitive)
	ON_CBN_SELCHANGE(IDC_CACHETTL, &CSettingsPropertyPage::OnCbnSelchangeCachettl)
	ON_BN_CLICKED(IDC_DEFAULTS, &CSettingsPropertyPage::OnBnClickedDefaults)
	ON_BN_CLICKED(IDC_RECOMMENDED, &CSettingsPropertyPage::OnBnClickedRecommended)
	ON_BN_CLICKED(IDC_MOUNTMANAGER, &CSettingsPropertyPage::OnClickedMountmanager)
	ON_BN_CLICKED(IDC_RESETWARNINGS, &CSettingsPropertyPage::OnClickedResetwarnings)
	ON_BN_CLICKED(IDC_ENABLE_SAVING_PASSWORDS, &CSettingsPropertyPage::OnClickedEnableSavingPasswords)
	ON_BN_CLICKED(IDC_USE_IMPERSONATION, &CSettingsPropertyPage::OnClickedUseImpersonation)
END_MESSAGE_MAP()


// CSettingsPropertyPage message handlers

static int buffer_sizes[] = { 4, 8, 16, 32, 64, 128, 256, 512, 1024 };

static int ttls[] = { 0, 1, 2, 5, 10, 15, 30, 45, 60, 90, 120, 300, 600, 900, 1800, 3600};

static const WCHAR* ttl_strings[] = { L"infinite", L"1 second", L"2 seconds", L"5 seconds", L"10 seconds", L"15 seconds", L"30 seconds", L"45 seconds", L"60 seconds", L"90 seconds", L"2 minutes", L"5 minutes", L"10 minutes", L"15 minutes", L"30 minutes", L"1 hour" };

BOOL CSettingsPropertyPage::OnInitDialog()
{
	CCryptPropertyPage::OnInitDialog();

	// TODO:  Add extra initialization here

	int nThreads = theApp.GetProfileInt(L"Settings", L"Threads", PER_FILESYSTEM_THREADS_DEFAULT);

	int bufferblocks = theApp.GetProfileInt(L"Settings", L"BufferBlocks", BUFFERBLOCKS_DEFAULT);

	int cachettl = theApp.GetProfileInt(L"Settings", L"CacheTTL", CACHETTL_DEFAULT);

	bool bCaseInsensitive = theApp.GetProfileInt(L"Settings", L"CaseInsensitive", CASEINSENSITIVE_DEFAULT) != 0;

	bool bMountManager = theApp.GetProfileInt(L"Settings", L"MountManager", MOUNTMANAGER_DEFAULT) != 0;

	bool bEnableSavingPasswords = theApp.GetProfileInt(L"Settings", L"EnableSavingPasswords", ENABLE_SAVING_PASSWORDS_DEFAULT) != 0;

	bool bUseImpersonation = theApp.GetProfileInt(L"Settings", L"UseImpersonation", USE_IMPERSONATION_DEFAULT) != 0;

	return SetControls(nThreads, bufferblocks, cachettl, bCaseInsensitive, bMountManager, bEnableSavingPasswords, bUseImpersonation);
}


BOOL CSettingsPropertyPage::SetControls(int nThreads, int bufferblocks, int cachettl, bool bCaseInsensitive, bool bMountManager, bool bEnableSavingPasswords, bool bUseImpersonation)
{

	m_bCaseInsensitive =  bCaseInsensitive;
	m_bMountManager = bMountManager;
	m_bEnableSavingPasswords = bEnableSavingPasswords;
	m_bUseImpersonation = bUseImpersonation;

	int i;

	CComboBox *pBox = (CComboBox*)GetDlgItem(IDC_THREADS);

	if (!pBox)
		return FALSE;

	pBox->ResetContent();

	WCHAR buf[80];

	for (i = 0; i < 15; i++) {
		if (i == 0) {
			pBox->AddString(L"Dokany default (5)");
		} else if (i == 1) {
			pBox->AddString(L"1 (cppcryptfs default)");
		} else {
			swprintf_s(buf, L"%d", i);
			pBox->AddString(buf);
		}
	}

	pBox->SetCurSel(nThreads);

	pBox = (CComboBox*)GetDlgItem(IDC_BUFFERSIZE);

	if (!pBox)
		return FALSE;

	pBox->ResetContent();

	for (i = 0; i < sizeof(buffer_sizes)/sizeof(buffer_sizes[0]); i++) {
		swprintf_s(buf, L"%d", buffer_sizes[i]);
		pBox->AddString(buf);
	}

	int bits = 0;

	int n = bufferblocks;
	while (n) {
		bits++;
		n >>= 1;
	}

	pBox->SetCurSel(bits-1);

	pBox = (CComboBox*)GetDlgItem(IDC_CACHETTL);

	if (!pBox)
		return FALSE;

	pBox->ResetContent();

	static_assert(sizeof(ttls) / sizeof(ttls[0]) == sizeof(ttl_strings) / sizeof(ttl_strings[0]), "mismatch in sizes of ttls/ttl_strings");

	int selitem = 0;

	for (i = 0; i < sizeof(ttls) / sizeof(ttls[0]); i++) {
		pBox->AddString(ttl_strings[i]);
		if (cachettl == ttls[i]) {
			selitem = i;
		}
	}

	pBox->SetCurSel(selitem);

	CheckDlgButton(IDC_CASEINSENSITIVE, m_bCaseInsensitive ? 1 : 0);

	CheckDlgButton(IDC_MOUNTMANAGER, m_bMountManager ? 1 : 0);

	CheckDlgButton(IDC_ENABLE_SAVING_PASSWORDS, m_bEnableSavingPasswords ? 1 : 0);

	CheckDlgButton(IDC_USE_IMPERSONATION, m_bUseImpersonation ? 1 : 0);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}


void CSettingsPropertyPage::OnSelchangeThreads()
{
	// TODO: Add your control notification handler code here

	CComboBox *pBox = (CComboBox*)GetDlgItem(IDC_THREADS);

	if (!pBox)
		return;

	int nThreads = pBox->GetCurSel();

	theApp.WriteProfileInt(L"Settings", L"Threads", nThreads);
}


void CSettingsPropertyPage::OnSelchangeBuffersize()
{
	// TODO: Add your control notification handler code here

	CComboBox *pBox = (CComboBox*)GetDlgItem(IDC_BUFFERSIZE);

	if (!pBox)
		return;

	int selIndex = pBox->GetCurSel();

	int nBlocks = 1 << selIndex;

	theApp.WriteProfileInt(L"Settings",  L"BufferBlocks", nBlocks);
}

void CSettingsPropertyPage::OnCbnSelchangeCachettl()
{
	// TODO: Add your control notification handler code here

	CComboBox *pBox = (CComboBox*)GetDlgItem(IDC_CACHETTL);

	if (!pBox)
		return;

	int selIndex = pBox->GetCurSel();

	int cachettl = ttls[selIndex];

	theApp.WriteProfileInt(L"Settings",  L"CacheTTL", cachettl);
}


void CSettingsPropertyPage::OnBnClickedCaseinsensitive()
{
	// TODO: Add your control notification handler code here

	m_bCaseInsensitive = !m_bCaseInsensitive;

	CheckDlgButton(IDC_CASEINSENSITIVE, m_bCaseInsensitive ? 1 : 0);

	theApp.WriteProfileInt(L"Settings", L"CaseInsensitive", m_bCaseInsensitive ? 1 : 0);
}


void CSettingsPropertyPage::SaveSettings()
{
	OnSelchangeThreads();
	OnSelchangeBuffersize();
	OnCbnSelchangeCachettl();

	m_bCaseInsensitive = !m_bCaseInsensitive; // OnBnClickedCaseinsensitive() flips it
	m_bMountManager = !m_bMountManager; // ditto
	m_bEnableSavingPasswords = !m_bEnableSavingPasswords; // ditto
	m_bUseImpersonation = !m_bUseImpersonation; // ditto

	OnBnClickedCaseinsensitive();
	OnClickedMountmanager();
	OnClickedEnableSavingPasswords();
	OnClickedUseImpersonation();
}

void CSettingsPropertyPage::OnBnClickedDefaults()
{
	// TODO: Add your control notification handler code here

	SetControls(PER_FILESYSTEM_THREADS_DEFAULT, BUFFERBLOCKS_DEFAULT, CACHETTL_DEFAULT, CASEINSENSITIVE_DEFAULT, MOUNTMANAGER_DEFAULT, ENABLE_SAVING_PASSWORDS_DEFAULT, USE_IMPERSONATION_DEFAULT);

	SaveSettings();
}


void CSettingsPropertyPage::OnBnClickedRecommended()
{
	// TODO: Add your control notification handler code here

	SetControls(PER_FILESYSTEM_THREADS_RECOMMENDED, BUFFERBLOCKS_RECOMMENDED, CACHETTL_RECOMMENDED, CASEINSENSITIVE_RECOMMENDED, MOUNTMANAGER_RECOMMENDED, ENABLE_SAVING_PASSWORDS_RECOMMENDED, USE_IMPERSONATION_RECOMMENDED);

	SaveSettings();
}



void CSettingsPropertyPage::OnClickedMountmanager()
{
	// TODO: Add your control notification handler code here

	m_bMountManager = !m_bMountManager;

	CheckDlgButton(IDC_MOUNTMANAGER, m_bMountManager ? 1 : 0);

	theApp.WriteProfileInt(L"Settings", L"MountManager", m_bMountManager ? 1 : 0);
}


void CSettingsPropertyPage::OnClickedResetwarnings()
{
	// TODO: Add your control notification handler code here

	theApp.WriteProfileInt(L"Settings", L"MountManagerWarn", MOUNTMANAGERWARN_DEFAULT);
}


void CSettingsPropertyPage::OnClickedEnableSavingPasswords()
{
	// TODO: Add your control notification handler code here

	m_bEnableSavingPasswords = !m_bEnableSavingPasswords;

	CheckDlgButton(IDC_ENABLE_SAVING_PASSWORDS, m_bEnableSavingPasswords ? 1 : 0);

	if (m_bEnableSavingPasswords) {
		theApp.WriteProfileInt(L"Settings", L"EnableSavingPasswords", TRUE);
	} else {
		theApp.WriteProfileInt(L"Settings", L"EnableSavingPasswords", FALSE);
		int numSavedPasswords = SavedPasswords::ClearSavedPasswords(FALSE);
		if (numSavedPasswords < 0) {
			MessageBox(L"unable to count saved passwords", L"cppcryptfs", MB_ICONEXCLAMATION | MB_OK);
		} else if (numSavedPasswords > 0) {
			int result = MessageBox(L"Delete all saved passwords?", L"cppcryptfs", MB_ICONWARNING | MB_YESNO);
			if (result == IDYES) {
				if (SavedPasswords::ClearSavedPasswords(TRUE) != numSavedPasswords) {
					MessageBox(L"unable to delete saved passwords", L"cppcryptfs", MB_ICONEXCLAMATION | MB_OK);
				}
			}
		}
	}
}


void CSettingsPropertyPage::OnClickedUseImpersonation()
{
	// TODO: Add your control notification handler code here

	// TODO: Add your control notification handler code here

	m_bUseImpersonation = !m_bUseImpersonation;

	CheckDlgButton(IDC_USE_IMPERSONATION, m_bUseImpersonation ? 1 : 0);

	theApp.WriteProfileInt(L"Settings", L"UseImpersonation", m_bUseImpersonation ? 1 : 0);
}
