#include "stdafx.h"
#include "HexSample.h"
#include "HexSampleDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CHexSampleDlg::CHexSampleDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_HEXSAMPLE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIconW(IDR_MAINFRAME);
}

HEXCOLOR* CHexSampleDlg::GetColor(ULONGLONG ullOffset)
{
	//Sample code for custom colors:
	if (ullOffset < 18)
	{
		static std::vector<HEXCOLOR> vec {
			{ RGB(50, 0, 0), RGB(255, 255, 255) },
			{ RGB(0, 150, 0), RGB(255, 255, 255) },
			{ RGB(255, 255, 0), RGB(0, 0, 0) },
			{ RGB(0, 0, 50), RGB(255, 255, 255) },
			{ RGB(50, 50, 110), RGB(255, 255, 255) },
			{ RGB(50, 250, 50), RGB(255, 255, 255) },
			{ RGB(110, 0, 0), RGB(255, 255, 255) },
			{ RGB(0, 110, 0), RGB(255, 255, 255) },
			{ RGB(0, 0, 110), RGB(255, 255, 255) },
			{ RGB(110, 110, 0), RGB(255, 255, 255) },
			{ RGB(0, 110, 110), RGB(255, 255, 255) },
			{ RGB(110, 110, 110), RGB(255, 255, 255) },
			{ RGB(220, 0, 0), RGB(255, 255, 255) },
			{ RGB(0, 220, 0), RGB(255, 255, 255) },
			{ RGB(0, 0, 220), RGB(255, 255, 255) },
			{ RGB(220, 220, 0), RGB(255, 255, 255) },
			{ RGB(0, 220, 220), RGB(255, 255, 255) },
			{ RGB(0, 250, 0), RGB(255, 255, 255) }
		};
		return &vec[ullOffset];
	}

	return nullptr;
}

void CHexSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CHexSampleDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SETDATARO, &CHexSampleDlg::OnBnSetDataRO)
	ON_BN_CLICKED(IDC_SETDATARW, &CHexSampleDlg::OnBnSetDataRW)
	ON_BN_CLICKED(IDC_CLEARDATA, &CHexSampleDlg::OnBnClearData)
	ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CHexSampleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);	 //Set big icon
	SetIcon(m_hIcon, FALSE); //Set small icon

	m_myHex->CreateDialogCtrl(IDC_MY_HEX, m_hWnd);
	m_myHex->SetWheelRatio(0.5);
	//m_myHex->SetSectorSize(64);

	//Classical approach:
	//HEXCREATESTRUCT hcs;
	//hcs.hwndParent = m_hWnd;
	//hcs.uID = IDC_MY_HEX;
	//hcs.enCreateMode = EHexCreateMode::CREATE_CUSTOMCTRL;
	//hcs.enShowMode = EHexShowMode::ASDWORD;
	//m_myHex->Create(hcs);

	m_hds.pData = reinterpret_cast<std::byte*>(m_data);
	m_hds.ullDataSize = sizeof(m_data);
//	m_hds.pHexVirtColors = this;
//	m_hds.fHighLatency = true;

	return TRUE; //return TRUE  unless you set the focus to a control
}

void CHexSampleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessageW(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CHexSampleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CHexSampleDlg::OnBnSetDataRO()
{
	if (!m_myHex->IsDataSet())
	{
		m_hds.fMutable = false;
		m_myHex->SetData(m_hds);
	}
	m_myHex->SetMutable(false);
}

void CHexSampleDlg::OnBnSetDataRW()
{
	if (!m_myHex->IsDataSet())
	{
		m_hds.fMutable = true;
		m_myHex->SetData(m_hds);
	}
	m_myHex->SetMutable(true);
}

void CHexSampleDlg::OnBnClearData()
{
	m_myHex->ClearData();
}

void CHexSampleDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
}