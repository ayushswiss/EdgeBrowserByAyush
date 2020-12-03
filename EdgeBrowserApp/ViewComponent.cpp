// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ViewComponent.h"
//#include "DCompTargetImpl.h"

//#include <d2d1helper.h>
#include <sstream>
#include <windowsx.h>
#ifdef USE_WEBVIEW2_WIN10
#include <windows.ui.composition.interop.h>
#endif

//#include "CheckFailure.h"

using namespace Microsoft::WRL;
//namespace numerics = winrt::Windows::Foundation::Numerics;
//static D2D1_MATRIX_4X4_F Convert3x2MatrixTo4x4Matrix(D2D1_MATRIX_3X2_F* matrix3x2);

ViewComponent::ViewComponent(
    CEdgeBrowserAppDlg* appWindow,
    IDCompositionDevice* dcompDevice,
#ifdef USE_WEBVIEW2_WIN10
    winrtComp::Compositor wincompCompositor,
#endif
    bool isDcompTargetMode)
    : m_appWindow(appWindow), m_controller(appWindow->GetWebViewController()),
      m_webView(appWindow->GetWebView()), m_dcompDevice(dcompDevice),
#ifdef USE_WEBVIEW2_WIN10
      m_wincompCompositor(wincompCompositor),
#endif
      m_isDcompTargetMode(isDcompTargetMode)
{
  
    m_controller->add_ZoomFactorChanged(
        Callback<ICoreWebView2ZoomFactorChangedEventHandler>(
            [this](ICoreWebView2Controller* sender, IUnknown* args) -> HRESULT {
                double zoomFactor;
                sender->get_ZoomFactor(&zoomFactor);

                std::wstring message = L"WebView2APISample (Zoom: " +
                    std::to_wstring(int(zoomFactor * 100)) + L"%)";
                SetWindowText(m_appWindow->GetMainWindow(), message.c_str());
                return S_OK;
            })
        .Get(),
                &m_zoomFactorChangedToken);
    //! [ZoomFactorChanged]

    // Set up compositor if we're running in windowless mode
    m_compositionController = m_controller.try_query<ICoreWebView2ExperimentalCompositionController>();
    if (m_compositionController)
    {
        if (m_dcompDevice)
        {
            //! [SetRootVisualTarget]
            // Set the host app visual that the WebView will connect its visual
            // tree to.
            BuildDCompTreeUsingVisual();
            if (m_isDcompTargetMode)
            {
               /* if (!m_dcompTarget)
                {
                    m_dcompTarget = Make<DCompTargetImpl>(this);
                }*/
               
                  // m_compositionController->put_RootVisualTarget(m_dcompTarget.get());
            }
            else
            {
                
                   // m_compositionController->put_RootVisualTarget(m_dcompWebViewVisual.get());
            }
            m_dcompDevice->Commit();
            //! [SetRootVisualTarget]
        }
#ifdef USE_WEBVIEW2_WIN10
        else if (m_wincompCompositor)
        {
            BuildWinCompVisualTree();
            CHECK_FAILURE(m_compositionController->put_RootVisualTarget(m_wincompWebViewVisual.as<IUnknown>().get()));
        }
#endif
        else
        {
            FAIL_FAST();
        }
        //! [CursorChanged]
        // Register a handler for the CursorChanged event.
        m_compositionController->add_CursorChanged(
            Callback<ICoreWebView2ExperimentalCursorChangedEventHandler>(
                [this](ICoreWebView2ExperimentalCompositionController* sender, IUnknown* args)
                    -> HRESULT {
                    HRESULT hr = S_OK;
                    HCURSOR cursor;
                    if (!m_useCursorId)
                    {
                        sender->get_Cursor(&cursor);
                    }
                    else
                    {
                        //! [SystemCursorId]
                        UINT32 cursorId;
                        wil::com_ptr<ICoreWebView2ExperimentalCompositionController2> compositionController2 =
                            m_controller.query<ICoreWebView2ExperimentalCompositionController2>();
                        compositionController2->get_SystemCursorId(&cursorId);
                        cursor = ::LoadCursor(nullptr, MAKEINTRESOURCE(cursorId));
                        if (cursor == nullptr)
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                        }
                        //! [SystemCursorId]
                    }

                    if (SUCCEEDED(hr))
                    {
                        SetClassLongPtr(
                            m_appWindow->GetMainWindow(), GCLP_HCURSOR, (LONG_PTR)cursor);
                    }
                    return hr;
                })
                .Get(),
            &m_cursorChangedToken);
        //! [CursorChanged]
    }
#ifdef USE_WEBVIEW2_WIN10
    else if (m_dcompDevice || m_wincompCompositor)
#else
    else if (m_dcompDevice)
#endif
    {
        //FAIL_FAST();
    }

    ResizeWebView();
}
bool ViewComponent::HandleWindowMessage(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    if (message == WM_COMMAND)
    {
        
        /*
        switch (LOWORD(wParam))
        {
        case IDM_TOGGLE_VISIBILITY:
            ToggleVisibility();
            return true;
        case IDM_ZOOM_05:
            SetZoomFactor(0.5f);
            return true;
        case IDM_ZOOM_10:
            SetZoomFactor(1.0f);
            return true;
        case IDM_ZOOM_20:
            SetZoomFactor(2.0f);
            return true;
        case IDM_SIZE_25:
            SetSizeRatio(0.5f);
            return true;
        case IDM_SIZE_50:
            SetSizeRatio(0.7071f);
            return true;
        case IDM_SIZE_75:
            SetSizeRatio(0.866f);
            return true;
        case IDM_SIZE_100:
            SetSizeRatio(1.0f);
            return true;
        case IDM_TRANSFORM_NONE:
            SetTransform(TransformType::kIdentity);
            return true;
        case IDM_TRANSFORM_ROTATE_30DEG:
            SetTransform(TransformType::kRotate30Deg);
            return true;
        case IDM_TRANSFORM_ROTATE_60DEG_DIAG:
            SetTransform(TransformType::kRotate60DegDiagonally);
            return true;
        case IDM_SCALE_50:
            SetScale(0.5f);
            return true;
        case IDM_SCALE_100:
            SetScale(1.0f);
            return true;
        case IDM_SCALE_125:
            SetScale(1.25f);
            return true;
        case IDM_SCALE_150:
            SetScale(1.5f);
            return true;
        case IDM_GET_WEBVIEW_BOUNDS:
            ShowWebViewBounds();
            return true;
        case IDM_GET_WEBVIEW_ZOOM:
            ShowWebViewZoom();
            return true;
        case IDM_TOGGLE_CURSOR_HANDLING:
            m_useCursorId = !m_useCursorId;
            return true;
             }*/
    }
       
    //! [ToggleIsVisibleOnMinimize]
    if (message == WM_SYSCOMMAND)
    {
        if (wParam == SC_MINIMIZE)
        {
            // Hide the webview when the app window is minimized.
            m_controller->put_IsVisible(FALSE);
        }
        else if (wParam == SC_RESTORE)
        {
            // When the app window is restored, show the webview
            // (unless the user has toggle visibility off).
            if (m_isVisible)
            {
                m_controller->put_IsVisible(TRUE);
            }
        }
    }
    //! [ToggleIsVisibleOnMinimize]
    if ((message >= WM_MOUSEFIRST && message <= WM_MOUSELAST) || message == WM_MOUSELEAVE)
    {
        OnMouseMessage(message, wParam, lParam);
    }
    else if (
        message == WM_POINTERACTIVATE || message == WM_POINTERDOWN ||
        message == WM_POINTERENTER || message == WM_POINTERLEAVE || message == WM_POINTERUP ||
        message == WM_POINTERUPDATE)
    {
        OnPointerMessage(message, wParam, lParam);
    }
    //! [NotifyParentWindowPositionChanged]
    if (message == WM_MOVE || message == WM_MOVING)
    {
        m_controller->NotifyParentWindowPositionChanged();
        return true;
    }
    //! [NotifyParentWindowPositionChanged]
    return false;
}
//! [ToggleIsVisible]
void ViewComponent::ToggleVisibility()
{
    BOOL visible;
    m_controller->get_IsVisible(&visible);
    m_isVisible = !visible;
    m_controller->put_IsVisible(m_isVisible);
}
//! [ToggleIsVisible]

void ViewComponent::SetSizeRatio(float ratio)
{
    m_webViewRatio = ratio;
    ResizeWebView();
}

void ViewComponent::SetZoomFactor(float zoom)
{
    m_webViewZoomFactor = zoom;
    m_controller->put_ZoomFactor(zoom);
}

void ViewComponent::SetBounds(RECT bounds)
{
    m_webViewBounds = bounds;
    ResizeWebView();
}

//! [SetBoundsAndZoomFactor]
void ViewComponent::SetScale(float scale)
{
    RECT bounds;
    m_controller->get_Bounds(&bounds);
    double scaleChange = scale / m_webViewScale;

    bounds.bottom = LONG(
        (bounds.bottom - bounds.top) * scaleChange + bounds.top);
    bounds.right = LONG(
        (bounds.right - bounds.left) * scaleChange + bounds.left);

    m_webViewScale = scale;
    m_controller->SetBoundsAndZoomFactor(bounds, scale);
}
//! [SetBoundsAndZoomFactor]

//! [ResizeWebView]
// Update the bounds of the WebView window to fit available space.
void ViewComponent::ResizeWebView()
{
    SIZE webViewSize = {
            LONG((m_webViewBounds.right - m_webViewBounds.left) * m_webViewRatio * m_webViewScale),
            LONG((m_webViewBounds.bottom - m_webViewBounds.top) * m_webViewRatio * m_webViewScale) };

    RECT desiredBounds = m_webViewBounds;
    desiredBounds.bottom = LONG(
        webViewSize.cy + m_webViewBounds.top);
    desiredBounds.right = LONG(
        webViewSize.cx + m_webViewBounds.left);

    m_controller->put_Bounds(desiredBounds);
    if (m_compositionController)
    {
        POINT webViewOffset = {m_webViewBounds.left, m_webViewBounds.top};

        if (m_dcompDevice)
        {
            m_dcompRootVisual->SetOffsetX(float(webViewOffset.x));
            m_dcompRootVisual->SetOffsetY(float(webViewOffset.y));
            m_dcompRootVisual->SetClip(
                {0, 0, float(webViewSize.cx), float(webViewSize.cy)});
            m_dcompDevice->Commit();
        }
#ifdef USE_WEBVIEW2_WIN10
        else if (m_wincompCompositor)
        {
            if (m_wincompRootVisual != nullptr)
            {
                numerics::float2 size = {static_cast<float>(webViewSize.cx),
                                         static_cast<float>(webViewSize.cy)};
                m_wincompRootVisual.Size(size);

                numerics::float3 offset = {static_cast<float>(webViewOffset.x),
                                           static_cast<float>(webViewOffset.y), 0.0f};
                m_wincompRootVisual.Offset(offset);

                winrtComp::IInsetClip insetClip = m_wincompCompositor.CreateInsetClip();
                m_wincompRootVisual.Clip(insetClip.as<winrtComp::CompositionClip>());
            }
        }
#endif
    }
}
//! [ResizeWebView]

// Show the current bounds of the WebView.
void ViewComponent::ShowWebViewBounds()
{
    RECT bounds;
    HRESULT result = m_controller->get_Bounds(&bounds);
    if (SUCCEEDED(result))
    {
        std::wstringstream message;
        message << L"Left:\t" << bounds.left << L"\n"
                << L"Top:\t" << bounds.top << L"\n"
                << L"Right:\t" << bounds.right << L"\n"
                << L"Bottom:\t" << bounds.bottom << std::endl;
        MessageBox(nullptr, message.str().c_str(), L"WebView Bounds", MB_OK);
    }
}

// Show the current zoom factor of the WebView.
void ViewComponent::ShowWebViewZoom()
{
    double zoomFactor;
    HRESULT result = m_controller->get_ZoomFactor(&zoomFactor);
    if (SUCCEEDED(result))
    {
        std::wstringstream message;
        message << L"Zoom Factor:\t" << zoomFactor << std::endl;
        MessageBox(nullptr, message.str().c_str(), L"WebView Zoom Factor", MB_OK);
    }
}

void ViewComponent::SetTransform(TransformType transformType)
{
    if (!m_compositionController)
    {
        MessageBox(
            nullptr,
            L"Setting transform is not supported in windowed mode."
            "Choose a windowless mode for creation before trying to apply a transform.",
            L"Applying transform failed.", MB_OK);
        return;
    }

//    if (transformType == TransformType::kRotate30Deg)
//    {
//        D2D1_POINT_2F center = D2D1::Point2F(
//            (m_webViewBounds.right - m_webViewBounds.left) / 2.f,
//            (m_webViewBounds.bottom - m_webViewBounds.top) / 2.f);
//        m_webViewTransformMatrix =
//            Convert3x2MatrixTo4x4Matrix(&D2D1::Matrix3x2F::Rotation(30.0f, center));
//    }
//    else if (transformType == TransformType::kRotate60DegDiagonally)
//    {
//        m_webViewTransformMatrix = D2D1::Matrix4x4F::RotationArbitraryAxis(
//            float(m_webViewBounds.right), float(m_webViewBounds.bottom), 0, 60);
//    }
//    else if (transformType == TransformType::kIdentity)
//    {
//        m_webViewTransformMatrix = D2D1::Matrix4x4F();
//    }
//
//#ifdef USE_WEBVIEW2_WIN10
//    if (m_dcompDevice && !m_wincompCompositor)
//#else
//    if (m_dcompDevice)
//#endif
//    {
//        wil::com_ptr<IDCompositionVisual3> dcompWebViewVisual3;
//        m_dcompWebViewVisual->QueryInterface(IID_PPV_ARGS(&dcompWebViewVisual3));
//        dcompWebViewVisual3->SetTransform(m_webViewTransformMatrix);
//        m_dcompDevice->Commit();
//    }
#ifdef USE_WEBVIEW2_WIN10
    else if (m_wincompCompositor && !m_dcompDevice)
    {
        if (m_wincompWebViewVisual != nullptr)
        {
            m_wincompWebViewVisual.TransformMatrix(
                *reinterpret_cast<numerics::float4x4*>(&m_webViewTransformMatrix));
        }
    }
#endif
    else
    {
        FAIL_FAST();
    }
}

//static D2D1_MATRIX_4X4_F Convert3x2MatrixTo4x4Matrix(D2D1_MATRIX_3X2_F* matrix3x2)
//{
//    D2D1_MATRIX_4X4_F matrix4x4 = D2D1::Matrix4x4F();
//    matrix4x4._11 = matrix3x2->m11;
//    matrix4x4._12 = matrix3x2->m12;
//    matrix4x4._21 = matrix3x2->m21;
//    matrix4x4._22 = matrix3x2->m22;
//    matrix4x4._41 = matrix3x2->dx;
//    matrix4x4._42 = matrix3x2->dy;
//    return matrix4x4;
//}

//! [SendMouseInput]
bool ViewComponent::OnMouseMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    // Manually relay mouse messages to the WebView
#ifdef USE_WEBVIEW2_WIN10
    if (m_dcompDevice || m_wincompCompositor)
#else
    if (m_dcompDevice)
#endif
    {
        POINT point;
        POINTSTOPOINT(point, lParam);
        if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
        {
            // Mouse wheel messages are delivered in screen coordinates.
            // SendMouseInput expects client coordinates for the WebView, so convert
            // the point from screen to client.
            ::ScreenToClient(m_appWindow->GetMainWindow(), &point);
        }
        // Send the message to the WebView if the mouse location is inside the
        // bounds of the WebView, if the message is telling the WebView the
        // mouse has left the client area, or if we are currently capturing
        // mouse events.
        bool isMouseInWebView = PtInRect(&m_webViewBounds, point);
        if (isMouseInWebView || message == WM_MOUSELEAVE || m_isCapturingMouse)
        {
            DWORD mouseData = 0;

            switch (message)
            {
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
                mouseData = GET_WHEEL_DELTA_WPARAM(wParam);
                break;
            case WM_XBUTTONDBLCLK:
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
                mouseData = GET_XBUTTON_WPARAM(wParam);
                break;
            case WM_MOUSEMOVE:
                if (!m_isTrackingMouse)
                {
                    // WebView needs to know when the mouse leaves the client area
                    // so that it can dismiss hover popups. TrackMouseEvent will
                    // provide a notification when the mouse leaves the client area.
                    TrackMouseEvents(TME_LEAVE);
                    m_isTrackingMouse = true;
                }
                break;
            case WM_MOUSELEAVE:
                m_isTrackingMouse = false;
                break;
            }

            // We need to capture the mouse in case the user drags the
            // mouse outside of the window bounds and we still need to send
            // mouse messages to the WebView process. This is useful for
            // scenarios like dragging the scroll bar or panning a map.
            // This is very similar to the Pointer Message case where a
            // press started inside of the WebView.
            if (message == WM_LBUTTONDOWN || message == WM_MBUTTONDOWN ||
                message == WM_RBUTTONDOWN || message == WM_XBUTTONDOWN)
            {
                if (isMouseInWebView && ::GetCapture() != m_appWindow->GetMainWindow())
                {
                    m_isCapturingMouse = true;
                    ::SetCapture(m_appWindow->GetMainWindow());
                }
            }
            else if (message == WM_LBUTTONUP || message == WM_MBUTTONUP ||
                message == WM_RBUTTONUP || message == WM_XBUTTONUP)
            {
                if (::GetCapture() == m_appWindow->GetMainWindow())
                {
                    m_isCapturingMouse = false;
                    ::ReleaseCapture();
                }
            }

            // Adjust the point from app client coordinates to webview client coordinates.
            // WM_MOUSELEAVE messages don't have a point, so don't adjust the point.
            if (message != WM_MOUSELEAVE)
            {
                point.x -= m_webViewBounds.left;
                point.y -= m_webViewBounds.top;
            }

            m_compositionController->SendMouseInput(
                static_cast<COREWEBVIEW2_MOUSE_EVENT_KIND>(message),
                static_cast<COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS>(GET_KEYSTATE_WPARAM(wParam)),
                mouseData, point);
            return true;
        }
        else if (message == WM_MOUSEMOVE && m_isTrackingMouse)
        {
            // When the mouse moves outside of the WebView, but still inside the app
            // turn off mouse tracking and send the WebView a leave event.
            m_isTrackingMouse = false;
            TrackMouseEvents(TME_LEAVE | TME_CANCEL);
            OnMouseMessage(WM_MOUSELEAVE, 0, 0);
        }
    }
    return false;
}
//! [SendMouseInput]

bool ViewComponent::OnPointerMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    bool handled = false;
#ifdef USE_WEBVIEW2_WIN10
    if (m_dcompDevice || m_wincompCompositor)
#else
    if (m_dcompDevice)
#endif
    {
        POINT point;
        POINTSTOPOINT(point, lParam);
        UINT pointerId = GET_POINTERID_WPARAM(wParam);

        ::ScreenToClient(m_appWindow->GetMainWindow(), &point);

        bool pointerStartedInWebView = m_pointerIdsStartingInWebView.find(pointerId) !=
                                       m_pointerIdsStartingInWebView.end();
        // We want to send pointer input to the WebView for all pointers that either is in the
        // WebView or started inside the WebView. For example, if a user started a page scroll
        // inside of the WebView but dragged their finger outside of the WebView, we need to
        // keep sending pointer events for those pointers.
        if (PtInRect(&m_webViewBounds, point) || pointerStartedInWebView)
        {
            if (!pointerStartedInWebView &&
                (message == WM_POINTERENTER || message == WM_POINTERDOWN))
            {
                m_pointerIdsStartingInWebView.insert(pointerId);
            }
            else if (message == WM_POINTERLEAVE)
            {
                m_pointerIdsStartingInWebView.erase(pointerId);
            }

            handled = true;
            wil::com_ptr<ICoreWebView2ExperimentalPointerInfo> pointer_info;
           /* COREWEBVIEW2_MATRIX_4X4* webviewMatrix =
                reinterpret_cast<COREWEBVIEW2_MATRIX_4X4*>(&m_webViewTransformMatrix);
            m_compositionController->CreateCoreWebView2PointerInfoFromPointerId(
                pointerId, m_appWindow->GetMainWindow(), *webviewMatrix, &pointer_info);*/
            m_compositionController->SendPointerInput(
                static_cast<COREWEBVIEW2_POINTER_EVENT_KIND>(message), pointer_info.get());
        }
    }
    return handled;
}

void ViewComponent::TrackMouseEvents(DWORD mouseTrackingFlags)
{
    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(tme);
    tme.dwFlags = mouseTrackingFlags;
    tme.hwndTrack = m_appWindow->GetMainWindow();
    tme.dwHoverTime = 0;
    ::TrackMouseEvent(&tme);
}

//! [BuildDCompTree]
// Create host app visual that the WebView will connect to.
//   - Create a IDCompositionTarget for the host window
//   - Create a visual and set that as the IDCompositionTarget's root
//   - Create another visual and add that to the IDCompositionTarget's root.
//     This visual will be the visual root for the WebView.
void ViewComponent::BuildDCompTreeUsingVisual()
{
    
    if (m_dcompWebViewVisual == nullptr)
    {
        m_dcompDevice->CreateTargetForHwnd(
            m_appWindow->GetMainWindow(), TRUE, &m_dcompHwndTarget);
        m_dcompDevice->CreateVisual(&m_dcompRootVisual);
        m_dcompHwndTarget->SetRoot(m_dcompRootVisual.get());
        m_dcompDevice->CreateVisual(&m_dcompWebViewVisual);
        m_dcompRootVisual->AddVisual(m_dcompWebViewVisual.get(), TRUE, nullptr);
    }
}
//! [BuildDCompTree]

void ViewComponent::DestroyDCompVisualTree()
{
    if (m_dcompWebViewVisual)
    {
        m_dcompWebViewVisual->RemoveAllVisuals();
        m_dcompWebViewVisual.reset();

        m_dcompRootVisual->RemoveAllVisuals();
        m_dcompRootVisual.reset();

        m_dcompHwndTarget->SetRoot(nullptr);
        m_dcompHwndTarget.reset();

        m_dcompDevice->Commit();
    }

    /*if (m_dcompTarget)
    {
        m_dcompTarget->RemoveOwnerRef();
        m_dcompTarget = nullptr;
    }*/
}

#ifdef USE_WEBVIEW2_WIN10
void ViewComponent::BuildWinCompVisualTree()
{
    namespace abiComp = ABI::Windows::UI::Composition;

    if (m_wincompWebViewVisual == nullptr)
    {
        auto interop = m_wincompCompositor.as<abiComp::Desktop::ICompositorDesktopInterop>();
        winrt::check_hresult(interop->CreateDesktopWindowTarget(
            m_appWindow->GetMainWindow(), false,
            reinterpret_cast<abiComp::Desktop::IDesktopWindowTarget**>(winrt::put_abi(m_wincompHwndTarget))));

        m_wincompRootVisual = m_wincompCompositor.CreateContainerVisual();
        m_wincompHwndTarget.Root(m_wincompRootVisual);

        m_wincompWebViewVisual = m_wincompCompositor.CreateContainerVisual();
        m_wincompRootVisual.Children().InsertAtTop(m_wincompWebViewVisual);
    }
}

void ViewComponent::DestroyWinCompVisualTree()
{
    if (m_wincompWebViewVisual != nullptr)
    {
        m_wincompWebViewVisual.Children().RemoveAll();
        m_wincompWebViewVisual = nullptr;

        m_wincompRootVisual.Children().RemoveAll();
        m_wincompRootVisual = nullptr;

        m_wincompHwndTarget.Root(nullptr);
        m_wincompHwndTarget = nullptr;
    }
}
#endif

ViewComponent::~ViewComponent()
{
    m_controller->remove_ZoomFactorChanged(m_zoomFactorChangedToken);
    if (m_compositionController)
    {
        m_compositionController->remove_CursorChanged(m_cursorChangedToken);
        // If the webview closes because the AppWindow is closed (as opposed to being closed
        // explicitly), this will no-op because in this case, the webview closes before the ViewComponent
        // is destroyed. If the webview is closed explicitly, this will succeed.
        m_compositionController->put_RootVisualTarget(nullptr);
        DestroyDCompVisualTree();
#ifdef USE_WEBVIEW2_WIN10
        DestroyWinCompVisualTree();
#endif
    }
}
