
// FTSv3Dlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "FTSv3.h"
#include "FTSv3Dlg.h"
#include "afxdialogex.h"
#include <memory>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include <string>
#include "CNetwork.h"

using namespace std;

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CFTSv3Dlg 대화 상자



CFTSv3Dlg::CFTSv3Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_FTSV3_DIALOG, pParent)
	, m_strCode(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFTSv3Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
	DDX_Text(pDX, IDC_EDIT_CODE, m_strCode);
}

BEGIN_MESSAGE_MAP(CFTSv3Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(IDC_RADIO_SENDER, &CFTSv3Dlg::OnRadioSender)
	ON_COMMAND(IDC_RADIO_RECEIVER, &CFTSv3Dlg::OnRadioReceiver)
	ON_BN_CLICKED(IDC_BUTTON_START, &CFTSv3Dlg::OnClickedButtonStart)
	ON_MESSAGE(WM_ADD_LOG, &CFTSv3Dlg::OnAddLog)
	ON_BN_CLICKED(IDC_BUTTON1, &CFTSv3Dlg::OnClickedButton1)
END_MESSAGE_MAP()


// CFTSv3Dlg 메시지 처리기

BOOL CFTSv3Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CFTSv3Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CFTSv3Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CFTSv3Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CFTSv3Dlg::OnRadioSender()
{
	m_bRole = 0;
}

void CFTSv3Dlg::OnRadioReceiver()
{
	m_bRole = 1;
}

void CFTSv3Dlg::OnClickedButtonStart()
{
	UpdateData(TRUE);
	addLog("=== START ===");
	if (m_strCode.IsEmpty()) {
		addLog("Enter Code!!");
		return;
	}
	m_network = std::make_unique<CNetwork>(m_hWnd, string(CT2CA(m_strCode)));
	m_network->setupIce();

	if (m_bRole) {
		addLog("Receiver mode");
		m_network->startReceive();
	}
	else {
		addLog("Sender mode");
		CFileDialog dlg(TRUE, NULL, _T("*"), OFN_HIDEREADONLY, _T("*.*"));
		if (dlg.DoModal() != IDOK)
			return;
		m_network->path = CT2CA(dlg.GetPathName());
		m_network->startSend();
	}
}

void CFTSv3Dlg::addLog(const std::string& log)
{
	const int MAX_LEN = 800;

	m_strLog += CString(log.c_str());
	m_strLog += "\r\n";

	if (m_strLog.GetLength() > MAX_LEN) {
		m_strLog = m_strLog.Right(MAX_LEN);
	}

	m_editLog.SetSel(m_editLog.GetWindowTextLengthW());
	m_editLog.ReplaceSel(m_strLog);
}


LRESULT CFTSv3Dlg::OnAddLog(WPARAM, LPARAM lParam)
{
	auto* text = reinterpret_cast<std::string*>(lParam);
	if (text) {
		addLog(*text);
		delete text;
	}
	return 0;
}
void CFTSv3Dlg::OnClickedButton1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	PWSTR downloadPath = nullptr;

	if (SUCCEEDED(SHGetKnownFolderPath(
		FOLDERID_Downloads,
		0,
		NULL,
		&downloadPath)))
	{
		ShellExecuteW(
			nullptr,
			L"open",
			downloadPath,
			nullptr,
			nullptr,
			SW_SHOWNORMAL
		);

		CoTaskMemFree(downloadPath);
	}
}
