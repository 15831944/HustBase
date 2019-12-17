// HustBase.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "HustBase.h"

#include "MainFrm.h"
#include "HustBaseDoc.h"
#include "HustBaseView.h"
#include "TreeList.h"

#include "IX_Manager.h"
#include "PF_Manager.h"
#include "RM_Manager.h"
#include "SYS_Manager.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHustBaseApp

BEGIN_MESSAGE_MAP(CHustBaseApp, CWinApp)
	//{{AFX_MSG_MAP(CHustBaseApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_CREATEDB, OnCreateDB)
	ON_COMMAND(ID_OPENDB, OnOpenDB)
	ON_COMMAND(ID_DROPDB, OnDropDb)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHustBaseApp construction

CHustBaseApp::CHustBaseApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CHustBaseApp object

CHustBaseApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CHustBaseApp initialization
bool CHustBaseApp::pathvalue=false;

BOOL CHustBaseApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CHustBaseDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CHustBaseView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CHustBaseApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CHustBaseApp message handlers

// 12/16
// �����������ݿⰴť���˴�Ӧ��ʾ�û��������ݿ�Ĵ洢·�������ƣ�������CreateDB�����������ݿ⡣
// ����·��ѡ��Ի��򴴽��µ����ݿ⡿
void CHustBaseApp::OnCreateDB()
{
	//AfxMessageBox("���ԣ��������ݿ�");

	BROWSEINFO bi;
	char szPath[MAX_PATH];

	ZeroMemory(&bi, sizeof(BROWSEINFO));
	bi.hwndOwner = GetForegroundWindow();		// ������	
	bi.pidlRoot = NULL;							// Ĭ��ʹ������Ŀ¼
	bi.lpszTitle = "�������ݿ�";					// ����
	bi.pszDisplayName = szPath;					// �����û�ѡ�е�Ŀ¼�ַ������ڴ��ַ
	bi.ulFlags = 0x0040;						// �����½��ļ��а�ť
	
	char *dbpath, *dbname;
	CString str;

	LPCITEMIDLIST pidl = SHBrowseForFolder(&bi);

	if (pidl == NULL) {
		AfxMessageBox("�������ݿ�ʧ�ܣ�");
		return;
	} else {
		SHGetPathFromIDList(pidl, str.GetBuffer(MAX_PATH * 2));
		str.ReleaseBuffer();
		dbpath = str.GetBuffer(0);
		dbname = szPath;

		//AfxMessageBox(dbpath);
		//AfxMessageBox(dbname);

		RC rc = CreateDB(dbpath, dbname);
		if (rc != SUCCESS) {
			AfxMessageBox("�������ݿ�ʧ�ܣ�");
			return;
		}
	}
}

void CHustBaseApp::OnOpenDB() 
{
	//���������ݿⰴť���˴�Ӧ��ʾ�û��������ݿ�����λ�ã�������OpenDB�����ı䵱ǰ���ݿ�·�������ڽ������Ŀؼ�����ʾ���ݿ��еı�����Ϣ��
	AfxMessageBox("���ԣ������ݿ�");

	/*
	���飺�ѵ�ǰ·����λ��Ҫ�򿪵����ݿ����ڵ��ļ��У�SetCurrentDirectory����Ȼ���ȡ���ļ�����SYSTABLES��SYSCOLUMNS�ļ������ݣ�
	�ڽ��������ʾ��ǰ���ݿ�Ľṹ����ʾ����ͨ������PopulateTree()������ʵ�֣��������Ϊ��
						CHustBaseDoc *pDoc;
					pDoc = CHustBaseDoc::GetDoc();
					CHustBaseApp::pathvalue = true;
						pDoc->m_pTreeView->PopulateTree();
	*/

	BROWSEINFO bi;
	char path[MAX_PATH];

	ZeroMemory(&bi, sizeof(bi));
	bi.pidlRoot = NULL;
	LPCITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if (pidl != NULL) {
		SHGetPathFromIDList(pidl, path);
	}

	SetCurrentDirectory(path);

	// ������ݿ��ʽ���Ƿ���SYSTABLES��SYSCOLUMNS
	CFileFind fileFind;
	BOOL table_Exist = (BOOL)fileFind.FindFile("SYSTABLES");
	BOOL columns_Exist = (BOOL)fileFind.FindFile("SYSCOLUMNS");
	if (!table_Exist || !columns_Exist) {
		AfxMessageBox("���ݿ��ʽ���󣡲�����ϵͳ��");
		return;
	}

	CHustBaseApp::pathvalue = true;
	CHustBaseDoc *pDoc;
	pDoc = CHustBaseDoc::GetDoc();
	pDoc->m_pTreeView->PopulateTree();

	RC rc = OpenDB(path);
	if (rc != SUCCESS) {
		return;
	}
}

void CHustBaseApp::OnDropDb() 
{
	//����ɾ�����ݿⰴť���˴�Ӧ��ʾ�û��������ݿ�����λ�ã�������DropDB����ɾ�����ݿ�����ݡ�
	AfxMessageBox("���ԣ�ɾ�����ݿ�");

	//���飺ɾ��ָ�����ݿ������ļ����е��������ݿ��ļ� ��ֱ��ɾ�������ݿ��ļ���
	BROWSEINFO bi;
	LPITEMIDLIST lpDlist = NULL;
	char szPath[MAX_PATH];
	char *dbName;
	RC rc;

	SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &lpDlist);
	if (lpDlist == NULL) return;

	ZeroMemory(&bi, sizeof(BROWSEINFO));//���������ͻ����
	bi.hwndOwner = GetForegroundWindow();//������	
	bi.pidlRoot = lpDlist;
	bi.lpszTitle = "Delete Database";
	bi.pszDisplayName = szPath;

	lpDlist = SHBrowseForFolder(&bi);
	if (lpDlist != NULL)
	{
		SHGetPathFromIDList(lpDlist, szPath);//���ļ���·��ȡ����

		CHustBaseApp::pathvalue = false; dbName = szPath;
		rc = DropDB(dbName);
		if (rc != SUCCESS)return;
	}
}
