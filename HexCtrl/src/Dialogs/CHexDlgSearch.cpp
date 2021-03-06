/****************************************************************************************
* Copyright © 2018-2020 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information visit the project's official repository.                         *
****************************************************************************************/
#include "stdafx.h"
#include "../../res/HexCtrlRes.h"
#include "../Helper.h"
#include "CHexDlgCallback.h"
#include "CHexDlgSearch.h"
#include <cassert>
#include <thread>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

/************************************************************
* CHexDlgSearch class implementation.						*
* This class implements search routines within HexControl.	*
************************************************************/
BEGIN_MESSAGE_MAP(CHexDlgSearch, CDialogEx)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BUTTON_SEARCH_F, &CHexDlgSearch::OnButtonSearchF)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BUTTON_SEARCH_B, &CHexDlgSearch::OnButtonSearchB)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BUTTON_REPLACE, &CHexDlgSearch::OnButtonReplace)
	ON_BN_CLICKED(IDC_HEXCTRL_SEARCH_BUTTON_REPLACE_ALL, &CHexDlgSearch::OnButtonReplaceAll)
	ON_CBN_SELCHANGE(IDC_HEXCTRL_SEARCH_COMBO_MODE, &CHexDlgSearch::OnComboModeSelChange)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_HEXCTRL_SEARCH_LIST_MAIN, &CHexDlgSearch::OnListGetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HEXCTRL_SEARCH_LIST_MAIN, &CHexDlgSearch::OnListItemChanged)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CHexDlgSearch::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_CHECK_SELECTION, m_stChkSel);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_SEARCH, m_stComboSearch);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_REPLACE, m_stComboReplace);
	DDX_Control(pDX, IDC_HEXCTRL_SEARCH_COMBO_MODE, m_stComboMode);
}

BOOL CHexDlgSearch::Create(UINT nIDTemplate, CHexCtrl* pHexCtrl)
{
	assert(pHexCtrl);
	if (pHexCtrl == nullptr)
		return FALSE;

	m_pHexCtrl = pHexCtrl;

	return CDialogEx::Create(nIDTemplate, pHexCtrl);
}

BOOL CHexDlgSearch::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_stBrushDefault.CreateSolidBrush(m_clrBkTextArea);

	auto iIndex = m_stComboMode.AddString(L"Hex");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_HEX));
	m_stComboMode.SetCurSel(iIndex);
	iIndex = m_stComboMode.AddString(L"Text (ASCII)");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_ASCII));
	iIndex = m_stComboMode.AddString(L"Text (wchar_t)");
	m_stComboMode.SetItemData(iIndex, static_cast<DWORD_PTR>(EMode::SEARCH_WCHAR));

	m_pListMain->CreateDialogCtrl(IDC_HEXCTRL_SEARCH_LIST_MAIN, this);
	m_pListMain->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_pListMain->InsertColumn(0, L"\u2116", 0, 40);
	m_pListMain->InsertColumn(1, L"Offset", LVCFMT_LEFT, 445);

	return TRUE;
}

void CHexDlgSearch::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if (nState == WA_INACTIVE)
		SetLayeredWindowAttributes(0, 150, LWA_ALPHA);
	else
	{
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
		m_stComboSearch.SetFocus();
		if (GetHexCtrl()->IsCreated() && GetHexCtrl()->IsDataSet())
		{
			bool fMutable = GetHexCtrl()->IsMutable();
			m_stComboReplace.EnableWindow(fMutable);
			GetDlgItem(IDC_HEXCTRL_SEARCH_BUTTON_REPLACE)->EnableWindow(fMutable);
			GetDlgItem(IDC_HEXCTRL_SEARCH_BUTTON_REPLACE_ALL)->EnableWindow(fMutable);
		}
	}

	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
}

void CHexDlgSearch::OnOK()
{
	OnButtonSearchF();
}

void CHexDlgSearch::OnCancel()
{
	GetHexCtrl()->SetFocus();

	CDialogEx::OnCancel();
}

void CHexDlgSearch::OnButtonSearchF()
{
	m_iDirection = 1;
	m_fReplace = false;
	m_fAll = false;
	PrepareSearch();
}

void CHexDlgSearch::OnButtonSearchB()
{
	m_iDirection = -1;
	m_fReplace = false;
	m_fAll = false;
	PrepareSearch();
}

void CHexDlgSearch::OnButtonReplace()
{
	m_iDirection = 1;
	m_fReplace = true;
	m_fAll = false;
	PrepareSearch();
}

void CHexDlgSearch::OnButtonReplaceAll()
{
	m_iDirection = 1;
	m_fReplace = true;
	m_fAll = true;
	PrepareSearch();
}

void CHexDlgSearch::OnComboModeSelChange()
{
	if (auto eMode = GetSearchMode(); eMode != m_eModeCurr)
	{
		ResetSearch();
		m_eModeCurr = eMode;
	}
}

void CHexDlgSearch::OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT)
	{
		const auto nItemID = static_cast<size_t>(pItem->iItem);
		const auto nMaxLengh = static_cast<size_t>(pItem->cchTextMax);

		switch (pItem->iSubItem)
		{
		case 0: //Index number.
			swprintf_s(pItem->pszText, nMaxLengh, L"%zd", nItemID + 1);
			break;
		case 1: //Offset
			swprintf_s(pItem->pszText, nMaxLengh, L"0x%llX", m_vecSearchRes[nItemID]);
			break;
		}
	}
}

void CHexDlgSearch::OnListItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem != -1 && pNMI->iSubItem != -1 && (pNMI->uNewState & LVIS_SELECTED))
		HexCtrlHgl(m_vecSearchRes[static_cast<size_t>(pNMI->iItem)], m_fReplace ? m_nSizeReplace : m_nSizeSearch);
}

HBRUSH CHexDlgSearch::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (pWnd->GetDlgCtrlID() == IDC_HEXCTRL_SEARCH_STATIC_TEXTBOTTOM)
	{
		pDC->SetBkColor(m_clrBkTextArea);
		pDC->SetTextColor(m_fFound ? m_clrSearchFound : m_clrSearchFailed);
		return m_stBrushDefault;
	}

	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CHexDlgSearch::AddToList(ULONGLONG ullOffset)
{
	int iHighlight;
	if (auto iter = std::find(m_vecSearchRes.begin(), m_vecSearchRes.end(), ullOffset);
		iter == m_vecSearchRes.end() && m_vecSearchRes.size() < 1024) //Max 1024 found search occurences.
	{
		m_vecSearchRes.emplace_back(m_ullOffset);
		iHighlight = static_cast<int>(m_vecSearchRes.size());
		m_pListMain->SetItemCountEx(iHighlight);
		iHighlight -= 1;
	}
	else
		iHighlight = static_cast<int>(iter - m_vecSearchRes.begin());

	m_pListMain->SetItemState(-1, 0, LVIS_SELECTED);
	m_pListMain->SetItemState(iHighlight, LVIS_SELECTED, LVIS_SELECTED);
	m_pListMain->EnsureVisible(iHighlight, TRUE);
}

void CHexDlgSearch::HexCtrlHgl(ULONGLONG ullOffset, ULONGLONG ullSize)
{
	auto pHexCtrl = GetHexCtrl();
	if (m_fSelection)
	{
		//Highlight selection.
		std::vector<HEXSPANSTRUCT> vec { { ullOffset, ullSize } };
		pHexCtrl->SetSelHighlight(vec);
		pHexCtrl->GoToOffset(ullOffset);
		pHexCtrl->Redraw();
	}
	else
		pHexCtrl->GoToOffset(ullOffset, true, ullSize);
}

void CHexDlgSearch::Search(bool fForward)
{
	m_iDirection = fForward ? 1 : -1;
	m_fReplace = false;
	m_fAll = false;
	Search();
}

bool CHexDlgSearch::IsSearchAvail()
{
	auto pHexCtrl = GetHexCtrl();
	return !(m_wstrTextSearch.empty() || !pHexCtrl->IsDataSet() || m_ullOffset >= pHexCtrl->GetDataSize());
}

BOOL CHexDlgSearch::ShowWindow(int nCmdShow)
{
	if (nCmdShow == SW_SHOW)
	{
		int iChkStatus { BST_UNCHECKED };
		if (auto vecSel = m_pHexCtrl->GetSelection(); vecSel.size() == 1 && vecSel.back().ullSize > 1)
			iChkStatus = BST_CHECKED;

		m_stChkSel.SetCheck(iChkStatus);
		BringWindowToTop();
	}

	return CDialogEx::ShowWindow(nCmdShow);
}

void CHexDlgSearch::PrepareSearch()
{
	static const wchar_t* const wstrReplaceWarning { L"Replacing string is longer than Find string.\r\n"
		"Do you want to overwrite the bytes following search occurrence?\r\n"
		"Choosing \"No\" will cancel search." };
	static const wchar_t* const wstrWrongInput { L"Wrong input data format!" };

	auto pHexCtrl = GetHexCtrl();
	auto ullDataSize = pHexCtrl->GetDataSize();

	CStringW wstrTextSearch;
	GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_SEARCH, wstrTextSearch);
	if (wstrTextSearch.IsEmpty())
		return;

	if (wstrTextSearch.Compare(m_wstrTextSearch.data()) != 0)
	{
		ResetSearch();
		m_wstrTextSearch = wstrTextSearch;
	}
	ComboSearchFill(wstrTextSearch);

	if (m_fReplace)
	{
		wchar_t warrReplace[64];
		GetDlgItemTextW(IDC_HEXCTRL_SEARCH_COMBO_REPLACE, warrReplace, _countof(warrReplace));
		m_wstrTextReplace = warrReplace;
		if (m_wstrTextReplace.empty())
			return;

		ComboReplaceFill(warrReplace);
	}
	GetDlgItem(IDC_HEXCTRL_SEARCH_COMBO_SEARCH)->SetFocus();

	switch (GetSearchMode())
	{
	case EMode::SEARCH_HEX:
	{
		m_strSearch = WstrToStr(m_wstrTextSearch);
		m_strReplace = WstrToStr(m_wstrTextReplace);
		if (!StrToHex(m_strSearch, m_strSearch))
		{
			m_iWrap = 1;
			return;
		}
		m_nSizeSearch = m_strSearch.size();
		if ((m_fReplace && !StrToHex(m_strReplace, m_strReplace)))
		{
			MessageBoxW(wstrWrongInput, L"Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return;
		}
		m_nSizeReplace = m_strReplace.size();
		m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
		m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());
	}
	break;
	case EMode::SEARCH_ASCII:
	{
		m_strSearch = WstrToStr(m_wstrTextSearch);
		m_strReplace = WstrToStr(m_wstrTextReplace);
		m_nSizeSearch = m_strSearch.size();
		m_nSizeReplace = m_strReplace.size();
		m_pSearchData = reinterpret_cast<std::byte*>(m_strSearch.data());
		m_pReplaceData = reinterpret_cast<std::byte*>(m_strReplace.data());
	}
	break;
	case EMode::SEARCH_WCHAR:
	{
		m_nSizeSearch = m_wstrTextSearch.size() * sizeof(wchar_t);
		m_nSizeReplace = m_wstrTextReplace.size() * sizeof(wchar_t);
		m_pSearchData = reinterpret_cast<std::byte*>(m_wstrTextSearch.data());
		m_pReplaceData = reinterpret_cast<std::byte*>(m_wstrTextReplace.data());
	}
	break;
	}

	//Search in selection.
	if (m_stChkSel.GetCheck() == BST_CHECKED)
	{
		auto vecSel = pHexCtrl->GetSelection();
		if (vecSel.empty()) //No selection.
			return;

		auto ullSelSize = vecSel.front().ullSize;
		if (ullSelSize < m_nSizeSearch) //Selection is too small.
			return;

		m_ullSearchStart = vecSel.front().ullOffset;
		m_ullSearchEnd = m_ullSearchStart + ullSelSize - m_nSizeSearch;
		m_ullEndSentinel = m_ullSearchStart + ullSelSize;
		m_fSelection = true;
	}
	else //Search in whole data.
	{
		m_ullSearchStart = 0;
		m_ullSearchEnd = ullDataSize - m_nSizeSearch;
		m_ullEndSentinel = pHexCtrl->GetDataSize();
		m_fSelection = false;
	}

	if (m_ullOffset + m_nSizeSearch > ullDataSize || m_ullOffset + m_nSizeReplace > ullDataSize)
	{
		m_ullOffset = 0;
		m_fSecondMatch = false;
		return;
	}

	if (m_fReplaceWarning && m_fReplace && (m_nSizeReplace > m_nSizeSearch))
	{
		if (IDNO == MessageBoxW(wstrReplaceWarning, L"Warning", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST))
			return;

		m_fReplaceWarning = false;
	}

	Search();
	SetActiveWindow();
}

void CHexDlgSearch::Search()
{
	auto pHexCtrl = GetHexCtrl();
	if (m_wstrTextSearch.empty() || !pHexCtrl->IsDataSet() || m_ullOffset >= pHexCtrl->GetDataSize())
		return;

	m_fFound = false;
	auto ullUntil = m_ullSearchEnd;

	//Actual Search.
	if (m_fReplace && m_fAll) //Replace All
	{
		auto ullStart = m_ullSearchStart;
		while (true)
		{
			if (Find(ullStart, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel))
			{
				Replace(ullStart, m_pReplaceData, m_nSizeSearch, m_nSizeReplace, false);
				ullStart += m_nSizeReplace;
				m_fFound = true;
				m_dwReplaced++;
				if (ullStart > ullUntil)
					break;
			}
			else
				break;
		}
	}
	else //Search or Search&Replace.
	{
		if (m_iDirection == 1) //Forward direction.
		{
			if (m_fReplace && m_fSecondMatch)
			{
				Replace(m_ullOffset, m_pReplaceData, m_nSizeSearch, m_nSizeReplace);
				m_ullOffset += m_nSizeReplace; //Increase next search step to replaced count.
				m_dwReplaced++;
			}
			else
				m_ullOffset = m_fSecondMatch ? m_ullOffset + 1 : m_ullSearchStart;

			if (Find(m_ullOffset, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel))
			{
				m_fFound = true;
				m_fSecondMatch = true;
				m_dwCount++;
			}
			if (!m_fFound && m_fSecondMatch)
			{
				m_ullOffset = m_ullSearchStart; //Starting from the beginning.
				if (Find(m_ullOffset, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel))
				{
					m_fFound = true;
					m_fSecondMatch = true;
					m_fDoCount = true;
					m_dwCount = 1;
				}
				m_iWrap = 1;
			}
		}
		else if (m_iDirection == -1) //Backward direction
		{
			ullUntil = m_ullSearchStart;
			if (m_fSecondMatch && m_ullOffset > 0)
			{
				m_ullOffset--;
				if (Find(m_ullOffset, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel, false))
				{
					m_fFound = true;
					m_fSecondMatch = true;
					m_dwCount--;
				}
			}
			if (!m_fFound)
			{
				m_ullOffset = m_ullSearchEnd;
				if (Find(m_ullOffset, ullUntil, m_pSearchData, m_nSizeSearch, m_ullEndSentinel, false))
				{
					m_fFound = true;
					m_fSecondMatch = true;
					m_iWrap = -1;
					m_fDoCount = false;
					m_dwCount = 1;
				}
			}
		}
	}

	std::wstring wstrInfo(128, 0);
	if (m_fFound)
	{
		if (m_fReplace && m_fAll)
		{
			swprintf_s(wstrInfo.data(), wstrInfo.size(), L"%lu occurrence(s) replaced.", m_dwReplaced);
			m_dwReplaced = 0;
			pHexCtrl->RedrawWindow();
		}
		else
		{
			if (m_fDoCount)
				swprintf_s(wstrInfo.data(), wstrInfo.size(), L"Found occurrence \u2116 %lu from the beginning.", m_dwCount);
			else
				wstrInfo = L"Search found occurrence.";

			HexCtrlHgl(m_ullOffset, m_fReplace ? m_nSizeReplace : m_nSizeSearch);
			AddToList(m_ullOffset);
		}
	}
	else
	{
		ResetSearch();
		if (m_iWrap == 1)
			wstrInfo = L"Didn't find any occurrence, the end is reached.";
		else
			wstrInfo = L"Didn't find any occurrence, the begining is reached.";
	}

	GetDlgItem(IDC_HEXCTRL_SEARCH_STATIC_TEXTBOTTOM)->SetWindowTextW(wstrInfo.data());
}

bool CHexDlgSearch::Find(ULONGLONG& ullStart, ULONGLONG ullEnd, std::byte* pSearch,
	size_t nSizeSearch, ULONGLONG ullEndSentinel, bool fForward)
{
	if (ullStart + nSizeSearch > ullEndSentinel)
		return false;

	ULONGLONG ullSize = fForward ? ullEnd - ullStart : ullStart - ullEnd; //Depends on search direction
	ULONGLONG ullSizeChunk { };    //Size of the chunk to work with.
	ULONGLONG ullChunks { };
	ULONGLONG ullMemToAcquire { }; //Size of VirtualData memory for acquiring. It's bigger than ullSizeChunk.
	ULONGLONG ullOffsetSearch { };
	constexpr auto sizeQuick { 1024 * 256 }; //256KB.
	auto pHexCtrl = GetHexCtrl();

	switch (pHexCtrl->GetDataMode())
	{
	case EHexDataMode::DATA_MEMORY:
		ullSizeChunk = ullSize;
		ullMemToAcquire = ullSizeChunk;
		ullChunks = 1;
		break;
	case EHexDataMode::DATA_MSG:
	case EHexDataMode::DATA_VIRTUAL:
	{
		ullMemToAcquire = pHexCtrl->GetCacheSize();
		if (ullMemToAcquire > ullSize + nSizeSearch)
			ullMemToAcquire = ullSize + nSizeSearch;
		ullSizeChunk = ullMemToAcquire - nSizeSearch;
		if (ullSize > ullSizeChunk)
			ullChunks = (ullSize % ullSizeChunk) ? ullSize / ullSizeChunk + 1 : ullSize / ullSizeChunk;
		else
			ullChunks = 1;
	}
	break;
	}

	bool fResult { false };
	CHexDlgCallback dlg(L"Searching...");
	std::thread thrd([&]() {
		if (fForward)
		{
			ullOffsetSearch = ullStart;
			for (ULONGLONG iterChunk = 0; iterChunk < ullChunks; ++iterChunk)
			{
				if (ullOffsetSearch + ullMemToAcquire > ullEndSentinel)
				{
					ullMemToAcquire = ullEndSentinel - ullOffsetSearch;
					ullSizeChunk = ullMemToAcquire - nSizeSearch;
				}
				if (iterChunk > 0)
					ullOffsetSearch += ullSizeChunk;

				auto pData = pHexCtrl->GetData({ ullOffsetSearch, ullMemToAcquire });
				for (ULONGLONG i = 0; i <= ullSizeChunk; ++i)
				{
					if (memcmp(pData + i, pSearch, nSizeSearch) == 0)
					{
						ullStart = ullOffsetSearch + i;
						fResult = true;
						goto exit;
					}
					if (dlg.IsCanceled())
						goto exit;
				}
			}
		}
		else
		{
			ullOffsetSearch = ullStart - ullSizeChunk;
			for (ULONGLONG iterChunk = ullChunks; iterChunk > 0; --iterChunk)
			{
				auto pData = pHexCtrl->GetData({ ullOffsetSearch, ullMemToAcquire });
				for (auto i = static_cast<LONGLONG>(ullSizeChunk); i >= 0; --i)	//i might be negative.
				{
					if (memcmp(pData + i, pSearch, nSizeSearch) == 0)
					{
						ullStart = ullOffsetSearch + i;
						fResult = true;
						goto exit;
					}
					if (dlg.IsCanceled())
						goto exit;
				}

				if ((ullOffsetSearch - ullSizeChunk) < ullEnd
					|| (ullOffsetSearch - ullSizeChunk) > (std::numeric_limits<ULONGLONG>::max() - ullSizeChunk))
				{
					ullMemToAcquire = (ullOffsetSearch - ullEnd) + nSizeSearch;
					ullSizeChunk = ullMemToAcquire - nSizeSearch;
				}
				ullOffsetSearch -= ullSizeChunk;
			}
		}
	exit:
		dlg.Cancel();
		});
	if (ullSize > sizeQuick) //Showing "Cancel" dialog only when data > sizeQuick
		dlg.DoModal();
	thrd.join();

	return fResult;
}

void CHexDlgSearch::Replace(ULONGLONG ullIndex, std::byte* pData, size_t nSizeData, size_t nSizeReplace, bool fRedraw)
{
	SMODIFY hms;
	hms.vecSpan.emplace_back(HEXSPANSTRUCT { ullIndex, nSizeData });
	hms.ullDataSize = nSizeReplace;
	hms.pData = pData;
	GetHexCtrl()->Modify(hms, fRedraw);
}

CHexCtrl* CHexDlgSearch::GetHexCtrl()const
{
	return m_pHexCtrl;
}

CHexDlgSearch::EMode CHexDlgSearch::GetSearchMode()
{
	return static_cast<EMode>(m_stComboMode.GetItemData(m_stComboMode.GetCurSel()));
}

void CHexDlgSearch::ComboSearchFill(LPCWSTR pwsz)
{
	//Insert wstring into ComboBox only if it's not already presented.
	if (m_stComboSearch.FindStringExact(0, pwsz) == CB_ERR)
	{
		//Keep max 50 strings in list.
		if (m_stComboSearch.GetCount() == 50)
			m_stComboSearch.DeleteString(49);
		m_stComboSearch.InsertString(0, pwsz);
	}
}

void CHexDlgSearch::ComboReplaceFill(LPCWSTR pwsz)
{
	//Insert wstring into ComboBox only if it's not already presented.
	if (m_stComboReplace.FindStringExact(0, pwsz) == CB_ERR)
	{
		//Keep max 50 strings in list.
		if (m_stComboReplace.GetCount() == 50)
			m_stComboReplace.DeleteString(49);
		m_stComboReplace.InsertString(0, pwsz);
	}
}

void CHexDlgSearch::ResetSearch()
{
	m_ullOffset = { };
	m_dwCount = { };
	m_dwReplaced = { };
	m_iWrap = { };
	m_fSecondMatch = { false };
	m_fFound = { false };
	m_fDoCount = { true };
	m_pListMain->SetItemCountEx(0);
	m_vecSearchRes.clear();
	GetHexCtrl()->ClearSelHighlight();

	GetDlgItem(IDC_HEXCTRL_SEARCH_STATIC_TEXTBOTTOM)->SetWindowTextW(L"");
}

void CHexDlgSearch::OnDestroy()
{
	CDialogEx::OnDestroy();

	m_pListMain->DestroyWindow();
	m_stBrushDefault.DeleteObject();
}