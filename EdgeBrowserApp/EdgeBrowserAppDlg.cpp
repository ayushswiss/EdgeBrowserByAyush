//Author: Ayush Chaudhary
//Email: ayush.shyam@gmail.com
//License: Free license 
//Description: In this application I show how to use webview2 edge browser in yoor dialog based applications.
// Very easy implementation of webview

#include "stdafx.h"
#include "afxdialogex.h"
//#include "framework.h"
#include "EdgeBrowserApp.h"
#include "EdgeBrowserAppDlg.h"
#include "winuser.h"
#include "ViewComponent.h"
//#include "pch.h"
#include <ShObjIdl_core.h>
#include <Shellapi.h>
#include <ShlObj_core.h>
//#include <wrl.h>
//#include <winrt/windows.system.h>

#ifdef __windows__
#undef __windows__
#endif

static constexpr UINT s_runAsyncWindowMessage = WM_APP;

// CEdgeBrowserAppDlg dialog

CEdgeBrowserAppDlg::CEdgeBrowserAppDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_EDGEBROWSERAPP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    g_hInstance = GetModuleHandle(NULL);
   
}

void CEdgeBrowserAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CEdgeBrowserAppDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_WM_SIZE()
END_MESSAGE_MAP()

void CEdgeBrowserAppDlg::OnSize(UINT a, int b, int c)
{
    ResizeEverything();
}

// CEdgeBrowserAppDlg message handlers

BOOL CEdgeBrowserAppDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
    
    HRESULT hresult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    SetWindowLongPtr(this->GetSafeHwnd(), GWLP_USERDATA, (LONG_PTR)this);

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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
    InitializeWebView();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

LRESULT CALLBACK CEdgeBrowserAppDlg::WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (auto app = (CEdgeBrowserAppDlg*)GetWindowLongPtr(hWnd, GWLP_USERDATA))
    {
        LRESULT result = 0;
        if (app->HandleWindowMessage(hWnd, message, wParam, lParam, &result))
        {
            return result;
        }
    }
   // DefWindowProc()
    return CDialog::DefWindowProc(message, wParam, lParam);
    //return TRUE;
}

void CEdgeBrowserAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CEdgeBrowserAppDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

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
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CEdgeBrowserAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
// Register the Win32 window class for the app window.

void CEdgeBrowserAppDlg::ResizeEverything()
{
    RECT availableBounds = { 0 };
    GetClientRect(&availableBounds);
   // ClientToScreen(&availableBounds);

    if (auto view = GetComponent<ViewComponent>())
    {
        view->SetBounds(availableBounds);
    }
}


bool CEdgeBrowserAppDlg::HandleWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    // Give all components a chance to handle the message first.
    for (auto& component : m_components)
    {
        if (component->HandleWindowMessage(hWnd, message, wParam, lParam, result))
        {
            return true;
        }
    }

    switch (message)
    {
    case WM_SIZE:
    {
        // Don't resize the app or webview when the app is minimized
        // let WM_SYSCOMMAND to handle it
        if (lParam != 0)
        {
            ResizeEverything();
            return true;
        }
    }
    case WM_CLOSE:
    {
        PostQuitMessage(0);
    }
    break;
    //! [DPIChanged]
    case WM_DPICHANGED:
    {       
        return true;
    }
    break;
    //! [DPIChanged]
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(&ps);
        EndPaint(&ps);
        return true;
    }
    break;
    case s_runAsyncWindowMessage:
    {
        auto* task = reinterpret_cast<std::function<void()>*>(wParam);
        (*task)();
        delete task;
        return true;
    }
    break;
    case WM_NCDESTROY:
    {
        int retValue = 0;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
        // NotifyClosed();
         /*if (--s_appInstances == 0)
         {
             PostQuitMessage(retValue);
         }*/
    }
    break;
    case WM_QUERYENDSESSION:
    {
        // yes, we can shut down
        // Register how we might be restarted
        RegisterApplicationRestart(L"--restore", RESTART_NO_CRASH | RESTART_NO_HANG);
        *result = TRUE;
        return true;
    }
    break;
    case WM_ENDSESSION:
    {
        if (wParam == TRUE)
        {
            // save app state and exit.
            PostQuitMessage(0);
            return true;
        }
    }
    break;
    case WM_KEYDOWN:
    {
        TRACE("KEY DOWN");
        // If bit 30 is set, it means the WM_KEYDOWN message is autorepeated.
        // We want to ignore it in that case.

    }
    break;
    case WM_COMMAND:
    {
        return ExecuteWebViewCommands(wParam, lParam) || ExecuteAppCommands(wParam, lParam);
    }
    break;
    }
    return false;
}

bool CEdgeBrowserAppDlg::ExecuteWebViewCommands(WPARAM wParam, LPARAM lParam)
{
    if (!m_webView)
        return false;

    switch (LOWORD(wParam))
    {
        case IDM_GET_BROWSER_VERSION_AFTER_CREATION:
        {
            //! [GetBrowserVersionString]
            wil::unique_cotaskmem_string version_info;
            m_webViewEnvironment->get_BrowserVersionString(&version_info);
            AfxMessageBox(L"Browser Version Info After WebView Creation", MB_OK);
            return true;
        }    
    }
    return false;
}

// Handle commands not related to the WebView, which will work even if the WebView
// is not currently initialized.
bool CEdgeBrowserAppDlg::ExecuteAppCommands(WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
        case IDM_GET_BROWSER_VERSION_BEFORE_CREATION:
        {
            wil::unique_cotaskmem_string version_info;
            GetAvailableCoreWebView2BrowserVersionString(nullptr, &version_info);
            AfxMessageBox(L"Browser Version Info Before WebView Creation",MB_OK);
            return true;
        }
    }
    return false;
}

void CEdgeBrowserAppDlg::RunAsync(std::function<void()> callback)
{
    auto* task = new std::function<void()>(callback);
    PostMessage(s_runAsyncWindowMessage, reinterpret_cast<WPARAM>(task), 0);
}

void CEdgeBrowserAppDlg::InitializeWebView()
{

    CloseWebView();
    m_dcompDevice = nullptr;


    HRESULT hr2 = DCompositionCreateDevice2(nullptr, IID_PPV_ARGS(&m_dcompDevice));
    if (!SUCCEEDED(hr2))
    {
        AfxMessageBox(L"Attempting to create WebView using DComp Visual is not supported.\r\n"
            "DComp device creation failed.\r\n"
            "Current OS may not support DComp.\r\n"
            "Create with Windowless DComp Visual Failed", MB_OK);
        return;
    }


#ifdef USE_WEBVIEW2_WIN10
    m_wincompCompositor = nullptr;
#endif
    LPCWSTR subFolder = nullptr;
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    options->put_AllowSingleSignOnUsingOSPrimaryAccount(FALSE);


    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(subFolder, nullptr, options.Get(), Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(this, &CEdgeBrowserAppDlg::OnCreateEnvironmentCompleted).Get());

    if (!SUCCEEDED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            TRACE("Couldn't find Edge installation. Do you have a version installed that is compatible with this ");
        }
        else
        {
            AfxMessageBox(L"Failed to create webview environment");
        }
    }
}

HRESULT CEdgeBrowserAppDlg::DCompositionCreateDevice2(IUnknown* renderingDevice, REFIID riid, void** ppv)
{
    HRESULT hr = E_FAIL;
    static decltype(::DCompositionCreateDevice2)* fnCreateDCompDevice2 = nullptr;
    if (fnCreateDCompDevice2 == nullptr)
    {
        HMODULE hmod = ::LoadLibraryEx(L"dcomp.dll", nullptr, 0);
        if (hmod != nullptr)
        {
            fnCreateDCompDevice2 = reinterpret_cast<decltype(::DCompositionCreateDevice2)*>(
                ::GetProcAddress(hmod, "DCompositionCreateDevice2"));
        }
    }
    if (fnCreateDCompDevice2 != nullptr)
    {
        hr = fnCreateDCompDevice2(renderingDevice, riid, ppv);
    }
    return hr;
}

void CEdgeBrowserAppDlg::CloseWebView(bool cleanupUserDataFolder)
{

    if (m_controller)
    {
        m_controller->Close();
        m_controller = nullptr;
        m_webView = nullptr;
    }
    m_webViewEnvironment = nullptr;
    if (cleanupUserDataFolder)
    {
        //Clean user data        
    }
}

HRESULT CEdgeBrowserAppDlg::OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment)
{
    m_webViewEnvironment = environment;
    auto webViewExperimentalEnvironment = m_webViewEnvironment.try_query<ICoreWebView2ExperimentalEnvironment>();
#ifdef USE_WEBVIEW2_WIN10
    if (webViewExperimentalEnvironment && (m_dcompDevice || m_wincompCompositor))
#else
    if (webViewExperimentalEnvironment && m_dcompDevice)
#endif
    {
        webViewExperimentalEnvironment->CreateCoreWebView2CompositionController(this->GetSafeHwnd(),
            Microsoft::WRL::Callback<ICoreWebView2ExperimentalCreateCoreWebView2CompositionControllerCompletedHandler>(
                [this](
                    HRESULT result,
                    ICoreWebView2ExperimentalCompositionController* compositionController) -> HRESULT {
                        auto controller =
                            wil::com_ptr<ICoreWebView2ExperimentalCompositionController>(compositionController)
                            .query<ICoreWebView2Controller>();
                        return OnCreateCoreWebView2ControllerCompleted(result, controller.get());
                }).Get());
    }
    else
    {
        m_webViewEnvironment->CreateCoreWebView2Controller(
            this->GetSafeHwnd(), Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(this, &CEdgeBrowserAppDlg::OnCreateCoreWebView2ControllerCompleted).Get());
    }

    return S_OK;
}

HRESULT CEdgeBrowserAppDlg::OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller)
{
    if (result == S_OK)
    {
        m_controller = controller;
        wil::com_ptr<ICoreWebView2> coreWebView2;
        m_controller->get_CoreWebView2(&coreWebView2);
        coreWebView2.query_to(&m_webView);

        NewComponent<ViewComponent>(
            this, m_dcompDevice.get(),
#ifdef USE_WEBVIEW2_WIN10
            m_wincompCompositor,
#endif
            m_creationModeId == IDM_CREATION_MODE_TARGET_DCOMP);

           HRESULT hresult = m_webView->Navigate(L"https://ayushshyam.wixsite.com/perdesijaat");

        if (hresult == S_OK)
        {
            TRACE("Web Page Opened Successfully");
            ResizeEverything();
        }

    }
    else
    {
        TRACE("Failed to create webview");
    }
    return S_OK;
}


