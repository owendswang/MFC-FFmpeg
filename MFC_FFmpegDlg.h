
// MFC_FFmpegDlg.h : header file
//

#pragma once


// CMFCFFmpegDlg dialog
class CMFCFFmpegDlg : public CDialogEx
{
// Construction
public:
	CMFCFFmpegDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFC_FFMPEG_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	LONG dlgMinWidth;
	LONG dlgMinHeight;
	CWinThread* pThreadPlay;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnBnClickedButtonPlay();
	afx_msg void OnBnClickedButtonPause();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnBnClickedButtonBrowse();
	CEdit m_url;
	afx_msg void OnDropFiles(HDROP hDropInfo);
};
