module;
/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <SDKDDKVer.h>
#include "StrToNum.h"
#include "resource.h"
#include <afxdialogex.h>
#include <Wbemidl.h>
#include <comutil.h>
#include <format>
#include <memory>
#include <string>
#include <vector>
#pragma comment(lib, "wbemuuid.lib") //For Wbemidl.h
export module DlgOpenDevice;

import Utility;
using namespace Utility;


//CDlgOpenDisk.

class CDlgOpenDisk final : public CDialogEx
{
public:
	[[nodiscard]] auto GetPaths() -> std::vector<std::wstring>&;
	void SetIWbemServices(IWbemServices* pWbemServices);
private:
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
	void OnOK()override;
	void OnCancel()override;
	afx_msg void OnListDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP();
	[[nodiscard]] static auto GetPhysicalDisks(IWbemServices* pWbemServices) -> std::vector<Utility::PHYSICALDISK>;
private:
	CListCtrl m_list;
	IWbemServices* m_pWbemServices { };
	std::vector<Utility::PHYSICALDISK> m_vecPhysicalDisks;
	std::vector<std::wstring> m_vecPaths; //Paths to open.
};

BEGIN_MESSAGE_MAP(CDlgOpenDisk, CDialogEx)
	ON_NOTIFY(NM_DBLCLK, IDC_OPENDISK_LIST_DISKS, &CDlgOpenDisk::OnListDblClick)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_OPENDISK_LIST_DISKS, &CDlgOpenDisk::OnListItemChanged)
END_MESSAGE_MAP()

auto CDlgOpenDisk::GetPaths()->std::vector<std::wstring>&
{
	return m_vecPaths;
}

void CDlgOpenDisk::SetIWbemServices(IWbemServices* pWbemServices)
{
	m_pWbemServices = pWbemServices;
}


//CDlgOpenDisk Private methods.

void CDlgOpenDisk::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OPENDISK_LIST_DISKS, m_list);
}

BOOL CDlgOpenDisk::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_list.SetExtendedStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list.InsertColumn(0, L"Disk", 0, 150);
	m_list.InsertColumn(1, L"Size", 0, 70);
	m_list.InsertColumn(2, L"Path", 0, 120);
	m_list.InsertColumn(3, L"Media Type", 0, 80);
	m_list.InsertColumn(4, L"Bus Type", 0, 100);

	m_vecPhysicalDisks = GetPhysicalDisks(m_pWbemServices);
	int index = 0;
	for (const auto& it : m_vecPhysicalDisks) {
		m_list.InsertItem(index, it.wstrFriendlyName.data());
		m_list.SetItemText(index, 1, std::format(L"{:.1f} GB", static_cast<double>(it.ullSize) / 1024 / 1024 / 1024).data());
		m_list.SetItemText(index, 2, it.wstrPath.data());
		if (g_mapMediaType.contains(it.eMediaType)) {
			m_list.SetItemText(index, 3, g_mapMediaType.at(it.eMediaType).data());
		}

		if (g_mapBusType.contains(it.eBusType)) {
			m_list.SetItemText(index, 4, g_mapBusType.at(it.eBusType).data());
		}

		++index;
	}

	return TRUE;
}

void CDlgOpenDisk::OnOK()
{
	m_vecPaths.clear();
	int nItem { -1 };
	for (auto i { 0UL }; i < m_list.GetSelectedCount(); ++i) {
		nItem = m_list.GetNextItem(nItem, LVNI_SELECTED);
		m_vecPaths.emplace_back(std::move(m_vecPhysicalDisks.at(nItem).wstrPath));
	}

	if (!m_vecPaths.empty()) {
		static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDOK);
	}
}

void CDlgOpenDisk::OnCancel()
{
	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}

void CDlgOpenDisk::OnListDblClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem >= 0 && pNMI->iSubItem >= 0) {
		OnOK();
	}
}

void CDlgOpenDisk::OnListItemChanged(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	GetDlgItem(IDOK)->EnableWindow(m_list.GetSelectedCount() > 0);
}

auto CDlgOpenDisk::GetPhysicalDisks(IWbemServices *pWbemServices)->std::vector<PHYSICALDISK>
{
	if (pWbemServices == nullptr) {
		return { };
	}

	CComPtr<IEnumWbemClassObject> pMSFT_PhysicalDisk;
	pWbemServices->ExecQuery(_bstr_t(L"WQL"), _bstr_t(L"SELECT * FROM MSFT_PhysicalDisk"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pMSFT_PhysicalDisk);

	std::vector<Utility::PHYSICALDISK> vecRet;
	while (pMSFT_PhysicalDisk) {
		IWbemClassObject* pStorage { };
		ULONG uRet { 0 };
		if (const auto hr = pMSFT_PhysicalDisk->Next(WBEM_INFINITE, 1, &pStorage, &uRet); hr != S_OK) {
			break;
		}

		PHYSICALDISK stDisk;
		VARIANT varFriendlyName;
		pStorage->Get(L"FriendlyName", 0, &varFriendlyName, nullptr, nullptr);
		stDisk.wstrFriendlyName = varFriendlyName.bstrVal;

		VARIANT varDeviceId;
		pStorage->Get(L"DeviceId", 0, &varDeviceId, nullptr, nullptr);
		stDisk.wstrPath = std::wstring { L"\\\\.\\PhysicalDrive" } + varDeviceId.bstrVal;

		VARIANT varSize;
		pStorage->Get(L"Size", 0, &varSize, nullptr, nullptr);
		if (const auto opt = stn::StrToULL(varSize.bstrVal); opt) {
			stDisk.ullSize = *opt;
		}

		VARIANT varBusType;
		pStorage->Get(L"BusType", 0, &varBusType, nullptr, nullptr);
		stDisk.eBusType = static_cast<Utility::EBusType>(varBusType.uiVal);

		VARIANT varMediaType;
		pStorage->Get(L"MediaType", 0, &varMediaType, nullptr, nullptr);
		stDisk.eMediaType = static_cast<Utility::EMediaType>(varMediaType.uiVal);

		vecRet.emplace_back(stDisk);
		pStorage->Release();
	}

	return vecRet;
}


//CDlgOpenVolume.

class CDlgOpenVolume final : public CDialogEx
{
public:
	[[nodiscard]] auto GetPaths() -> std::vector<std::wstring>&;
	void SetIWbemServices(IWbemServices* pWbemServices);
private:
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
	void OnOK()override;
	void OnCancel()override;
	afx_msg void OnListDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP();
	[[nodiscard]] static auto GetVolumes(IWbemServices* pWbemServices) -> std::vector<Utility::VOLUME>;
private:
	CListCtrl m_list;
	IWbemServices* m_pWbemServices { };
	std::vector<Utility::VOLUME> m_vecVolumes;
	std::vector<std::wstring> m_vecPaths; //Paths to open.
};

BEGIN_MESSAGE_MAP(CDlgOpenVolume, CDialogEx)
	ON_NOTIFY(NM_DBLCLK, IDC_OPENVOLUME_LIST_VOLUMES, &CDlgOpenVolume::OnListDblClick)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_OPENVOLUME_LIST_VOLUMES, &CDlgOpenVolume::OnListItemChanged)
END_MESSAGE_MAP()

auto CDlgOpenVolume::GetPaths()->std::vector<std::wstring>&
{
	return m_vecPaths;
}

void CDlgOpenVolume::SetIWbemServices(IWbemServices* pWbemServices)
{
	m_pWbemServices = pWbemServices;
}


//CDlgOpenVolume Private methods.

void CDlgOpenVolume::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OPENVOLUME_LIST_VOLUMES, m_list);
}

BOOL CDlgOpenVolume::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_list.SetExtendedStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list.InsertColumn(0, L"Drive", 0, 100);
	m_list.InsertColumn(1, L"Label", 0, 100);
	m_list.InsertColumn(2, L"Size", 0, 70);
	m_list.InsertColumn(3, L"Type", 0, 100);
	m_list.InsertColumn(4, L"File System", 0, 80);
	m_list.InsertColumn(5, L"Path", 0, 300);

	m_vecVolumes = GetVolumes(m_pWbemServices);
	int index = 0;
	for (const auto& it : m_vecVolumes) {
		m_list.InsertItem(index, it.wstrDriveLetter.data());
		m_list.SetItemText(index, 1, it.wstrFileSystemLabel.data());
		m_list.SetItemText(index, 2, std::format(L"{:.1f} GB", static_cast<double>(it.ullSize) / 1024 / 1024 / 1024).data());
		m_list.SetItemText(index, 3, it.wstrDriveType.data());
		m_list.SetItemText(index, 4, it.wstrFileSystem.data());
		m_list.SetItemText(index, 5, it.wstrPath.data());

		++index;
	}

	return TRUE;
}

void CDlgOpenVolume::OnOK()
{
	m_vecPaths.clear();
	int nItem { -1 };
	for (auto i { 0UL }; i < m_list.GetSelectedCount(); ++i) {
		nItem = m_list.GetNextItem(nItem, LVNI_SELECTED);
		auto& refPath = m_vecVolumes.at(nItem).wstrPath;
		if (refPath.ends_with(L'\\')) { //Remove a trailing slash to satisfy CreateFileW.
			refPath = refPath.substr(0, refPath.size() - 1);
		}

		m_vecPaths.emplace_back(std::move(refPath));
	}

	if (!m_vecPaths.empty()) {
		static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDOK);
	}
}

void CDlgOpenVolume::OnCancel()
{
	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}

void CDlgOpenVolume::OnListDblClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem >= 0 && pNMI->iSubItem >= 0) {
		OnOK();
	}
}

void CDlgOpenVolume::OnListItemChanged(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	GetDlgItem(IDOK)->EnableWindow(m_list.GetSelectedCount() > 0);
}

auto CDlgOpenVolume::GetVolumes(IWbemServices *pWbemServices)->std::vector<Utility::VOLUME>
{
	if (pWbemServices == nullptr) {
		return { };
	}

	CComPtr<IEnumWbemClassObject> pMSFT_Volume;
	pWbemServices->ExecQuery(_bstr_t(L"WQL"), _bstr_t(L"SELECT * FROM MSFT_Volume"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pMSFT_Volume);

	std::vector<VOLUME> vecRet;
	while (pMSFT_Volume) {
		IWbemClassObject* pStorage { };
		ULONG uRet { 0 };
		if (const auto hr = pMSFT_Volume->Next(WBEM_INFINITE, 1, &pStorage, &uRet); hr != S_OK) {
			break;
		}

		VOLUME stDisk;
		VARIANT varDriveLetter;
		pStorage->Get(L"DriveLetter", 0, &varDriveLetter, nullptr, nullptr);
		stDisk.wstrDriveLetter = std::wstring { varDriveLetter.uiVal } + L":\\";

		VARIANT varFileSystemLabel;
		pStorage->Get(L"FileSystemLabel", 0, &varFileSystemLabel, nullptr, nullptr);
		stDisk.wstrFileSystemLabel = varFileSystemLabel.bstrVal;

		VARIANT varFileSystem;
		pStorage->Get(L"FileSystem", 0, &varFileSystem, nullptr, nullptr);
		stDisk.wstrFileSystem = varFileSystem.bstrVal;

		VARIANT varDriveType;
		pStorage->Get(L"DriveType", 0, &varDriveType, nullptr, nullptr);
		switch (varDriveType.uintVal) {
		case 0:
			stDisk.wstrDriveType = L"Unknown";
			break;
		case 1:
			stDisk.wstrDriveType = L"Invalid Root Path";
			break;
		case 2:
			stDisk.wstrDriveType = L"Removable";
			break;
		case 3:
			stDisk.wstrDriveType = L"Fixed";
			break;
		case 4:
			stDisk.wstrDriveType = L"Remote";
			break;
		case 5:
			stDisk.wstrDriveType = L"CD-ROM";
			break;
		case 6:
			stDisk.wstrDriveType = L"RAM Disk";
			break;
		default:
			break;
		}

		VARIANT varPath;
		pStorage->Get(L"Path", 0, &varPath, nullptr, nullptr);
		stDisk.wstrPath = varPath.bstrVal;

		VARIANT varSize;
		pStorage->Get(L"Size", 0, &varSize, nullptr, nullptr);
		stDisk.ullSize = varSize.ullVal;
		if (const auto opt = stn::StrToULL(varSize.bstrVal); opt) {
			stDisk.ullSize = *opt;
		}

		vecRet.emplace_back(stDisk);
		pStorage->Release();
	}

	return vecRet;
}


//CDlgOpenPath.

class CDlgOpenPath final : public CDialogEx
{
public:
	[[nodiscard]] auto GetPaths() -> std::vector<std::wstring>&;
private:
	void DoDataExchange(CDataExchange* pDX)override;
	void OnOK()override;
	void OnCancel()override;
	void OnComboPathEdit();
	DECLARE_MESSAGE_MAP();
private:
	CComboBox m_stComboPath;
	std::vector<std::wstring> m_vecPaths; //Paths to open.
};

BEGIN_MESSAGE_MAP(CDlgOpenPath, CDialogEx)
	ON_CBN_EDITUPDATE(IDC_OPENPATH_COMBO_PATH, &CDlgOpenPath::OnComboPathEdit)
END_MESSAGE_MAP()

auto CDlgOpenPath::GetPaths()->std::vector<std::wstring>&
{
	return m_vecPaths;
}


//CDlgOpenPath Private methods.

void CDlgOpenPath::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OPENPATH_COMBO_PATH, m_stComboPath);
}

void CDlgOpenPath::OnOK()
{
	m_vecPaths.clear();
	CString cstrText;
	m_stComboPath.GetWindowTextW(cstrText);
	m_vecPaths.emplace_back(cstrText);
	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDOK);
}

void CDlgOpenPath::OnCancel()
{
	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}

void CDlgOpenPath::OnComboPathEdit()
{
	GetDlgItem(IDOK)->EnableWindow(m_stComboPath.GetWindowTextLengthW() > 0);
}


//CDlgOpenDevice.

export class CDlgOpenDevice final : public CDialogEx
{
public:
	CDlgOpenDevice(CWnd* pParent = nullptr);
	~CDlgOpenDevice() = default;
	INT_PTR DoModal(int iTab = 0);
	[[nodiscard]] auto GetPaths() -> std::vector<std::wstring>&;
private:
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
	afx_msg void OnDestroy();
	afx_msg void OnTabSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	void SetCurrentTab(int iTab);
	DECLARE_MESSAGE_MAP();
private:
	CTabCtrl m_tabMain;
	CRect m_rcWnd; //Dialog rect to set minimum size in the OnGetMinMaxInfo.
	std::unique_ptr<CDlgOpenDisk> m_pDlgDisk;
	std::unique_ptr<CDlgOpenVolume> m_pDlgVolume;
	std::unique_ptr<CDlgOpenPath> m_pDlgPath;
	int m_iCurTab { }; //Current tab. To avoid call m_tabMain.GetCurSel after dialog destroyed.
};


BEGIN_MESSAGE_MAP(CDlgOpenDevice, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_OPENDEVICE_TAB, &CDlgOpenDevice::OnTabSelChanged)
	ON_WM_GETMINMAXINFO()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CDlgOpenDevice::CDlgOpenDevice(CWnd* pParent) : CDialogEx(IDD_OPENDEVICE, pParent)
{
}

INT_PTR CDlgOpenDevice::DoModal(int iTab)
{
	m_iCurTab = iTab;

	return CDialogEx::DoModal();
}

auto CDlgOpenDevice::GetPaths()->std::vector<std::wstring>&
{
	switch (m_iCurTab) {
	case 0:
		return m_pDlgDisk->GetPaths();
	case 1:
		return m_pDlgVolume->GetPaths();
	case 2:
		return m_pDlgPath->GetPaths();
	default:
		std::unreachable();
	}
}


//CDlgOpenDevice Private methods.

void CDlgOpenDevice::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OPENDEVICE_TAB, m_tabMain);
}

BOOL CDlgOpenDevice::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	GetWindowRect(m_rcWnd);

	m_tabMain.InsertItem(0, L"Physical Disks");
	m_tabMain.InsertItem(1, L"Volumes");
	m_tabMain.InsertItem(3, L"Path");
	CRect rcTab;
	m_tabMain.GetItemRect(0, rcTab);

	//Initialize COM and IWbemServices interface for the "Root\\Microsoft\\Windows\\Storage".
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
	CComPtr<IWbemLocator> pWbemLocator;
	CComPtr<IWbemServices> pWbemServices;
	if (CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
		reinterpret_cast<LPVOID*>(&pWbemLocator)) == S_OK) {
		pWbemLocator->ConnectServer(_bstr_t(L"Root\\Microsoft\\Windows\\Storage"), nullptr, nullptr, nullptr, 0,
			nullptr, nullptr, &pWbemServices);
		CoSetProxyBlanket(pWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
			RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
	}

	m_pDlgDisk = std::make_unique<CDlgOpenDisk>();
	m_pDlgDisk->SetIWbemServices(pWbemServices);
	m_pDlgDisk->Create(IDD_OPENDISK, this);
	m_pDlgDisk->SetWindowPos(nullptr, rcTab.left, rcTab.bottom + 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

	m_pDlgVolume = std::make_unique<CDlgOpenVolume>();
	m_pDlgVolume->SetIWbemServices(pWbemServices);
	m_pDlgVolume->Create(IDD_OPENVOLUME, this);
	m_pDlgVolume->SetWindowPos(nullptr, rcTab.left, rcTab.bottom + 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);

	m_pDlgPath = std::make_unique<CDlgOpenPath>();
	m_pDlgPath->Create(IDD_OPENPATH, this);
	m_pDlgPath->SetWindowPos(nullptr, rcTab.left, rcTab.bottom + 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);

	EnableDynamicLayout(TRUE);
	const auto pLayout = GetDynamicLayout();
	pLayout->Create(this);
	pLayout->AddItem(m_pDlgDisk->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
	pLayout->AddItem(m_pDlgVolume->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
	pLayout->AddItem(m_pDlgPath->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));

	SetCurrentTab(m_iCurTab);

	const auto hIcon = AfxGetApp()->LoadIconW(IDR_HEXER_FRAME);
	SetIcon(hIcon, TRUE);
	SetIcon(hIcon, FALSE);

	return TRUE;
}

void CDlgOpenDevice::OnDestroy()
{
	CDialogEx::OnDestroy();

	//CoUninitialize must be called only after all CComPtr<> objects are destroyed.
	//Othervise exceptions occur.
	CoUninitialize();
}

void CDlgOpenDevice::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize.x = m_rcWnd.Width();
	lpMMI->ptMinTrackSize.y = m_rcWnd.Height();

	CDialogEx::OnGetMinMaxInfo(lpMMI);
}

void CDlgOpenDevice::OnTabSelChanged(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	SetCurrentTab(m_tabMain.GetCurSel());
}

void CDlgOpenDevice::SetCurrentTab(int iTab)
{
	m_iCurTab = iTab;
	m_tabMain.SetCurSel(iTab);
	switch (iTab) {
	case 0:
		m_pDlgVolume->ShowWindow(SW_HIDE);
		m_pDlgPath->ShowWindow(SW_HIDE);
		m_pDlgDisk->ShowWindow(SW_SHOW);
		break;
	case 1:
		m_pDlgDisk->ShowWindow(SW_HIDE);
		m_pDlgPath->ShowWindow(SW_HIDE);
		m_pDlgVolume->ShowWindow(SW_SHOW);
		break;
	case 2:
		m_pDlgDisk->ShowWindow(SW_HIDE);
		m_pDlgVolume->ShowWindow(SW_HIDE);
		m_pDlgPath->ShowWindow(SW_SHOW);
		break;
	default:
		std::unreachable();
	}
}