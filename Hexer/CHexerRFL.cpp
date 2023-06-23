/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CHexerRFL.h"
#include <algorithm>
#include <cassert>
#include <format>
import Utility;

void CHexerRFL::Initialize(HMENU hMenu, int iIDMenuFirst, HBITMAP hBMPDisk, std::vector<std::wstring>* pVecData, int iMaxEntry)
{
	assert(IsMenu(hMenu));
	assert(pVecData != nullptr);
	if (!IsMenu(hMenu) || pVecData == nullptr) {
		return;
	}

	m_hMenu = hMenu;
	m_iIDMenuFirst = iIDMenuFirst;
	m_hBMPDisk = hBMPDisk;
	m_pVecData = pVecData;
	m_iMaxEntry = iMaxEntry;
	m_fInit = true;

	RebuildRFLMenu();
}

void CHexerRFL::AddToRFL(std::wstring_view wsvPath)
{
	assert(m_fInit);
	if (!m_fInit)
		return;

	std::erase(*m_pVecData, wsvPath); //Remove any duplicates.
	m_pVecData->emplace(m_pVecData->begin(), wsvPath);
	if (m_pVecData->size() > m_iMaxEntry) {
		m_pVecData->resize(m_iMaxEntry);
	}

	RebuildRFLMenu();
}

auto CHexerRFL::GetPathFromRFL(UINT uID)const->std::wstring
{
	assert(m_fInit);
	if (!m_fInit)
		return { };

	const auto uIndex = uID - m_iIDMenuFirst;
	if (uIndex >= m_pVecData->size())
		return { };

	return m_pVecData->at(uIndex);
}

void CHexerRFL::RebuildRFLMenu()
{
	while (GetMenuItemCount(m_hMenu) > 0) {
		DeleteMenu(m_hMenu, 0, MF_BYPOSITION); //Removing all RFL menu items.
	}

	auto uIndex { 0 };
	for (const auto& wstr : *m_pVecData) {
		if (uIndex >= m_iMaxEntry) //Adding not more than m_iMaxEntry.
			break;

		const auto fDevice = wstr.starts_with(L"\\\\");
		const auto wstrMenu = std::vformat(fDevice ? L"{} Device: {}" : L"{} {}",
			std::make_wformat_args(uIndex + 1, wstr));
		const auto iMenuID = m_iIDMenuFirst + uIndex;
		AppendMenuW(m_hMenu, MF_STRING, iMenuID, wstrMenu.data());

		if (fDevice) {
			MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP }, .hbmpItem { m_hBMPDisk } };
			SetMenuItemInfoW(m_hMenu, iMenuID, FALSE, &mii);
		}

		++uIndex;
	}
}