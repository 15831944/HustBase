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

// 关联创建数据库按钮，此处应提示用户输入数据库的存储路径和名称，并调用CreateDB函数创建数据库。
// 【用路径选择对话框创建新的数据库】
void CHustBaseApp::OnCreateDB()
{
	//AfxMessageBox("测试：创建数据库");

	BROWSEINFO bi;
	LPITEMIDLIST lpDlist;
	char selectedPath[20];
	char path[MAX_PATH];
	char db_path[MAX_PATH];

	SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &lpDlist);
	if (lpDlist == NULL) return;

	ZeroMemory(&bi, sizeof(BROWSEINFO));
	bi.hwndOwner = GetForegroundWindow();		// 父窗口	
	bi.pidlRoot = lpDlist;						// 使用桌面目录
	bi.lpszTitle = "创建数据库";					// 标题
	bi.pszDisplayName = selectedPath;			// 保存用户选中的目录字符串的内存地址
	bi.ulFlags = 0x0040;						// 增添新建文件夹按钮

	LPCITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if (pidl != NULL) {
		SHGetPathFromIDList(pidl, path);

		//AfxMessageBox(path);
		//AfxMessageBox(selectedPath);

		char *pos = strrchr(path, '\\');
		*pos = '\0';
		RC rc = CreateDB(path, selectedPath);
		if (rc != SUCCESS) {
			AfxMessageBox("创建数据库失败！");
			return;
		}
		CoTaskMemFree((LPVOID)pidl);			//释放pIDL所指向内存空间
	}
}

// 关联打开数据库按钮，此处应提示用户输入数据库所在位置，并调用OpenDB函数改变当前数据库路径，
// 并在界面左侧的控件中显示数据库中的表、列信息。
void CHustBaseApp::OnOpenDB() 
{
	//AfxMessageBox("测试：打开数据库");

	BROWSEINFO bi;
	LPITEMIDLIST lpDlist;
	char path[MAX_PATH];

	SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &lpDlist);
	if (lpDlist == NULL) return;

	ZeroMemory(&bi, sizeof(bi));
	bi.pidlRoot = lpDlist;						// 使用桌面目录
	bi.lpszTitle = "打开数据库";					// 标题

	LPCITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if (pidl != NULL) {
		SHGetPathFromIDList(pidl, path);

		// 检查数据库，是否有SYSTABLES和SYSCOLUMNS
		SetCurrentDirectory(path);
		CFileFind fileFind;
		BOOL table_exist = fileFind.FindFile("SYSTABLES");
		BOOL colmn_exist = fileFind.FindFile("SYSCOLUMNS");
		if (!table_exist || !colmn_exist) {
			AfxMessageBox("数据库格式错误！不存在系统文件！");
			return;
		}

		RC rc = OpenDB(path);
		if (rc != SUCCESS) {
			AfxMessageBox("打开数据库失败！");
			return;
		}

		// 显示数据库结构
		CHustBaseApp::pathvalue = true;
		CHustBaseDoc *pDoc;
		pDoc = CHustBaseDoc::GetDoc();
		pDoc->m_pTreeView->PopulateTree();

		CoTaskMemFree((LPVOID)pidl);			//释放pIDL所指向内存空间
	}
}

// 关联删除数据库按钮，此处应提示用户输入数据库所在位置，并调用DropDB函数删除数据库的内容。
void CHustBaseApp::OnDropDb() 
{
	// AfxMessageBox("测试：删除数据库");

	BROWSEINFO bi;
	LPITEMIDLIST lpDlist = NULL;
	char path[MAX_PATH];

	SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &lpDlist);
	if (lpDlist == NULL) return;

	ZeroMemory(&bi, sizeof(BROWSEINFO));
	bi.hwndOwner = GetForegroundWindow();		// 父窗口	
	bi.pidlRoot = lpDlist;						// 使用桌面目录
	bi.lpszTitle = "删除数据库";					// 标题
	bi.pszDisplayName = NULL;

	LPCITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if (pidl != NULL) {
		SHGetPathFromIDList(pidl, path);

		// 检查数据库，是否有SYSTABLES和SYSCOLUMNS
		// 不能用SetCurrentDirectory(path)，否则无法删除当前正在使用的目录
		CFileFind fileFind;
		char check_path[MAX_PATH];
		strcpy(check_path, path);
		strcat(check_path, "\\SYSTABLES");
		BOOL table_exist = fileFind.FindFile(check_path);
		strcpy(check_path, path);
		strcat(check_path, "\\SYSCOLUMNS");
		BOOL colmn_exist = fileFind.FindFile(check_path);
		if (!table_exist || !colmn_exist) {
			AfxMessageBox("数据库格式错误！不存在系统文件！");
			return;
		}

		// 检查是否为当前打开的数据库
		char cur_db[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, cur_db);
		if (strcmp(cur_db, path) == 0) {
			AfxMessageBox("无法删除当前打开的数据库！");
			return;
		}

		//CHustBaseApp::pathvalue = false;
		RC rc = DropDB(path);
		if (rc != SUCCESS) {
			AfxMessageBox("删除数据库失败！");
			return;
		}

		CoTaskMemFree((LPVOID)pidl);			//释放pIDL所指向内存空间
	}
}
