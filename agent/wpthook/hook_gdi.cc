/******************************************************************************
Copyright (c) 2010, Google Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of the <ORGANIZATION> nor the names of its contributors 
    may be used to endorse or promote products derived from this software 
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#include "StdAfx.h"
#include "hook_gdi.h"
#include "test_state.h"

static CGDIHook * pHook = NULL;
extern bool wpt_capturing_screen;

/******************************************************************************
*******************************************************************************
**																			                                     **
**							              	Stub Functions								               **
**							                                    												 **
*******************************************************************************
******************************************************************************/

BOOL __stdcall EndPaint_Hook(HWND hWnd, CONST PAINTSTRUCT *lpPaint) {
  BOOL ret = FALSE;
  if(pHook)
    ret = pHook->EndPaint(hWnd, lpPaint);
  return ret;
}

int __stdcall ReleaseDC_Hook(HWND hWnd, HDC hDC) {
  int ret = 0;
  if(pHook)
    ret = pHook->ReleaseDC(hWnd, hDC);
  return ret;
}

BOOL __stdcall SetWindowTextA_Hook(HWND hWnd, LPCSTR text)
{
  BOOL ret = FALSE;
  if(pHook)
    ret = pHook->SetWindowTextA(hWnd, text);
  return ret;
}

BOOL __stdcall SetWindowTextW_Hook(HWND hWnd, LPCWSTR text)
{
  BOOL ret = FALSE;
  if(pHook)
    ret = pHook->SetWindowTextW(hWnd, text);
  return ret;
}

BOOL __stdcall InvalidateRect_Hook(HWND hWnd, const RECT *lpRect, BOOL bErase) {
  BOOL ret = false;
  if(pHook)
    ret = pHook->InvalidateRect(hWnd, lpRect, bErase);
  return ret;
}

BOOL __stdcall InvalidateRgn_Hook(HWND hWnd, HRGN hRgn, BOOL bErase) {
  BOOL ret = false;
  if(pHook)
    ret = pHook->InvalidateRgn(hWnd, hRgn, bErase);
  return ret;
}

int __stdcall DrawTextA_Hook(HDC hDC, LPCSTR lpchText, int nCount,
                             LPRECT lpRect, UINT uFormat) {
  int height = 0;
  if(pHook)
    height = pHook->DrawTextA(hDC, lpchText, nCount, lpRect, uFormat);
  return height;
}

int __stdcall DrawTextW_Hook(HDC hDC, LPCWSTR lpchText, int nCount,
                             LPRECT lpRect, UINT uFormat) {
  int height = 0;
  if(pHook)
    height = pHook->DrawTextW(hDC, lpchText, nCount, lpRect, uFormat);
  return height;
}

int __stdcall DrawTextExA_Hook(HDC hdc, LPSTR lpchText, int cchText, 
    LPRECT lpRect, UINT dwDTFormat, LPDRAWTEXTPARAMS lpDTParams) {
  int height = 0;
  if(pHook)
    height = pHook->DrawTextExA(hdc, lpchText, cchText, lpRect, dwDTFormat,
                                lpDTParams);
  return height;
}

int __stdcall DrawTextExW_Hook(HDC hdc, LPWSTR lpchText, int cchText, 
    LPRECT lpRect, UINT dwDTFormat, LPDRAWTEXTPARAMS lpDTParams) {
  int height = 0;
  if(pHook)
    height = pHook->DrawTextExW(hdc, lpchText, cchText, lpRect, dwDTFormat,
                                lpDTParams);
  return height;
}

BOOL __stdcall BitBlt_Hook(HDC hdcDest, int nXDest, int nYDest, int nWidth,
    int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop) {
  BOOL ret = false;
  if(pHook)
    ret = pHook->BitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc,
                        nXSrc, nYSrc, dwRop);
  return ret;
}

/******************************************************************************
*******************************************************************************
**																		                                    	 **
**								              CGDIHook Class							                 **
**															                                     				 **
*******************************************************************************
******************************************************************************/

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
CGDIHook::CGDIHook(TestState& test_state):
  test_state_(test_state) {
  document_windows_.InitHashTable(257);
  InitializeCriticalSection(&cs);
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void CGDIHook::Init() {
  if (!pHook)
    pHook = this;

  EndPaint_ = hook.createHookByName("user32.dll", "EndPaint", EndPaint_Hook);
  ReleaseDC_ = hook.createHookByName("user32.dll","ReleaseDC",ReleaseDC_Hook);
  SetWindowTextA_ = hook.createHookByName("user32.dll", "SetWindowTextA", 
                                          SetWindowTextA_Hook);
  SetWindowTextW_ = hook.createHookByName("user32.dll", "SetWindowTextW", 
                                          SetWindowTextW_Hook);
  InvalidateRect_ = hook.createHookByName("user32.dll", "InvalidateRect", 
                                          InvalidateRect_Hook);
//  InvalidateRgn_ = hook.createHookByName("user32.dll", "InvalidateRgn", 
//                                         InvalidateRgn_Hook);
  DrawTextA_ = hook.createHookByName("user32.dll", "DrawTextA", 
                                     DrawTextA_Hook);
  DrawTextW_ = hook.createHookByName("user32.dll", "DrawTextW", 
                                     DrawTextW_Hook);
  DrawTextExA_ = hook.createHookByName("user32.dll", "DrawTextExA", 
                                       DrawTextExA_Hook);
  DrawTextExW_ = hook.createHookByName("user32.dll", "DrawTextExW", 
                                       DrawTextExW_Hook);
  BitBlt_ = hook.createHookByName("gdi32.dll", "BitBlt", BitBlt_Hook);
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
CGDIHook::~CGDIHook(void) {
  DeleteCriticalSection(&cs);
  if (pHook == this)
    pHook = NULL;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void CGDIHook::SendPaintEvent(int x, int y, int width, int height) {
  x = max(x,0);
  y = max(y,0);
  height = max(height,0);
  width = max(width,0);
  if (!test_state_._exit && test_state_._active)
    test_state_.PaintEvent(x, y, width, height);

  if (test_state_.gdi_only_)
    PostMessage(HWND_BROADCAST, test_state_.paint_msg_,
                MAKEWPARAM(x,y), MAKELPARAM(width, height));
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
bool CGDIHook::IsDocumentWindow(HWND hWnd) {
  bool is_document = false;
  if (!document_windows_.Lookup(hWnd, is_document)) {
    is_document = IsBrowserDocument(hWnd);
    document_windows_.SetAt(hWnd, is_document);
  }

  if (hWnd && hWnd != test_state_._document_window && 
      !test_state_._exit && test_state_._active && is_document) {
    test_state_.SetDocument(hWnd);
  }

  return is_document;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
bool CGDIHook::IsDocumentDC(HDC hdc) {
  bool is_document = false;
  HWND hWnd = WindowFromDC(hdc);
  if (hWnd)
    is_document = IsDocumentWindow(hWnd);
  return is_document;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
BOOL CGDIHook::EndPaint(HWND hWnd, CONST PAINTSTRUCT *lpPaint) {
  BOOL ret = FALSE;

  if (EndPaint_)
    ret = EndPaint_(hWnd, lpPaint);

  if (IsDocumentWindow(hWnd)) {
    WORD x = 0, y = 0, width = 0, height = 0;
    if (lpPaint) {
      x = (WORD)lpPaint->rcPaint.left;
      y = (WORD)lpPaint->rcPaint.top;
      width = (WORD)abs(lpPaint->rcPaint.right - lpPaint->rcPaint.left);
      height = (WORD)abs(lpPaint->rcPaint.bottom - lpPaint->rcPaint.top);
    }
    SendPaintEvent(x, y, width, height);
  }

  return ret;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
int CGDIHook::ReleaseDC(HWND hWnd, HDC hDC)
{
  int ret = 0;

  if( ReleaseDC_ )
    ret = ReleaseDC_(hWnd, hDC);

  bool is_document = false;
  if (!document_windows_.Lookup(hWnd, is_document)) {
    is_document = IsBrowserDocument(hWnd);
    document_windows_.SetAt(hWnd, is_document);
  }

  if (hWnd && hWnd != test_state_._document_window && 
      !test_state_._exit && test_state_._active && is_document) {
    test_state_.SetDocument(hWnd);
  }

  if (!wpt_capturing_screen && !test_state_._exit && test_state_._active && 
      hWnd == test_state_._document_window) {
    test_state_._screen_updated = true;
    test_state_.CheckStartRender();
  }

  return ret;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
BOOL CGDIHook::SetWindowTextA(HWND hWnd, LPCSTR text)
{
  BOOL ret = false;

  if (SetWindowTextA_)
    ret = SetWindowTextA_(hWnd, text);

  if (!test_state_._exit && test_state_._active && 
        hWnd == test_state_._frame_window) {
    CString title((LPCTSTR)CA2T(text));
    if( title.Left(11) != _T("about:blank") && 
        title.Compare(_T("Blank")) )
      test_state_.TitleSet(title);
  }

  return ret;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
BOOL CGDIHook::SetWindowTextW(HWND hWnd, LPCWSTR text)
{
  BOOL ret = false;

  if (SetWindowTextW_)
    ret = SetWindowTextW_(hWnd, text);

  if (!test_state_._exit && test_state_._active && 
        hWnd == test_state_._frame_window) {
    CString title((LPCTSTR)CW2T(text));
    if( title.Left(11) != _T("about:blank") )
      test_state_.TitleSet(title);
  }

  return ret;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
BOOL CGDIHook::InvalidateRect(HWND hWnd, const RECT *lpRect, BOOL bErase) {
  BOOL ret = false;
  if (InvalidateRect_)
    ret = InvalidateRect_(hWnd, lpRect, bErase);
  if (IsDocumentWindow(hWnd)) {
    WORD x = 0, y = 0, width = 0, height = 0;
    if (lpRect) {
      x = (WORD)lpRect->left;
      y = (WORD)lpRect->top;
      width = (WORD)abs(lpRect->right - lpRect->left);
      height = (WORD)abs(lpRect->bottom - lpRect->top);
    }
    SendPaintEvent(x, y, width, height);
  }
  return ret;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
BOOL CGDIHook::InvalidateRgn(HWND hWnd, HRGN hRgn, BOOL bErase) {
  BOOL ret = false;
  AtlTrace(_T("InvalidateRgn (%d)\n"), hWnd);
  if (InvalidateRgn_)
    ret = InvalidateRgn_(hWnd, hRgn, bErase);
  return ret;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
int CGDIHook::DrawTextA(HDC hDC, LPCSTR lpchText, int nCount, LPRECT lpRect,
                        UINT uFormat) {
  int height = 0;
  if (DrawTextA_)
    height = DrawTextA_(hDC, lpchText, nCount, lpRect, uFormat);
  if (IsDocumentDC(hDC)) {
    WORD x = 0, y = 0, width = 0, height = 0;
    if (lpRect) {
      x = (WORD)lpRect->left;
      y = (WORD)lpRect->top;
      width = (WORD)abs(lpRect->right - lpRect->left);
      height = (WORD)abs(lpRect->bottom - lpRect->top);
    }
    SendPaintEvent(x, y, width, height);
  }
  return height;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
int CGDIHook::DrawTextW(HDC hDC, LPCWSTR lpchText, int nCount, LPRECT lpRect,
                        UINT uFormat) {
  int height = 0;
  if (DrawTextW_)
    height = DrawTextW_(hDC, lpchText, nCount, lpRect, uFormat);
  if (IsDocumentDC(hDC)) {
    WORD x = 0, y = 0, width = 0, height = 0;
    if (lpRect) {
      x = (WORD)lpRect->left;
      y = (WORD)lpRect->top;
      width = (WORD)abs(lpRect->right - lpRect->left);
      height = (WORD)abs(lpRect->bottom - lpRect->top);
    }
    SendPaintEvent(x, y, width, height);
  }
  return height;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
int CGDIHook::DrawTextExA(HDC hdc, LPSTR lpchText, int cchText, LPRECT lpRect,
                          UINT dwDTFormat, LPDRAWTEXTPARAMS lpDTParams) {
  int height = 0;
  if (DrawTextExA_)
    height = DrawTextExA_(hdc, lpchText, cchText, lpRect, dwDTFormat,
                          lpDTParams);
  if (IsDocumentDC(hdc)) {
    WORD x = 0, y = 0, width = 0, height = 0;
    if (lpRect) {
      x = (WORD)lpRect->left;
      y = (WORD)lpRect->top;
      width = (WORD)abs(lpRect->right - lpRect->left);
      height = (WORD)abs(lpRect->bottom - lpRect->top);
    }
    SendPaintEvent(x, y, width, height);
  }
  return height;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
int CGDIHook::DrawTextExW(HDC hdc, LPWSTR lpchText, int cchText, LPRECT lpRect,
                          UINT dwDTFormat, LPDRAWTEXTPARAMS lpDTParams) {
  int height = 0;
  if (DrawTextExW_)
    height = DrawTextExW_(hdc, lpchText, cchText, lpRect, dwDTFormat,
                          lpDTParams);
  if (IsDocumentDC(hdc)) {
    WORD x = 0, y = 0, width = 0, height = 0;
    if (lpRect) {
      x = (WORD)lpRect->left;
      y = (WORD)lpRect->top;
      width = (WORD)abs(lpRect->right - lpRect->left);
      height = (WORD)abs(lpRect->bottom - lpRect->top);
    }
    SendPaintEvent(x, y, width, height);
  }
  return height;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
BOOL CGDIHook::BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth,
    int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop) {
  BOOL ret = false;
  if (BitBlt_)
    ret = BitBlt_(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc,
                  nYSrc, dwRop);
  if (IsDocumentDC(hdcDest))
    SendPaintEvent(nXDest, nYDest, nWidth, nHeight);
  return ret;
}
