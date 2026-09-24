
// RFT_IF_CANDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "RFT_IF_UART.h"
#include "RFT_IF_UARTDlg.h"
#include "afxdialogex.h"

#include "RT_Console_Rev0.1.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")


LARGE_INTEGER g_st, g_ed, g_freq;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define NUM_OF_SAMPLE_FOR_GRAPH (500)
#define WAIT_TIMEOUT	(100)	// for waiting response packet
#define WAIT_SLEEP_TIME (50)	// for waiting response packet


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRFT_IF_CANDlg 대화 상자



CRFT_IF_CANDlg::CRFT_IF_CANDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRFT_IF_CANDlg::IDD, pParent)
	, m_strRxData(_T(""))
	, m_nCOM_PortNumber(1)
	, m_fDivider_Force(50.0)
	, m_fDivider_Torque(1000.0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRFT_IF_CANDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_BAUDRATE, m_combo_baudrate);
	DDX_Text(pDX, IDC_EDIT_RX_DATA, m_strRxData);
	DDX_Control(pDX, IDC_CUSTOM_CHART_FORCE, m_ChartCtrl_Force);
	DDX_Control(pDX, IDC_CUSTOM_CHART_MOMENT, m_ChartCtrl_Torque); //
	DDX_Text(pDX, IDC_EDIT_COM_PORT_NUMBER, m_nCOM_PortNumber);
	DDX_Control(pDX, IDC_COMBO_FILTER_TYPE, m_combo_Filter_Cutoff_Frq);
	DDX_Text(pDX, IDC_EDIT_DIVIDER_FORCE, m_fDivider_Force);
	DDX_Text(pDX, IDC_EDIT_DIVIDER_TORQUE, m_fDivider_Torque);
	DDX_Control(pDX, IDC_COMBO_CFG_BRATE, m_cmb_cfg_baudrate);
	DDX_Control(pDX, IDC_COMBO_CFG_OUT_RATE, m_cmb_cfg_sensor_out_rate);
	DDX_Control(pDX, IDC_EDIT_CMD_STATE, m_edit_status);
	DDX_Control(pDX, IDC_COMBO_FILTER_MA_COUNT, m_combo_Filter_MovingAverage_Count);
}

BEGIN_MESSAGE_MAP(CRFT_IF_CANDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CHECK_INTERFACE_OPEN, &CRFT_IF_CANDlg::OnBnClickedCheckInterfaceOpen)
	ON_BN_CLICKED(IDC_CHECK_BIAS, &CRFT_IF_CANDlg::OnBnClickedCheckBias)
	ON_BN_CLICKED(IDC_CHECK_FT_OUT_CONT, &CRFT_IF_CANDlg::OnBnClickedCheckFtOutCont)	
	ON_BN_CLICKED(IDC_BUTTON_FILTER_SETTING, &CRFT_IF_CANDlg::OnBnClickedButtonFilterSetting)
	ON_BN_CLICKED(IDC_CHECK_DATA_LOGGING, &CRFT_IF_CANDlg::OnBnClickedCheckDataLogging)
	ON_BN_CLICKED(IDOK, &CRFT_IF_CANDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_SET_BAUDRATE, &CRFT_IF_CANDlg::OnBnClickedSetBaudrate)
	ON_BN_CLICKED(IDC_SET_OUTRATE, &CRFT_IF_CANDlg::OnBnClickedSetOutrate)
	ON_CBN_SELCHANGE(IDC_COMBO_FILTER_TYPE, &CRFT_IF_CANDlg::OnCbnSelchangeComboFilterType)
END_MESSAGE_MAP()


// CRFT_IF_CANDlg 메시지 처리기

BOOL CRFT_IF_CANDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.
	///////////////////////////////////////////////////////////////////////////////
	// Process priority setting
	if (::SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) // 본 프로그램을 최고 프라이어티 클래스로...
	{
		CONSOLE_S(CONSOLE_RED, "\n======== Priority setting OK.. =========\n");
	}
	// priority 설정이 잘못 되었더라도... 
	timeBeginPeriod(1); // increase time resolution

	
	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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
	
	m_cmb_cfg_baudrate.InsertString(0, "115.2Kbps");
	m_cmb_cfg_baudrate.InsertString(1, "921.6Kbps");
	m_cmb_cfg_baudrate.InsertString(2, "460.8Kbps");
	m_cmb_cfg_baudrate.InsertString(3, "230.4Kbps");
	m_cmb_cfg_baudrate.InsertString(4, "115.2Kbps");
	m_cmb_cfg_baudrate.InsertString(5, " 57.6Kbps");
	m_cmb_cfg_baudrate.SetCurSel(0);

	m_cmb_cfg_sensor_out_rate.InsertString(0, " 200Hz");
	m_cmb_cfg_sensor_out_rate.InsertString(1, "  10Hz");
	m_cmb_cfg_sensor_out_rate.InsertString(2, "  20Hz");
	m_cmb_cfg_sensor_out_rate.InsertString(3, "  50Hz");
	m_cmb_cfg_sensor_out_rate.InsertString(4, " 100Hz");
	m_cmb_cfg_sensor_out_rate.InsertString(5, " 200Hz");
	m_cmb_cfg_sensor_out_rate.InsertString(6, " 333Hz");
	m_cmb_cfg_sensor_out_rate.InsertString(7, " 500Hz");
	m_cmb_cfg_sensor_out_rate.InsertString(8, "1000Hz");
	m_cmb_cfg_sensor_out_rate.SetCurSel(0);



	m_combo_baudrate.InsertString(0, "57.6Kbps");
	m_combo_baudrate.InsertString(1, "115.2Kbps");
	m_combo_baudrate.InsertString(2, "230.4Kbps");
	m_combo_baudrate.InsertString(3, "460.8Kbps");
	m_combo_baudrate.InsertString(4, "921.6Kbps");
	m_combo_baudrate.SetCurSel(1);

	m_combo_Filter_Cutoff_Frq.InsertString(0, "No filtering");
	m_combo_Filter_Cutoff_Frq.InsertString(1, "500Hz");
	m_combo_Filter_Cutoff_Frq.InsertString(2, "300Hz");
	m_combo_Filter_Cutoff_Frq.InsertString(3, "200Hz");
	m_combo_Filter_Cutoff_Frq.InsertString(4, "150Hz");
	m_combo_Filter_Cutoff_Frq.InsertString(5, "100Hz");
	m_combo_Filter_Cutoff_Frq.InsertString(6, "50Hz");
	m_combo_Filter_Cutoff_Frq.InsertString(7, "40Hz");
	m_combo_Filter_Cutoff_Frq.InsertString(8, "30Hz");
	m_combo_Filter_Cutoff_Frq.InsertString(9, "20Hz");
	m_combo_Filter_Cutoff_Frq.InsertString(10, "10Hz");
	m_combo_Filter_Cutoff_Frq.InsertString(11, "5Hz");
	m_combo_Filter_Cutoff_Frq.InsertString(12, "3Hz");
	m_combo_Filter_Cutoff_Frq.InsertString(13, "2Hz");
	m_combo_Filter_Cutoff_Frq.InsertString(14, "1Hz");
	m_combo_Filter_Cutoff_Frq.SetCurSel(0);

	m_combo_Filter_MovingAverage_Count.InsertString(0, "No Filtering");
	
	CString str;
	for (int i = 1; i < 50; i++)
	{
		str.Format("%02d Samples", i+1);
		m_combo_Filter_MovingAverage_Count.InsertString(i, str);
	}
	m_combo_Filter_MovingAverage_Count.SetCurSel(0);

	GetDlgItem(IDC_CHECK_FT_OUT_CONT)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK_BIAS)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_FILTER_SETTING)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK_DATA_LOGGING)->EnableWindow(FALSE);

	// callback function setting
	m_RFT_IF.setCallback(callback_RFT_Data_Receive, this);

	// for graph
	CChartStandardAxis* pBottomAxis = m_ChartCtrl_Force.CreateStandardAxis(CChartCtrl::BottomAxis);
	pBottomAxis->SetMinMax(0, NUM_OF_SAMPLE_FOR_GRAPH);
	pBottomAxis->SetAutomatic(true);
	CChartStandardAxis* pLeftAxis = m_ChartCtrl_Force.CreateStandardAxis(CChartCtrl::LeftAxis);
	pLeftAxis->SetMinMax(-10, 10);
	pLeftAxis->SetAutomatic(true);
	m_ChartCtrl_Force.SetBackColor(RGB(255,255,255)); // white back-ground color
	m_ChartCtrl_Force.GetLegend()->SetVisible(true);

	m_pGraph_F[0] = m_ChartCtrl_Force.CreateLineSerie(false, false);
	m_pGraph_F[0]->SetWidth(1);
	m_pGraph_F[0]->SetPenStyle(0);
	m_pGraph_F[0]->SetName("Fx[N]");
	m_pGraph_F[0]->SetColor(RGB(255, 0, 0));

	m_pGraph_F[1] = m_ChartCtrl_Force.CreateLineSerie(false, false);
	m_pGraph_F[1]->SetWidth(1);
	m_pGraph_F[1]->SetPenStyle(0);
	m_pGraph_F[1]->SetName("Fy[N]");
	m_pGraph_F[1]->SetColor(RGB(0, 255, 0));

	m_pGraph_F[2] = m_ChartCtrl_Force.CreateLineSerie(false, false);
	m_pGraph_F[2]->SetWidth(1);
	m_pGraph_F[2]->SetPenStyle(0);
	m_pGraph_F[2]->SetName("Fz[N]");
	m_pGraph_F[2]->SetColor(RGB(0, 0, 255));


	CChartStandardAxis* pBottomAxis2 = m_ChartCtrl_Torque.CreateStandardAxis(CChartCtrl::BottomAxis);
	pBottomAxis2->SetMinMax(0, NUM_OF_SAMPLE_FOR_GRAPH);
	pBottomAxis2->SetAutomatic(true);
	CChartStandardAxis* pLeftAxis2 = m_ChartCtrl_Torque.CreateStandardAxis(CChartCtrl::LeftAxis);
	pLeftAxis2->SetMinMax(-10, 10);
	pLeftAxis2->SetAutomatic(true);
	m_ChartCtrl_Torque.SetBackColor(RGB(255, 255, 255)); // white back-ground color
	m_ChartCtrl_Torque.GetLegend()->SetVisible(true);

	m_pGraph_F[3] = m_ChartCtrl_Torque.CreateLineSerie(false, false);
	m_pGraph_F[3]->SetWidth(1);
	m_pGraph_F[3]->SetPenStyle(0);
	m_pGraph_F[3]->SetName("Tx[Nm]");
	m_pGraph_F[3]->SetColor(RGB(255, 0, 0));

	m_pGraph_F[4] = m_ChartCtrl_Torque.CreateLineSerie(false, false);
	m_pGraph_F[4]->SetWidth(1);
	m_pGraph_F[4]->SetPenStyle(0);
	m_pGraph_F[4]->SetName("Ty[Nm]");
	m_pGraph_F[4]->SetColor(RGB(0, 255, 0));

	m_pGraph_F[5] = m_ChartCtrl_Torque.CreateLineSerie(false, false);
	m_pGraph_F[5]->SetWidth(1);
	m_pGraph_F[5]->SetPenStyle(0);
	m_pGraph_F[5]->SetName("Tz[Nm]");
	m_pGraph_F[5]->SetColor(RGB(0, 0, 255));

	::QueryPerformanceFrequency(&g_freq);

	SetTimer(0, 100, NULL);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CRFT_IF_CANDlg::OnSysCommand(UINT nID, LPARAM lParam)
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
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CRFT_IF_CANDlg::OnPaint()
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
HCURSOR CRFT_IF_CANDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRFT_IF_CANDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}



void CRFT_IF_CANDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	::QueryPerformanceCounter(&g_st); // get start time.

	if (m_RFT_IF.m_nCurrMode == CMD_FT_CONT)
	{
		m_strRxData.Format("froce<%.03f %.03f %.03f> torque<%.03f %.03f %.03f> overload[0x%02X] measure_err:[0x%02X]",
			m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[0], m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[1], m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[2],
			m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[3], m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[4], m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[5],
			m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForceStatus, m_RFT_IF.m_RFT_IF_PACKET.m_measure_error);

		//CONSOLE_S("%s\n", m_strRxData.GetBuffer());
		if (m_RFT_IF.m_bIsEnabled_Callback == false)
		{
			m_vGraphDatas_F[0].push_back(m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[0]);
			m_vGraphDatas_F[1].push_back(m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[1]);
			m_vGraphDatas_F[2].push_back(m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[2]);
			m_vGraphDatas_F[3].push_back(m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[3]);
			m_vGraphDatas_F[4].push_back(m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[4]);
			m_vGraphDatas_F[5].push_back(m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[5]);
		}


		GetDlgItem(IDC_EDIT_RX_DATA)->SetWindowTextA(m_strRxData);
		m_ChartCtrl_Force.EnableRefresh(false);
		m_ChartCtrl_Torque.EnableRefresh(false);
#if 1
		for (int i = 0; i < RFT_NUM_OF_FORCE; i++)
		{
			int size = m_vGraphDatas_F[i].size();
			double *x = new double[size];
			double *y = new double[size];

			for (int idx = 0; idx < size; idx++)
			{
				x[idx] = idx;
				y[idx] = m_vGraphDatas_F[i][idx];
			}

			m_pGraph_F[i]->SetPoints(x, y, size);

			delete[] x;
			delete[] y;

			if (size > NUM_OF_SAMPLE_FOR_GRAPH)
			{
				int del_size = (size - NUM_OF_SAMPLE_FOR_GRAPH);
				m_vGraphDatas_F[i].erase(m_vGraphDatas_F[i].begin(), m_vGraphDatas_F[i].begin() + del_size);
			}
		}
#endif
		m_ChartCtrl_Force.EnableRefresh(true);
		m_ChartCtrl_Torque.EnableRefresh(true);

		//m_ChartCtrl_Force.RefreshCtrl();
		//m_ChartCtrl_Torque.RefreshCtrl();

	}

	::QueryPerformanceCounter(&g_ed); // get end time
	double passedTime = ((double)g_ed.QuadPart - g_st.QuadPart) / ((double)g_freq.QuadPart);
	//CONSOLE_S("on timer: %f\n", passedTime);

	CDialogEx::OnTimer(nIDEvent);
}


BOOL CRFT_IF_CANDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	int YesNo = 0;
	bool isForcedReturn = false;
	switch (pMsg->message)    /// 종료 키보드 메세지 리턴
	{
	case WM_KEYDOWN:
		if ((pMsg->wParam == VK_ESCAPE) | (pMsg->wParam == VK_RETURN))
			isForcedReturn = true;
		break;

	case WM_SYSKEYDOWN:    // Alt + F4 메세지 처리
		if (pMsg->wParam == VK_F4)
		{
			YesNo = AfxMessageBox("Do you want to exit?", MB_YESNO, NULL);

			if (YesNo == IDYES)
			{
				DestroyWindow();
			}

			isForcedReturn = true;
		}
		break;
	case WM_LBUTTONDOWN:
		//CONSOLE_S("MAIN DLG BN_CLICKED\n");
		break;
	default:
		break;
	}

	if (isForcedReturn)
		return TRUE;

	return CDialogEx::PreTranslateMessage(pMsg);
}



void CRFT_IF_CANDlg::OnBnClickedCheckInterfaceOpen()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	UpdateData();

	bool isOn = (bool)((CButton*)GetDlgItem(IDC_CHECK_INTERFACE_OPEN))->GetCheck();
	if (isOn)
	{
		int baudrateIdx = m_combo_baudrate.GetCurSel();
		int baudRate;
		if (baudrateIdx == 0)
			baudRate = 57600;
		else if (baudrateIdx == 1)
			baudRate = 115200;
		else if (baudrateIdx == 2)
			baudRate = 230400;
		else if (baudrateIdx == 3)
			baudRate = 460800;
		else if (baudrateIdx == 4)
			baudRate = 921600;

		bool enableCallback = true;


		if (m_RFT_IF.openPort(m_fDivider_Force, m_fDivider_Torque, m_nCOM_PortNumber, baudRate, 8, ONESTOPBIT, NOPARITY, FC_NONE, enableCallback))
		{
			((CButton*)GetDlgItem(IDC_CHECK_INTERFACE_OPEN))->SetWindowTextA("Close COM Port");

			GetDlgItem(IDC_CHECK_FT_OUT_CONT)->EnableWindow(TRUE);
			GetDlgItem(IDC_CHECK_BIAS)->EnableWindow(TRUE);
			GetDlgItem(IDC_BUTTON_FILTER_SETTING)->EnableWindow(TRUE);
			GetDlgItem(IDC_CHECK_DATA_LOGGING)->EnableWindow(TRUE);
			GetDlgItem(IDC_SET_BAUDRATE)->EnableWindow(TRUE);
			GetDlgItem(IDC_SET_OUTRATE)->EnableWindow(TRUE);
		}
		else
		{
			((CButton*)GetDlgItem(IDC_CHECK_INTERFACE_OPEN))->SetCheck(0);
			((CButton*)GetDlgItem(IDC_CHECK_INTERFACE_OPEN))->SetWindowTextA("Open COM Port");

			GetDlgItem(IDC_CHECK_FT_OUT_CONT)->EnableWindow(FALSE);
			GetDlgItem(IDC_CHECK_BIAS)->EnableWindow(FALSE);
			GetDlgItem(IDC_BUTTON_FILTER_SETTING)->EnableWindow(FALSE);
			GetDlgItem(IDC_CHECK_DATA_LOGGING)->EnableWindow(FALSE);
			GetDlgItem(IDC_SET_BAUDRATE)->EnableWindow(FALSE);
			GetDlgItem(IDC_SET_OUTRATE)->EnableWindow(FALSE);
		}
	}
	else
	{
		((CButton*)GetDlgItem(IDC_CHECK_INTERFACE_OPEN))->SetWindowTextA("Open COM Port");
		GetDlgItem(IDC_CHECK_FT_OUT_CONT)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_BIAS)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_FILTER_SETTING)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_DATA_LOGGING)->EnableWindow(FALSE);
		m_RFT_IF.closePort();
	}
}



void CRFT_IF_CANDlg::callback_RFT_Data_Receive( void *callbackParam)
{

	CRFT_IF_CANDlg *pThisClass = (CRFT_IF_CANDlg*)callbackParam;

	if (pThisClass->m_RFT_IF.m_nCurrMode == CMD_FT_CONT)
	{
		// for graph.... 그래프는 on timer에서 그린다.
		pThisClass->m_vGraphDatas_F[0].push_back(pThisClass->m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[0]);
		pThisClass->m_vGraphDatas_F[1].push_back(pThisClass->m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[1]);
		pThisClass->m_vGraphDatas_F[2].push_back(pThisClass->m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[2]);
		pThisClass->m_vGraphDatas_F[3].push_back(pThisClass->m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[3]);
		pThisClass->m_vGraphDatas_F[4].push_back(pThisClass->m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[4]);
		pThisClass->m_vGraphDatas_F[5].push_back(pThisClass->m_RFT_IF.m_RFT_IF_PACKET.m_rcvdForce[5]);
	}
}


void CRFT_IF_CANDlg::OnBnClickedCheckBias()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	if (((CButton*)GetDlgItem(IDC_CHECK_BIAS))->GetCheck())
	{
		m_RFT_IF.set_FT_Bias(1);
	}
	else
	{
		m_RFT_IF.set_FT_Bias(0);
	}
}

void CRFT_IF_CANDlg::OnBnClickedCheckFtOutCont()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	if (((CButton*)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->GetCheck())
	{
		m_RFT_IF.rqst_FT_Continuous();

		GetDlgItem(IDC_BUTTON_FILTER_SETTING)->EnableWindow(FALSE);
		GetDlgItem(IDC_SET_BAUDRATE)->EnableWindow(FALSE);
		GetDlgItem(IDC_SET_OUTRATE)->EnableWindow(FALSE);
	}
	else
	{
		if (m_RFT_IF.m_RFT_IF_PACKET.m_measure_error) // error가 발생한 경우에는 명령을 한번더 보내고 버튼 복원
		{
			m_RFT_IF.rqst_FT_Continuous();
			((CButton*)GetDlgItem(IDC_CHECK_FT_OUT_CONT))->SetCheck(TRUE);
		}
		else
		{
			m_RFT_IF.rqst_FT_Stop();
			Sleep(500);

			GetDlgItem(IDC_BUTTON_FILTER_SETTING)->EnableWindow(TRUE);
			GetDlgItem(IDC_SET_BAUDRATE)->EnableWindow(TRUE);
			GetDlgItem(IDC_SET_OUTRATE)->EnableWindow(TRUE);
		}
	}
}




void CRFT_IF_CANDlg::OnBnClickedButtonFilterSetting()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	UpdateData();

	int filterSel = m_combo_Filter_Cutoff_Frq.GetCurSel();
	int MA_filterCount = m_combo_Filter_MovingAverage_Count.GetCurSel();

	unsigned char param[3] = { 0, 0, 0 };

	if (filterSel != 0)
	{
		param[0] = 1;
		param[1] = filterSel;
		param[2] = MA_filterCount;
	}
	else
	{
		param[0] = 0;
		param[1] = 0;
		param[2] = 0;
	}
#ifdef NEW_FILTER
	m_RFT_IF.set_FT_Filter_Type(param[0], param[1], param[2]);
#else
	m_RFT_IF.set_FT_Filter_Type(param[0], param[1]);
#endif
	int waitTimeOutCnt = 0;

	do{
		Sleep(WAIT_SLEEP_TIME);
		waitTimeOutCnt++;
		if (waitTimeOutCnt >= WAIT_TIMEOUT)
			break;
	} while (m_RFT_IF.m_bIsRcvd_Response_Pkt == false);

	// 여기서 데이터 처리.....
	if (m_RFT_IF.m_bIsRcvd_Response_Pkt)
	{
		if (m_RFT_IF.m_RFT_IF_PACKET.m_response_result != 1)
		{
			m_strRxData.Format("F/T Filter Setting Error[Error Code: %d]", m_RFT_IF.m_RFT_IF_PACKET.m_response_errcode);
		}
		else
		{
			m_strRxData.Format("F/T Filter Setting - Successed");			
		}

		GetDlgItem(IDC_EDIT_RX_DATA)->SetWindowTextA(m_strRxData);
	}
	else
	{
		m_strRxData = "Time out - Receive response packet";
		GetDlgItem(IDC_EDIT_RX_DATA)->SetWindowTextA(m_strRxData);
	}
}


void CRFT_IF_CANDlg::OnBnClickedCheckDataLogging()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	bool isTrue = ((CButton*)GetDlgItem(IDC_CHECK_DATA_LOGGING))->GetCheck();

	if (isTrue)
	{
		if (m_RFT_IF.startLogging("RFT"))
		{
			GetDlgItem(IDC_CHECK_DATA_LOGGING)->SetWindowTextA("Stop Data Logging");
		}
		else
		{
			GetDlgItem(IDC_CHECK_DATA_LOGGING)->SetWindowTextA("Start Data Logging");
			((CButton*)GetDlgItem(IDC_CHECK_DATA_LOGGING))->SetCheck(0);
		}
	}
	else
	{
		m_RFT_IF.stopLogging();
		GetDlgItem(IDC_CHECK_DATA_LOGGING)->SetWindowTextA("Start Data Logging");
	}
}


void CRFT_IF_CANDlg::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CDialogEx::OnOK();
}


void CRFT_IF_CANDlg::OnBnClickedConfig()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	
}


void CRFT_IF_CANDlg::OnBnClickedSendCmd()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CRFT_IF_CANDlg::OnBnClickedSetBaudrate()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	int cfgBaudRateSel = m_cmb_cfg_baudrate.GetCurSel();
	unsigned char param[2] = { 0, 0 };
	param[0] = CMD_SET_COMM_BAUDRATE;
	param[1] = cfgBaudRateSel;

	m_RFT_IF.set_Comm_Speed(param[1]);
	int waitTimeOutCnt = 0;
	do {
		Sleep(WAIT_SLEEP_TIME);
		waitTimeOutCnt++;
		if (waitTimeOutCnt >= WAIT_TIMEOUT)
			break;
	} while (m_RFT_IF.m_bIsRcvd_Response_Pkt == false);

	// 여기서 데이터 처리.....
	if (m_RFT_IF.m_bIsRcvd_Response_Pkt)
	{		
		if (m_RFT_IF.m_RFT_IF_PACKET.m_response_result == 1)
			m_strRxData.Format("Settings Completed : \r\nApplied after Reboot");
		else
			m_strRxData.Format("Setting Fail : \r\n[Error Code: %d]", m_RFT_IF.m_RFT_IF_PACKET.m_response_errcode);

		GetDlgItem(IDC_EDIT_CMD_STATE)->SetWindowTextA(m_strRxData);
	}
	else
	{
		m_strRxData = "Time out - Receive response packet";
		GetDlgItem(IDC_EDIT_CMD_STATE)->SetWindowTextA(m_strRxData);
	}
}


void CRFT_IF_CANDlg::OnBnClickedSetOutrate()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	int cfgOutRateSel = m_cmb_cfg_sensor_out_rate.GetCurSel();
	unsigned char param[2] = { 0, 0 };
	param[0] = CMD_SET_CONT_OUT_FRQ;
	param[1] = cfgOutRateSel;

	m_RFT_IF.set_FT_Cont_Interval(param[1]);
	int waitTimeOutCnt = 0;
	do {
		Sleep(WAIT_SLEEP_TIME);
		waitTimeOutCnt++;
		if (waitTimeOutCnt >= WAIT_TIMEOUT)
			break;
	} while (m_RFT_IF.m_bIsRcvd_Response_Pkt == false);

	// 여기서 데이터 처리.....
	if (m_RFT_IF.m_bIsRcvd_Response_Pkt)
	{
		if (m_RFT_IF.m_RFT_IF_PACKET.m_response_result == 1)
		{
			CString str = "";			
			m_cmb_cfg_sensor_out_rate.GetLBText(m_cmb_cfg_sensor_out_rate.GetCurSel(), str);
			m_strRxData.Format("Settings Completed : \r\n%s", str);
		}
		else
			m_strRxData.Format("Settings Failed : [Error Code: %d]", m_RFT_IF.m_RFT_IF_PACKET.m_response_errcode);

		GetDlgItem(IDC_EDIT_CMD_STATE)->SetWindowTextA(m_strRxData);
	}
	else
	{
		m_strRxData = "Time out - Receive response packet";
		GetDlgItem(IDC_EDIT_CMD_STATE)->SetWindowTextA(m_strRxData);
	}
}


void CRFT_IF_CANDlg::OnCbnSelchangeComboFilterType()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (m_combo_Filter_Cutoff_Frq.GetCurSel() == 0)
	{
		m_combo_Filter_MovingAverage_Count.EnableWindow(0);
	}
	else
	{
		m_combo_Filter_MovingAverage_Count.EnableWindow(1);
	}
}
