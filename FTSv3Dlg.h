
// FTSv3Dlg.h: 헤더 파일
//
#pragma once
#include <string>
#include <memory>
#include "CNetwork.h"

#define WM_ADD_LOG (WM_USER + 1)

// CFTSv3Dlg 대화 상자
class CFTSv3Dlg : public CDialogEx
{
// 생성입니다.
public:
	CFTSv3Dlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FTSV3_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

private:
	std::unique_ptr<CNetwork> m_network;

// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_editLog;
	CString m_strCode;
	afx_msg void OnRadioSender();
	afx_msg void OnRadioReceiver();
	afx_msg void OnClickedButtonStart();
	bool m_bRole;
	void addLog(const std::string& log);
	CString m_strLog;
	afx_msg LRESULT OnAddLog(WPARAM wParam, LPARAM lParam);
	afx_msg void OnClickedButton1();
};
