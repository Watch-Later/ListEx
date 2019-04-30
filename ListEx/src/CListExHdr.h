/********************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/					*
* Github repository URL: https://github.com/jovibor/ListEx						*
* This software is available under the "MIT License".							*
* This is an extended and featured version of CMFCListCtrl class.				*
* CListEx - list control class with the ability to set tooltips on arbitrary	*
* cells, and also with a lots of other stuff to customize your control in many	*
* different aspects. For more info see official documentation on github.		*
********************************************************************************/
#pragma once
#include <unordered_map> //std::unordered_map and related.

namespace LISTEX {
	/********************************************
	* CListExHdr class definition.				*
	********************************************/
	class CListExHdr : public CMFCHeaderCtrl
	{
	public:
		CListExHdr();
		virtual ~CListExHdr() {}
		void SetHeight(DWORD dwHeight);
		void SetFont(const LOGFONTW* pLogFontNew);
		void SetColor(COLORREF clrText, COLORREF clrBk);
		void SetColumnColor(DWORD dwColumn, COLORREF clr);
	protected:
		afx_msg void OnDrawItem(CDC* pDC, int iItem, CRect rect, BOOL bIsPressed, BOOL bIsHighlighted) override;
		afx_msg LRESULT OnLayout(WPARAM wParam, LPARAM lParam);
		DECLARE_MESSAGE_MAP()
	private:
		CFont m_fontHdr;
		COLORREF m_clrBkNWA { GetSysColor(COLOR_WINDOW) }; //Bk of non working area.
		COLORREF m_clrText { GetSysColor(COLOR_WINDOWTEXT) };
		COLORREF m_clrBk { GetSysColor(COLOR_WINDOW) };
		HDITEMW m_hdItem { }; //For drawing.
		WCHAR m_wstrHeaderText[MAX_PATH] { };
		DWORD m_dwHeaderHeight { 19 }; //Standard (default) height.
		std::unordered_map<DWORD, COLORREF> m_umapClrColumn { }; //Color of individual columns.
	};
}