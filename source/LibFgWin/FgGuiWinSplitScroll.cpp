//
// Copyright (c) 2015 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
// Authors:     Andrew Beatty
// Created:     March 22, 2011
//

#include "stdafx.h"

#include "FgGuiApiSplit.hpp"
#include "FgGuiWin.hpp"
#include "FgThrowWindows.hpp"
#include "FgMatrixC.hpp"
#include "FgBounds.hpp"
#include "FgDefaultVal.hpp"
#include "FgMetaFormat.hpp"
#include "FgAlgs.hpp"

using namespace std;

// Debug:
ostream &
operator<<(ostream & os,const SCROLLINFO & si)
{
    return os << "min: " << si.nMin
        << " max: " << si.nMax
        << " page: " << si.nPage
        << " pos: " << si.nPos
        << " trackPos " << si.nTrackPos;
}

struct  FgGuiWinSplitScroll : public FgGuiOsBase
{
    FgGuiApiSplitScroll         m_api;
    HWND                        hwndThis;
    FgGuiOsPtrs                 m_panes;
    FgVect2I                    m_client;   // doesn't include slider
    FgString                    m_store;
    SCROLLINFO                  m_si;

    FgGuiWinSplitScroll(const FgGuiApiSplitScroll & api) :
        m_api(api), hwndThis(0)
    {
        m_si.cbSize = sizeof(m_si);     // Never changes
        m_si.nMin = 0;                  // Never changes
        m_si.nPos = 0;                  // Initial value
        m_si.nTrackPos = 0;             // This value is always the same as the above value.
    }

    virtual void
    create(HWND parentHwnd,int ident,const FgString & store,DWORD extStyle,bool visible)
    {
//fgout << fgnl << "SplitScroll::create: visible: " << visible << " extStyle: " << extStyle << " ident: " << ident << fgpush;
        m_store = store;
        FgCreateChild   cc;
        cc.extStyle = extStyle;
        cc.style = WS_VSCROLL;
        cc.visible = visible;
        fgCreateChild(parentHwnd,ident,this,cc);
//fgout << fgpop;
    }

    virtual void
    destroy()
    {
        // Automatically destroys children first:
        DestroyWindow(hwndThis);
    }

    virtual FgVect2UI
    getMinSize() const
    {
        // Set the minimum scrollable size as the smaller of the maximum element
        // and a fixed maximum min:
        uint        sd = 1;
        FgVect2UI   ret = m_api.minSize;
        // Add scroll bar width:
        ret[1-sd] += GetSystemMetrics(SM_CXVSCROLL);
        return ret;
    }

    virtual FgVect2B
    wantStretch() const
    {
        for (size_t ii=0; ii<m_panes.size(); ++ii)
            if (m_panes[ii]->wantStretch()[0])
                return FgVect2B(true,true);
        return FgVect2B(false,true);
    }

    virtual void
    updateIfChanged()
    {
//fgout << fgnl << "SplitScroll::updateIfChanged";
        if (g_gg.dg.update(m_api.updateFlagIdx)) {
//fgout << " ... updating" << fgpush;
            // call DestroyWindow in all created sub-windows:
            for (size_t ii=0; ii<m_panes.size(); ++ii)
                m_panes[ii]->destroy();
            FgGuiPtrs            panes = m_api.getPanes();
            m_panes.resize(panes.size());
            for (size_t ii=0; ii<m_panes.size(); ++ii) {
                m_panes[ii] = panes[ii]->getInstance();
                m_panes[ii]->create(hwndThis,int(ii),m_store+"_"+fgToString(ii),0UL,true);
            }
//fgout << fgpop;
        }
        for (size_t ii=0; ii<m_panes.size(); ++ii)
            m_panes[ii]->updateIfChanged();
    }

    virtual void
    moveWindow(FgVect2I lo,FgVect2I sz)
    {
//fgout << fgnl << "SplitScroll::moveWindow: " << lo << "," << sz << fgpush;
        MoveWindow(hwndThis,lo[0],lo[1],sz[0],sz[1],FALSE);
//fgout << fgpop;
    }

    virtual void
    showWindow(bool s)
    {
//fgout << fgnl << "SplitScroll::showWindow: " << s << fgpush;
        ShowWindow(hwndThis,s ? SW_SHOW : SW_HIDE);
//fgout << fgpop;
    }

    virtual void
    saveState()
    {
        for (size_t ii=0; ii<m_panes.size(); ++ii)
            m_panes[ii]->saveState();
    }

    LRESULT
    wndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
    {
        if (message == WM_CREATE) {
//fgout << fgnl << "SplitScroll::WM_CREATE" << fgpush;
            hwndThis = hwnd;
            FGASSERT(m_panes.empty());
            FgGuiPtrs   panes = m_api.getPanes();
            m_panes.resize(panes.size());
            for (size_t ii=0; ii<m_panes.size(); ++ii) {
                m_panes[ii] = panes[ii]->getInstance();
                m_panes[ii]->create(hwndThis,int(ii),m_store+"_"+fgToString(ii),0UL,true);
            }
            g_gg.dg.update(m_api.updateFlagIdx);
//fgout << fgpop;
            return 0;
        }
        else if (message == WM_SIZE) {
            m_client = FgVect2I(LOWORD(lParam),HIWORD(lParam));
            if (m_client[0] * m_client[1] > 0) {
//fgout << fgnl << "SplitScroll::WM_SIZE: " << m_client << fgpush;
                resize();
//fgout << fgpop;
            }
            return 0;
        }
        else if (message == WM_VSCROLL) {
//fgout << "SplitScroll::WM_VSCROLL";
            int     tmp = m_si.nPos;
            // Get the current state, esp. trackbar drag position:
            m_si.fMask = SIF_ALL;
            GetScrollInfo(hwnd,SB_VERT,&m_si);
            int     msg = LOWORD(wParam);
            if (msg == SB_TOP)
                m_si.nPos = m_si.nMin;
            else if (msg == SB_BOTTOM)
                m_si.nPos = m_si.nMax;
            else if (msg == SB_LINEUP)
                m_si.nPos -= 5;
            else if (msg == SB_LINEDOWN)
                m_si.nPos += 5;
            else if (msg == SB_PAGEUP)
                m_si.nPos -= m_client[1];
            else if (msg == SB_PAGEDOWN)
                m_si.nPos += m_client[1];
            else if (msg == SB_THUMBTRACK)
                m_si.nPos = m_si.nTrackPos;
            m_si.fMask = SIF_POS;
            SetScrollInfo(hwnd,SB_VERT,&m_si,TRUE);
            // Windows may clamp the position:
            GetScrollInfo(hwnd,SB_VERT,&m_si);
            if (m_si.nPos != tmp) {
                resize();
                InvalidateRect(hwnd,NULL,FALSE);
            }
            return 0;
        }
        else if (message == WM_PAINT) {
//fgout << fgnl << "SplitScroll::WM_PAINT";
        }
        return DefWindowProc(hwnd,message,wParam,lParam);
    }

    FgVect2UI
    sumDims() const
    {
        FgVect2UI   sum(0);
        for (size_t ii=0; ii<m_panes.size(); ++ii)
            sum += m_panes[ii]->getMinSize();
        return sum;
    }

    void
    resize()
    {
        // No point in doing this before we have the client size (ie at first construction):
        if (m_client[1] > 0) {
            uint        sd = 1;
            FgVect2I    pos(0),
                        sz = m_client;
            pos[sd] = -m_si.nPos;
            for (size_t ii=0; ii<m_panes.size(); ++ii) {
                sz[sd] = m_panes[ii]->getMinSize()[sd];
                if ((pos[sd] > m_client[sd]) || (pos[sd]+sz[sd] < 0))
                    m_panes[ii]->showWindow(false);
                else {
                    m_panes[ii]->moveWindow(pos,sz);
                    m_panes[ii]->showWindow(true);
                }
                pos[sd] += sz[sd];
            }
            // Note that Windows wants the total range of the scrollable area,
            // not the effective slider range resulting from subtracting the 
            // currently displayed range:
            m_si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
            m_si.nMax = sumDims()[sd];
            m_si.nPage = m_client[sd];
            // Windows will clamp the position and otherwise adjust:
            SetScrollInfo(hwndThis,SB_VERT,&m_si,TRUE);
            m_si.fMask = SIF_ALL;
            GetScrollInfo(hwndThis,SB_VERT,&m_si);
        }
    }
};

FgSharedPtr<FgGuiOsBase>
fgGuiGetOsInstance(const FgGuiApiSplitScroll & def)
{return FgSharedPtr<FgGuiOsBase>(new FgGuiWinSplitScroll(def)); }
