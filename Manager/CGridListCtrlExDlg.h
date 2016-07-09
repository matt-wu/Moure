// CGridListCtrlExDlg.h : header file
//

#pragma once

#include "..\CGridListCtrlEx\CGridListCtrlGroups.h"
#include "CListCtrl_DataModel.h"


class CMoureListCtrlGroups : public CGridListCtrlGroups
{
public:
	virtual bool OnEditComplete(int nRow, int nCol, CWnd* pEditor, LV_DISPINFO* pLVDI);
};

// CGridListCtrlExDlg dialog
class CGridListCtrlExDlg : public CDialog
{
// Construction
public:
	explicit CGridListCtrlExDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_CGRIDLISTCTRLEX_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);

	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:

    int m_TimeoutSeconds;
    int m_TickCount;

	CListCtrl_DataModel m_DataModel;

private:
    int m_nStateImageIdx;

	CMoureListCtrlGroups m_ListCtrl;
	CImageList m_ImageList;

	CGridListCtrlExDlg(const CGridListCtrlExDlg&);
	CGridListCtrlExDlg& operator=(const CGridListCtrlExDlg&);

    VOID ClearListView();
    VOID UpdateListView();
};
