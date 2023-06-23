/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <afxdialogex.h>
#include <Wbemidl.h>
#include <string>
#include <vector>
import Utility;

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