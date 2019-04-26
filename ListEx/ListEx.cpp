/********************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/					*
* Github repository URL: https://github.com/jovibor/ListEx						*
* This software is available under the "MIT License".							*
* This is an extended and featured version of CMFCListCtrl class.				*
* CListEx - list control class with the ability to set tooltips on arbitrary	*
* cells, and also with a lots of other stuff to customize your control in many	*
* different aspects. For more info see official documentation on github.		*
********************************************************************************/
#include "pch.h"
#include "ListExHdr.h"
#include "ListEx.h"
#include "strsafe.h"

using namespace LISTEX;

/****************************************************
* CListEx class implementation.						*
****************************************************/
IMPLEMENT_DYNAMIC(CListEx, CMFCListCtrl)

BEGIN_MESSAGE_MAP(CListEx, CMFCListCtrl)
	ON_WM_SETCURSOR()
	ON_WM_KILLFOCUS()
	ON_WM_HSCROLL()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_PAINT()
	ON_WM_MEASUREITEM_REFLECT()
	ON_NOTIFY(HDN_DIVIDERDBLCLICKA, 0, &CListEx::OnHdnDividerdblclick)
	ON_NOTIFY(HDN_DIVIDERDBLCLICKW, 0, &CListEx::OnHdnDividerdblclick)
	ON_NOTIFY(HDN_BEGINTRACKA, 0, &CListEx::OnHdnBegintrack)
	ON_NOTIFY(HDN_BEGINTRACKW, 0, &CListEx::OnHdnBegintrack)
	ON_NOTIFY(HDN_TRACKA, 0, &CListEx::OnHdnTrack)
	ON_NOTIFY(HDN_TRACKW, 0, &CListEx::OnHdnTrack)
	ON_WM_CONTEXTMENU()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

bool CListEx::Create(const LISTEXCREATESTRUCT & lcs)
{
	if (lcs.fDialogCtrl)
		SetWindowLongPtrW(m_hWnd, GWL_STYLE, GetWindowLongPtrW(m_hWnd, GWL_STYLE) | LVS_OWNERDRAWFIXED | LVS_REPORT);
	else if (!CMFCListCtrl::Create(lcs.dwStyle | WS_CHILD | WS_VISIBLE | LVS_OWNERDRAWFIXED | LVS_REPORT,
		lcs.rect, lcs.pwndParent, lcs.nID))
		return false;

	if (lcs.pstColor)
		m_stColor = *lcs.pstColor;

	m_hwndTt = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
		TTS_BALLOON | TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		nullptr, nullptr, nullptr, nullptr);
	if (!m_hwndTt)
		return false;
	SetWindowTheme(m_hwndTt, nullptr, L""); //To prevent Windows from changing theme of Balloon window.

	m_stToolInfo.cbSize = TTTOOLINFOW_V1_SIZE;
	m_stToolInfo.uFlags = TTF_TRACK;
	m_stToolInfo.uId = 0x1;
	::SendMessageW(m_hwndTt, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)& m_stToolInfo);
	::SendMessageW(m_hwndTt, TTM_SETMAXTIPWIDTH, 0, (LPARAM)400); //to allow use of newline \n.
	::SendMessageW(m_hwndTt, TTM_SETTIPTEXTCOLOR, (WPARAM)m_stColor.clrTooltipText, 0);
	::SendMessageW(m_hwndTt, TTM_SETTIPBKCOLOR, (WPARAM)m_stColor.clrTooltipBk, 0);

	m_dwGridWidth = lcs.dwListGridWidth;
	m_stNMII.hdr.code = LISTEX_MSG_MENUSELECTED;
	m_stNMII.hdr.idFrom = GetDlgCtrlID();
	m_stNMII.hdr.hwndFrom = m_hWnd;

	LOGFONTW lf { };
	if (lcs.pListLogFont)
		lf = *lcs.pListLogFont;
	else
	{
		NONCLIENTMETRICSW ncm { };
		ncm.cbSize = sizeof(NONCLIENTMETRICSW);
		SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		ncm.lfMessageFont.lfHeight = 18; //For some weird reason above func returns this value as MAX_LONG.
		lf = ncm.lfMessageFont;
	}

	GetHeaderCtrl().SetColor(m_stColor.clrHeaderText, m_stColor.clrHeaderBk);
	SetHeaderHeight(lcs.dwHeaderHeight);
	SetHeaderFont(lcs.pHeaderLogFont);

	m_fontList.CreateFontIndirectW(&lf);
	m_penGrid.CreatePen(PS_SOLID, m_dwGridWidth, m_stColor.clrListGrid);
	m_fCreated = true;
	Update(0);

	return true;
}

void CListEx::CreateDialogCtrl()
{
	LISTEXCREATESTRUCT lcs;
	lcs.fDialogCtrl = true;
	Create(lcs);
}

bool CListEx::IsCreated()
{
	return m_fCreated;
}

void CListEx::SetColor(const LISTEXCOLORSTRUCT & lcs)
{
	m_stColor = lcs;
	GetHeaderCtrl().SetColor(m_stColor.clrHeaderText, m_stColor.clrHeaderBk);
	RedrawWindow();
}

void CListEx::SetFont(const LOGFONTW * pLogFontNew)
{
	if (!pLogFontNew)
		return;

	m_fontList.DeleteObject();
	m_fontList.CreateFontIndirectW(pLogFontNew);

	//To get WM_MEASUREITEM msg after changing font.
	CRect rc;
	GetWindowRect(&rc);
	WINDOWPOS wp;
	wp.hwnd = m_hWnd;
	wp.cx = rc.Width();
	wp.cy = rc.Height();
	wp.flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;
	SendMessageW(WM_WINDOWPOSCHANGED, 0, (LPARAM)& wp);

	Update(0);
}

void CListEx::SetFontSize(UINT uiSize)
{
	//Prevent size from being too small or too big.
	if (uiSize < 9 || uiSize > 75)
		return;

	LOGFONT lf;
	m_fontList.GetLogFont(&lf);
	lf.lfHeight = uiSize;
	m_fontList.DeleteObject();
	m_fontList.CreateFontIndirectW(&lf);

	//To get WM_MEASUREITEM msg after changing font.
	CRect rc;
	GetWindowRect(&rc);
	WINDOWPOS wp;
	wp.hwnd = m_hWnd;
	wp.cx = rc.Width();
	wp.cy = rc.Height();
	wp.flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;
	SendMessageW(WM_WINDOWPOSCHANGED, 0, (LPARAM)& wp);

	Update(0);
}

UINT CListEx::GetFontSize()
{
	LOGFONT lf;
	m_fontList.GetLogFont(&lf);

	return lf.lfHeight;
}

void CListEx::SetCellTooltip(int iItem, int iSubitem, const std::wstring & wstrTooltip, const std::wstring & wstrCaption)
{
	auto it = m_umapCellTt.find(iItem);

	//If there is no tooltip for such item/subitem we just set it.
	if (it == m_umapCellTt.end() && (!wstrTooltip.empty() || !wstrCaption.empty()))
	{
		//Initializing inner map.
		std::unordered_map<int, std::tuple< std::wstring, std::wstring>> umapInner {
			{ iSubitem, { wstrTooltip, wstrCaption } } };
		m_umapCellTt.insert({ iItem, std::move(umapInner) });
	}
	else
	{
		auto itInner = it->second.find(iSubitem);

		//If there is Item's tooltip but no Subitem's tooltip
		//inserting new Subitem into inner map.
		if (itInner == it->second.end())
			it->second.insert({ iSubitem, { wstrTooltip, wstrCaption } });
		else //If there is already exist this Item-Subitem's tooltip:
			 //change or erase it, depending on wstrTooltip emptiness.
			if (!wstrTooltip.empty())
				itInner->second = { wstrTooltip, wstrCaption };
			else
				it->second.erase(itInner);
	}
}

void CListEx::SetCellMenu(int iItem, int iSubitem, CMenu * pMenu)
{
	auto it = m_umapCellMenu.find(iItem);

	//If there is no tooltip for such item/subitem we just set it.
	if (it == m_umapCellMenu.end())
	{	//Initializing inner map.
		std::unordered_map<int, CMenu*> umapInner { { iSubitem, pMenu } };
		m_umapCellMenu.insert({ iItem, std::move(umapInner) });
	}
	else
	{
		auto itInner = it->second.find(iSubitem);

		//If there is Item's tooltip but no Subitem's tooltip
		//inserting new Subitem into inner map.
		if (itInner == it->second.end())
			it->second.insert({ iSubitem, pMenu });
		else //If there is already exist this cell's menu -> changing.
			itInner->second = pMenu;
	}
}

void CListEx::SetListMenu(CMenu * pMenu)
{
	m_pListMenu = pMenu;
}

void CListEx::SetCellData(int iItem, int iSubitem, DWORD_PTR dwData)
{
	auto it = m_umapCellData.find(iItem);

	//If there is no data for such item/subitem we just set it.
	if (it == m_umapCellData.end())
	{	//Initializing inner map.
		std::unordered_map<int, DWORD_PTR> umapInner { { iSubitem, dwData } };
		m_umapCellData.insert({ iItem, std::move(umapInner) });
	}
	else
	{
		auto itInner = it->second.find(iSubitem);

		if (itInner == it->second.end())
			it->second.insert({ iSubitem, dwData });
		else //If there is already exist this cell's data -> changing.
			itInner->second = dwData;
	}
}

DWORD_PTR CListEx::GetCellData(int iItem, int iSubitem)
{
	auto it = m_umapCellData.find(iItem);

	if (it != m_umapCellData.end())
	{
		auto itInner = it->second.find(iSubitem);

		//If subitem id found and its text is not empty.
		if (itInner != it->second.end())
			return itInner->second;
	}

	return 0;
}

void CListEx::SetHeaderHeight(DWORD dwHeight)
{
	GetHeaderCtrl().SetHeight(dwHeight);
	Update(0);
	GetHeaderCtrl().RedrawWindow();
}

void CListEx::SetHeaderFont(const LOGFONT * pLogFontNew)
{
	GetHeaderCtrl().SetFont(pLogFontNew);
	Update(0);
	GetHeaderCtrl().RedrawWindow();
}

void CListEx::SetHeaderColumnColor(DWORD nColumn, COLORREF clr)
{
	GetHeaderCtrl().SetColumnColor(nColumn, clr);
	Update(0);
	GetHeaderCtrl().RedrawWindow();
}

void CListEx::InitHeader()
{
	GetHeaderCtrl().SubclassDlgItem(0, this);
}

void CListEx::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	//Set row height according to current font's height.
	TEXTMETRICW tm;
	CDC* pDC = GetDC();
	pDC->SelectObject(&m_fontList);
	GetTextMetricsW(pDC->m_hDC, &tm);
	lpMIS->itemHeight = tm.tmHeight + tm.tmExternalLeading + 1;
	ReleaseDC(pDC);
}

void CListEx::DrawItem(LPDRAWITEMSTRUCT pDIS)
{
	if (pDIS->itemID == -1)
		return;

	CDC * pDC = CDC::FromHandle(pDIS->hDC);
	pDC->SelectObject(&m_penGrid);
	pDC->SelectObject(&m_fontList);
	COLORREF clrBkCurrRow = (pDIS->itemID % 2) ? m_stColor.clrListBkRow2 : m_stColor.clrListBkRow1;

	switch (pDIS->itemAction)
	{
	case ODA_SELECT:
	case ODA_DRAWENTIRE:
	{
		COLORREF clrText, clrBk;
		CRect rc;
		GetItemRect(pDIS->itemID, rc, LVIR_BOUNDS);

		if (pDIS->itemState & ODS_SELECTED)
		{
			pDIS->rcItem.left = rc.left;
			clrText = m_stColor.clrListTextSelected;
			clrBk = m_stColor.clrListBkSelected;
		}
		else
		{
			if (HasTooltip(pDIS->itemID, 0))
			{
				clrText = m_stColor.clrListTextCellTt;
				clrBk = m_stColor.clrListBkCellTt;
			}
			else
			{
				clrText = m_stColor.clrListText;
				clrBk = clrBkCurrRow;
			}
		}
		pDC->SetTextColor(clrText);
		pDC->FillSolidRect(&pDIS->rcItem, clrBk);

		CString strItem = GetItemText(pDIS->itemID, 0);
		rc.left += 3; //Drawing text +-3 px from rect bounds.
		ExtTextOutW(pDC->m_hDC, rc.left, rc.top, ETO_CLIPPED, rc, strItem, strItem.GetLength(), nullptr);
		rc.left -= 3;

		//Drawing Item's rect lines. 
		pDC->MoveTo(rc.left, rc.top);
		pDC->LineTo(rc.right, rc.top);
		pDC->MoveTo(rc.left, rc.top);
		pDC->LineTo(rc.left, rc.bottom);
		pDC->MoveTo(rc.left, rc.bottom);
		pDC->LineTo(rc.right, rc.bottom);
		pDC->MoveTo(rc.right, rc.top);
		pDC->LineTo(rc.right, rc.bottom);

		for (int i = 1; i < GetHeaderCtrl().GetItemCount(); i++)
		{
			GetSubItemRect(pDIS->itemID, i, LVIR_BOUNDS, rc);

			//Subitem's draw routine. Colors depending on whether subitem selected or not,
			//and has tooltip or not.
			if (pDIS->itemState & ODS_SELECTED)
			{
				clrText = m_stColor.clrListTextSelected;
				clrBk = m_stColor.clrListBkSelected;
			}
			else
			{
				if (HasTooltip(pDIS->itemID, i))
				{
					clrText = m_stColor.clrListTextCellTt;
					clrBk = m_stColor.clrListBkCellTt;
				}
				else
				{
					clrText = m_stColor.clrListText;
					clrBk = clrBkCurrRow;
				}
			}
			pDC->SetTextColor(clrText);
			pDC->FillSolidRect(&rc, clrBk);

			CString strSubitem = GetItemText(pDIS->itemID, i);
			rc.left += 3; //Drawing text +-3 px from rect bounds
			ExtTextOutW(pDC->m_hDC, rc.left, rc.top, ETO_CLIPPED, rc, strSubitem, strSubitem.GetLength(), nullptr);
			rc.left -= 3;

			//Drawing Subitem's rect lines. 
			pDC->MoveTo(rc.left, rc.top);
			pDC->LineTo(rc.right, rc.top);
			pDC->MoveTo(rc.left, rc.top);
			pDC->LineTo(rc.left, rc.bottom);
			pDC->MoveTo(rc.left, rc.bottom);
			pDC->LineTo(rc.right, rc.bottom);
			pDC->MoveTo(rc.right, rc.top);
			pDC->LineTo(rc.right, rc.bottom);
		}
	}
	break;
	case ODA_FOCUS:
		break;
	}
}

void CListEx::OnMouseMove(UINT nFlags, CPoint pt)
{
	LVHITTESTINFO hi { };
	hi.pt = pt;
	ListView_SubItemHitTest(m_hWnd, &hi);
	std::wstring  *pwstrTt { }, *pwstrCaption { };
	bool fHasTooltip = HasTooltip(hi.iItem, hi.iSubItem, &pwstrTt, &pwstrCaption);

	if (fHasTooltip)
	{
		//Check if cursor is still in the same cell's rect. If so - just leave.
		if (m_stCurrCell.iItem == hi.iItem && m_stCurrCell.iSubItem == hi.iSubItem)
			return;

		m_fTtShown = true;
		m_stCurrCell.iItem = hi.iItem;
		m_stCurrCell.iSubItem = hi.iSubItem;
		m_stToolInfo.lpszText = const_cast<LPWSTR>(pwstrTt->data());

		ClientToScreen(&pt);
		::SendMessage(m_hwndTt, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(pt.x, pt.y));
		::SendMessage(m_hwndTt, TTM_SETTITLE, (WPARAM)TTI_NONE, (LPARAM)pwstrCaption->data());
		::SendMessage(m_hwndTt, TTM_UPDATETIPTEXT, 0, (LPARAM)(LPTOOLINFO)& m_stToolInfo);
		::SendMessage(m_hwndTt, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)(LPTOOLINFO)& m_stToolInfo);

		//Timer to check whether mouse left subitem rect.
		SetTimer(ID_TIMER_TOOLTIP, 200, 0);
	}
	else
	{
		m_stCurrCell.iItem = hi.iItem;
		m_stCurrCell.iSubItem = hi.iSubItem;

		//If there is shown tooltip window.
		if (m_fTtShown)
		{
			m_fTtShown = false;
			::SendMessage(m_hwndTt, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)(LPTOOLINFO)& m_stToolInfo);
		}
	}
}

BOOL CListEx::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (nFlags == MK_CONTROL)
	{
		SetFontSize(GetFontSize() + zDelta / WHEEL_DELTA * 2);
		return TRUE;
	}
	GetHeaderCtrl().RedrawWindow();

	return CMFCListCtrl::OnMouseWheel(nFlags, zDelta, pt);
}

void CListEx::OnLButtonDown(UINT nFlags, CPoint pt)
{
	LVHITTESTINFO hi { };
	hi.pt = pt;
	ListView_SubItemHitTest(m_hWnd, &hi);
	if (hi.iSubItem == -1 || hi.iItem == -1)
		return;

	CMFCListCtrl::OnLButtonDown(nFlags, pt);
}

void CListEx::OnRButtonDown(UINT nFlags, CPoint pt)
{
	LVHITTESTINFO hi { };
	hi.pt = pt;
	ListView_SubItemHitTest(m_hWnd, &hi);
	if (hi.iSubItem == -1 || hi.iItem == -1)
		return;

	CMFCListCtrl::OnRButtonDown(nFlags, pt);
}

void CListEx::OnContextMenu(CWnd * pWnd, CPoint pt)
{
	LVHITTESTINFO hi;
	ScreenToClient(&pt);
	hi.pt = pt;
	ListView_SubItemHitTest(m_hWnd, &hi);
	m_stNMII.iItem = hi.iItem;
	m_stNMII.iSubItem = hi.iSubItem;

	ClientToScreen(&pt);
	CMenu* pMenu;
	if (HasMenu(hi.iItem, hi.iSubItem, &pMenu))
		pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, pt.x, pt.y, this);
}

BOOL CListEx::OnCommand(WPARAM wParam, LPARAM lParam)
{
	m_stNMII.lParam = LOWORD(wParam); //lParam holds uiMenuItemId.
	GetParent()->SendMessageW(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)& m_stNMII);

	return CMFCListCtrl::OnCommand(wParam, lParam);
}

void CListEx::OnTimer(UINT_PTR nIDEvent)
{
	//Checking if mouse left list's subitem rect,
	//if so — hiding tooltip and killing timer.
	if (nIDEvent == ID_TIMER_TOOLTIP)
	{
		LVHITTESTINFO hitInfo { };
		CPoint pt;
		GetCursorPos(&pt);
		ScreenToClient(&pt);
		hitInfo.pt = pt;
		ListView_SubItemHitTest(m_hWnd, &hitInfo);

		//If cursor is still hovers subitem then do nothing.
		if (m_stCurrCell.iItem == hitInfo.iItem && m_stCurrCell.iSubItem == hitInfo.iSubItem)
			return;
		else
		{	//If it left.
			m_fTtShown = false;
			::SendMessage(m_hwndTt, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)(LPTOOLINFO)& m_stToolInfo);
			KillTimer(ID_TIMER_TOOLTIP);
			m_stCurrCell.iItem = hitInfo.iItem;
			m_stCurrCell.iSubItem = hitInfo.iSubItem;
		}
	}

	CMFCListCtrl::OnTimer(nIDEvent);
}

BOOL CListEx::OnSetCursor(CWnd * pWnd, UINT nHitTest, UINT message)
{
	return CMFCListCtrl::OnSetCursor(pWnd, nHitTest, message);
}

void CListEx::OnKillFocus(CWnd * pNewWnd)
{
}

BOOL CListEx::OnEraseBkgnd(CDC * pDC)
{
	return FALSE;
}

void CListEx::OnPaint()
{
	//To avoid flickering.
	//Drawing to CMemDC, excluding list header area (rect).
	CRect rc, rcHdr;
	GetClientRect(&rc);
	GetHeaderCtrl().GetClientRect(rcHdr);
	rc.top += rcHdr.Height();

	CPaintDC dc(this);
	CMemDC memDC(dc, rc);
	CDC& rDC = memDC.GetDC();
	rDC.GetClipBox(&rc);
	rDC.FillSolidRect(rc, m_stColor.clrBkNWA);

	DefWindowProcW(WM_PAINT, (WPARAM)rDC.m_hDC, (LPARAM)0);
}

void CListEx::OnHdnDividerdblclick(NMHDR * pNMHDR, LRESULT * pResult)
{
	//LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	//*pResult = 0;
}

void CListEx::OnHdnBegintrack(NMHDR * pNMHDR, LRESULT * pResult)
{
	//LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	//*pResult = 0;
}

void CListEx::OnHdnTrack(NMHDR * pNMHDR, LRESULT * pResult)
{
	//LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	//*pResult = 0;
}

void CListEx::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar * pScrollBar)
{
	CMFCListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CListEx::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar * pScrollBar)
{
	GetHeaderCtrl().RedrawWindow();

	CMFCListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CListEx::OnDestroy()
{
	CMFCListCtrl::OnDestroy();

	::DestroyWindow(m_hwndTt);
}

bool CListEx::HasTooltip(int iItem, int iSubitem, std::wstring * *ppwstrText, std::wstring * *ppwstrCaption)
{
	//Can return true/false indicating if subitem has tooltip,
	//or can return pointers to tooltip text as well, if poiters are not nullptr.
	auto it = m_umapCellTt.find(iItem);
	if (it != m_umapCellTt.end())
	{
		auto itInner = it->second.find(iSubitem);

		//If subitem id found and its text is not empty.
		if (itInner != it->second.end() && !std::get<0>(itInner->second).empty())
		{
			//If pointer for text is nullptr we just return true.
			if (ppwstrText)
			{
				*ppwstrText = &std::get<0>(itInner->second);
				if (ppwstrCaption)
					*ppwstrCaption = &std::get<1>(itInner->second);
			}
			return true;
		}
	}

	return false;
}

bool CListEx::HasMenu(int iItem, int iSubitem, CMenu * *ppMenu)
{
	if (iItem < 0 || iSubitem < 0)
		return false;

	auto it = m_umapCellMenu.find(iItem);

	if (it != m_umapCellMenu.end())
	{
		auto itInner = it->second.find(iSubitem);

		//If subitem id found and its text is not empty.
		if (itInner != it->second.end())
		{
			if (ppMenu)
				*ppMenu = itInner->second;

			return true;
		}
	}

	//If there is no menu for cell, then checking global menu for the whole list.
	if (m_pListMenu)
	{
		*ppMenu = m_pListMenu;
		return true;
	}

	return false;
}