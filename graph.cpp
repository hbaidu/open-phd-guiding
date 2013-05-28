/*
 *  graph.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2008-2010 Craig Stark.
 *  All rights reserved.
 *
 *  Seriously modified by Bret McKee
 *  Copyright (c) 2013 Bret McKee
 *  All rights reserved.
 *
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs,
 *     Bret McKee, Dad Dog Development, Ltd, nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "phd.h"
#include <wx/dcbuffer.h>
#include <wx/utils.h>
#include <wx/colordlg.h>

static const int DefaultMinLength =  50;
static const int DefaultMaxLength = 400;
static const int DefaultMinHeight =  1;
static const int DefaultMaxHeight = 16;

BEGIN_EVENT_TABLE(GraphLogWindow, wxWindow)
    EVT_PAINT(GraphLogWindow::OnPaint)
    EVT_BUTTON(BUTTON_GRAPH_MODE,GraphLogWindow::OnButtonMode)
    EVT_BUTTON(BUTTON_GRAPH_LENGTH,GraphLogWindow::OnButtonLength)
    EVT_BUTTON(BUTTON_GRAPH_HEIGHT,GraphLogWindow::OnButtonHeight)
    EVT_BUTTON(BUTTON_GRAPH_CLEAR,GraphLogWindow::OnButtonClear)
END_EVENT_TABLE()

GraphLogWindow::GraphLogWindow(wxWindow *parent):
wxWindow(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize, wxFULL_REPAINT_ON_RESIZE,_("Profile"))
{
    wxCommandEvent dummy;

    //SetFont(wxFont(8,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));

    m_pParent = parent;

    wxBoxSizer *pMainSizer   = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *pButtonSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *pClientSizer = new wxBoxSizer(wxHORIZONTAL);

    m_pClient = new GraphLogClientWindow(this);

    pClientSizer->Add(pButtonSizer, wxSizerFlags().Left().DoubleHorzBorder().Expand());
    pClientSizer->Add(m_pClient, wxSizerFlags().Expand().Proportion(1));


    m_pControlSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pXControlPane = pMount->GetXGuideAlgorithmControlPane(this);
    if (m_pXControlPane != NULL)
        m_pControlSizer->Add(m_pXControlPane, wxSizerFlags().Expand());
    m_pYControlPane = pMount->GetYGuideAlgorithmControlPane(this);
    //wxStaticLine *pLine = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);
    //m_pControlSizer->Add(pLine, wxSizerFlags().Expand());
    if (m_pYControlPane != NULL)
    {
        m_pControlSizer->Add(m_pYControlPane, wxSizerFlags().Expand());
    }
    m_pScopePane = pMount->GetGraphControlPane(this, _("Scope:"));
    if (m_pScopePane != NULL)
        m_pControlSizer->Add(m_pScopePane, wxSizerFlags().Expand());

    pMainSizer->Add(pClientSizer, wxSizerFlags().Expand().Proportion(1));
    pMainSizer->Add(m_pControlSizer, wxSizerFlags().Expand().Border(wxALL, 10));

    m_visible = false;
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    SetBackgroundColour(*wxBLACK);

    m_pLengthButton = new wxButton(this,BUTTON_GRAPH_LENGTH,_T("foo"));
    m_pLengthButton->SetToolTip(_("# of frames of history to display"));
    OnButtonLength(dummy); // update the buttom label
    pButtonSizer->Add(m_pLengthButton, wxSizerFlags(0).Border(wxTOP, 5));

    m_pHeightButton = new wxButton(this,BUTTON_GRAPH_HEIGHT,_T("foo"));
    m_heightButtonLabelVal = 0;
    OnButtonHeight(dummy); // update the buttom label
    pButtonSizer->Add(m_pHeightButton);

    m_pModeButton = new wxButton(this,BUTTON_GRAPH_MODE,_T("RA/Dec"));
    m_pModeButton->SetToolTip(_("Toggle RA/Dec vs dx/dy.  Shift-click to change RA/dx color.  Ctrl-click to change Dec/dy color"));
    pButtonSizer->Add(m_pModeButton);

    m_pClearButton = new wxButton(this,BUTTON_GRAPH_CLEAR,_("Clear"));
    m_pClearButton->SetToolTip(_("Clear graph data"));
    pButtonSizer->Add(m_pClearButton);

    wxBoxSizer *pLabelSizer = new wxBoxSizer(wxHORIZONTAL);

    m_pLabel1 = new wxStaticText(this, wxID_ANY, _("RA"));
    m_pLabel1->SetForegroundColour(m_pClient->m_raOrDxColor);
    m_pLabel1->SetBackgroundColour(*wxBLACK);
    pLabelSizer->Add(m_pLabel1, wxSizerFlags().Left());

    m_pLabel2 = new wxStaticText(this, wxID_ANY, _("Dec"));
    m_pLabel2->SetForegroundColour(m_pClient->m_decOrDyColor);
    m_pLabel2->SetBackgroundColour(*wxBLACK);

    pLabelSizer->AddStretchSpacer();
    pLabelSizer->Add(m_pLabel2, wxSizerFlags().Right());

    pButtonSizer->Add(pLabelSizer, wxSizerFlags().Expand());

    m_pClient->m_pOscRMS = new wxStaticText(this, wxID_ANY, _("RMS: 0.00"));
    m_pClient->m_pOscRMS->SetForegroundColour(*wxLIGHT_GREY);
    m_pClient->m_pOscRMS->SetBackgroundColour(*wxBLACK);
    pButtonSizer->Add(m_pClient->m_pOscRMS);

    m_pClient->m_pOscIndex = new wxStaticText(this, wxID_ANY, _("Osc: 0.00"));
    m_pClient->m_pOscIndex->SetForegroundColour(*wxLIGHT_GREY);
    m_pClient->m_pOscIndex->SetBackgroundColour(*wxBLACK);
    pButtonSizer->Add(m_pClient->m_pOscIndex);

    SetSizer(pMainSizer);
    pMainSizer->SetSizeHints(this);
}

GraphLogWindow::~GraphLogWindow()
{
    delete m_pClient;
}

wxColor GraphLogWindow::GetRaOrDxColor(void)
{
    return m_pClient->m_raOrDxColor;
}

wxColor GraphLogWindow::GetDecOrDyColor(void)
{
    return m_pClient->m_decOrDyColor;
}

int GraphLogWindow::StringWidth(wxString string)
{
    int width, height;

    m_pParent->GetTextExtent(string, &width, &height);

    return width;
}

void GraphLogWindow::OnButtonMode(wxCommandEvent& WXUNUSED(evt)) {
    wxMouseState mstate = wxGetMouseState();

    if (wxGetKeyState(WXK_SHIFT)) {
        wxColourData cdata;
        cdata.SetColour(m_pClient->m_raOrDxColor);
        wxColourDialog cdialog(this, &cdata);
        if (cdialog.ShowModal() == wxID_OK) {
            cdata = cdialog.GetColourData();
            m_pClient->m_raOrDxColor = cdata.GetColour();
        }
    }
    if (wxGetKeyState(WXK_CONTROL)) {
        wxColourData cdata;
        cdata.SetColour(m_pClient->m_decOrDyColor);
        wxColourDialog cdialog(this, &cdata);
        if (cdialog.ShowModal() == wxID_OK) {
            cdata = cdialog.GetColourData();
            m_pClient->m_decOrDyColor = cdata.GetColour();
        }
    }

    switch (m_pClient->m_mode)
    {
        case GraphLogClientWindow::MODE_RADEC:
            m_pClient->m_mode = GraphLogClientWindow::MODE_DXDY;
            m_pModeButton->SetLabel(_T("dx/dy"));
            break;
        case GraphLogClientWindow::MODE_DXDY:
            m_pClient->m_mode = GraphLogClientWindow::MODE_RADEC;
            m_pModeButton->SetLabel(_T("RA/Dec"));
            break;
    }

    Refresh();
}

void GraphLogWindow::OnButtonLength(wxCommandEvent& WXUNUSED(evt))
{
    m_pClient->m_length *= 2;

    if (m_pClient->m_length > m_pClient->m_maxLength)
    {
            m_pClient->m_length = m_pClient->m_minLength;
    }

    this->m_pLengthButton->SetLabel(wxString::Format(_T("x:%3d"), m_pClient->m_length));
    this->Refresh();
}

void GraphLogWindow::OnButtonHeight(wxCommandEvent& WXUNUSED(evt))
{
    m_pClient->m_height *= 2;

    if (m_pClient->m_height > m_pClient->m_maxHeight)
    {
            m_pClient->m_height = m_pClient->m_minHeight;
    }

    UpdateHeightButtonLabel();
    this->Refresh();
}

void GraphLogWindow::SetState(bool is_active) {
    this->m_visible = is_active;
    this->Show(is_active);
    if (is_active)
    {
        //UpdateControls();
        Refresh();
    }
}

void GraphLogWindow::AppendData(float dx, float dy, float RA, float Dec)
{
    m_pClient->AppendData(dx, dy, RA, Dec);

    if (m_visible)
    {
        Refresh();
    }
}

void GraphLogWindow::UpdateControls()
{
    if (m_pXControlPane != NULL)
    {
        m_pControlSizer->Detach(m_pXControlPane);
        m_pXControlPane->Destroy();
    }
    m_pXControlPane = pMount->GetXGuideAlgorithmControlPane(this);
    if (m_pXControlPane != NULL)
        m_pControlSizer->Add(m_pXControlPane, wxSizerFlags().Expand());

    if (m_pYControlPane != NULL)
    {
        m_pControlSizer->Detach(m_pYControlPane);
        m_pYControlPane->Destroy();
    }
    m_pYControlPane = pMount->GetYGuideAlgorithmControlPane(this);
    if (m_pYControlPane != NULL)
        m_pControlSizer->Add(m_pYControlPane, wxSizerFlags().Expand());

    if (m_pScopePane != NULL)
    {
        m_pControlSizer->Detach(m_pScopePane);
        m_pScopePane->Destroy();
    }
    m_pScopePane = pMount->GetGraphControlPane(this, _("Scope:"));
    if (m_pScopePane != NULL)
        m_pControlSizer->Add(m_pScopePane, wxSizerFlags().Expand());

    m_pControlSizer->Layout();
}

void GraphLogWindow::OnButtonClear(wxCommandEvent& WXUNUSED(evt))
{
    m_pClient->m_nItems = 0;
    Refresh();
}

void GraphLogWindow::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
    wxAutoBufferedPaintDC dc(this);

    dc.SetBackground(*wxBLACK_BRUSH);
    //dc.SetBackground(wxColour(10,0,0));
    dc.Clear();

    switch (m_pClient->m_mode)
    {
        case GraphLogClientWindow::MODE_RADEC:
            m_pLabel1->SetLabel(_("RA"));
            m_pLabel2->SetLabel(_("Dec"));
            break;
        case GraphLogClientWindow::MODE_DXDY:
            m_pLabel1->SetLabel(_("dx"));
            m_pLabel2->SetLabel(_("dy"));
            break;
    }

    UpdateHeightButtonLabel();
}

void GraphLogWindow::UpdateHeightButtonLabel(void)
{
    int val = m_pClient->m_height;
    if (pFrame && pFrame->GetSampling() != 1.0)
        val = -val; // <0 indicates arc-sec

    if (m_heightButtonLabelVal != val)
    {
        if (val > 0)
        {
            m_pHeightButton->SetLabel(wxString::Format(_T("y:+/-%d"), m_pClient->m_height));
            m_pHeightButton->SetToolTip(_("# of pixels per Y division"));
        }
        else
        {
            m_pHeightButton->SetLabel(wxString::Format(_T("y:+/-%d''"), m_pClient->m_height));
            m_pHeightButton->SetToolTip(_("# of arc-sec per Y division"));
        }
        m_heightButtonLabelVal = val;
    }
}

BEGIN_EVENT_TABLE(GraphLogClientWindow, wxWindow)
EVT_PAINT(GraphLogClientWindow::OnPaint)
END_EVENT_TABLE()

GraphLogClientWindow::GraphLogClientWindow(wxWindow *parent) :
    wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(401,200), wxFULL_REPAINT_ON_RESIZE)
{
    m_nItems = 0;
    m_mode = MODE_RADEC;

    m_raOrDxColor  = wxColour(100,100,255);
    m_decOrDyColor = wxColour(255,0,0);

    int minLength = pConfig->GetInt("/graph/minLength", DefaultMinLength);
    SetMinLength(minLength);

    int maxLength = pConfig->GetInt("/graph/maxLength", DefaultMaxLength);
    SetMaxLength(maxLength);

    int minHeight = pConfig->GetInt("/graph/minHeight", DefaultMinHeight);
    SetMinHeight(minHeight);

    int maxHeight = pConfig->GetInt("/graph/maxHeight", DefaultMaxHeight);
    SetMaxHeight(maxHeight);

    m_length = m_minLength;
    m_height = m_maxHeight;

    m_pHistory = new S_HISTORY[m_maxLength];
}

GraphLogClientWindow::~GraphLogClientWindow(void)
{
    delete [] m_pHistory;
}

bool GraphLogClientWindow::SetMinLength(int minLength)
{
    bool bError = false;

    try
    {
        if (minLength < 1)
        {
            throw ERROR_INFO("minLength < 1");
        }
        m_minLength = minLength;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_minLength = DefaultMinLength;
    }

    pConfig->SetInt("/graph/minLength", m_minLength);

    return bError;
}

bool GraphLogClientWindow::SetMaxLength(int maxLength)
{
    bool bError = false;

    try
    {
        if (maxLength <= m_minLength)
        {
            throw ERROR_INFO("maxLength < m_minLength");
        }
        m_maxLength = maxLength;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_minLength = DefaultMinLength;
        m_maxLength = DefaultMaxLength;
    }

    pConfig->SetInt("/graph/maxLength", m_maxLength);

    return bError;
}

bool GraphLogClientWindow::SetMinHeight(int minHeight)
{
    bool bError = false;

    try
    {
        if (minHeight < 1)
        {
            throw ERROR_INFO("minHeight < 1");
        }
        m_minHeight = minHeight;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_minHeight = DefaultMinHeight;
    }

    pConfig->SetInt("/graph/minHeight", m_minHeight);

    return bError;
}

bool GraphLogClientWindow::SetMaxHeight(int maxHeight)
{
    bool bError = false;

    try
    {
        if (maxHeight <= m_minHeight)
        {
            throw ERROR_INFO("maxHeight < m_minHeight");
        }
        m_maxHeight = maxHeight;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_minHeight = DefaultMinHeight;
        m_maxHeight = DefaultMaxHeight;
    }

    pConfig->SetInt("/graph/maxHeight", m_maxHeight);

    return bError;
}

void GraphLogClientWindow::AppendData(float dx, float dy, float RA, float Dec)
{
    const int idx = m_maxLength - 1;

    memmove(m_pHistory, m_pHistory+1, sizeof(m_pHistory[0])*(m_maxLength-1));

    m_pHistory[idx].dx  = dx;
    m_pHistory[idx].dy  = dy;
    m_pHistory[idx].ra  = RA;
    m_pHistory[idx].dec = Dec;

    if (m_nItems < m_maxLength)
    {
        m_nItems++;
    }
}

void GraphLogClientWindow::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
    //wxAutoBufferedPaintDC dc(this);
    wxPaintDC dc(this);

    wxSize size = GetClientSize();
    wxSize center(size.x/2, size.y/2);

    const int leftEdge = 0;
    const int rightEdge = size.x-5;

    const int topEdge = 5;
    const int bottomEdge = size.y-5;

    const int xorig = 0;
    const int yorig = size.y/2;

    int i;

    const int xDivisions = m_length/m_xSamplesPerDivision-1;
    const int xPixelsPerDivision = size.x/2/(xDivisions+1);
    const int yPixelsPerDivision = size.y/2/(m_yDivisions+1);

    const double sampling = pFrame->GetSampling();

    wxPoint *pRaOrDxLine  = NULL;
    wxPoint *pDecOrDyLine = NULL;

    dc.SetBackground(*wxBLACK_BRUSH);
    //dc.SetBackground(wxColour(10,0,0));
    dc.Clear();

    wxPen GreyDashPen;
    GreyDashPen = wxPen(wxColour(200,200,200),1, wxDOT);

    // Draw axes
    dc.SetPen(*wxGREY_PEN);
    dc.DrawLine(center.x,topEdge,center.x,bottomEdge);
    dc.DrawLine(leftEdge,center.y,rightEdge,center.y);

    // draw a box around the client area
    dc.SetPen(*wxGREY_PEN);
    dc.DrawLine(leftEdge, topEdge, rightEdge, topEdge);
    dc.DrawLine(rightEdge, topEdge, rightEdge, bottomEdge);
    dc.DrawLine(rightEdge, bottomEdge, leftEdge, bottomEdge);
    dc.DrawLine(leftEdge, bottomEdge, leftEdge, topEdge);

    // Draw horiz rule (scale is 1 pixel error per 25 pixels) + scale labels
    dc.SetPen(GreyDashPen);
    dc.SetTextForeground(*wxLIGHT_GREY);
#if defined (__APPLE__)
    dc.SetFont(*wxSMALL_FONT);
#else
    dc.SetFont(*wxSWISS_FONT);
#endif

    for(i=1;i<=m_yDivisions;i++)
    {
        double div_y = center.y-i*yPixelsPerDivision;
        dc.DrawLine(leftEdge,div_y, rightEdge, div_y);
        dc.DrawText(wxString::Format("%g", i * (double)m_height / (m_yDivisions + 1)), leftEdge + 3, div_y - 13);

        div_y = center.y+i*yPixelsPerDivision;
        dc.DrawLine(leftEdge, div_y, rightEdge, div_y);
        dc.DrawText(wxString::Format("%g", -i * (double)m_height / (m_yDivisions + 1)), leftEdge + 3, div_y - 13);
    }

    for(i=1;i<=xDivisions;i++)
    {
        dc.DrawLine(center.x-i*xPixelsPerDivision, topEdge, center.x-i*xPixelsPerDivision, bottomEdge);
        dc.DrawLine(center.x+i*xPixelsPerDivision, topEdge, center.x+i*xPixelsPerDivision, bottomEdge);
    }

    const double xmag = size.x / (double)m_length;
    const double ymag = yPixelsPerDivision*(double)(m_yDivisions + 1)/(double)m_height;

    // Draw data
    if (m_nItems > 0)
    {
        pRaOrDxLine  = new wxPoint[m_maxLength];
        pDecOrDyLine = new wxPoint[m_maxLength];

        int start_item = m_maxLength;

        if (m_nItems < m_length)
        {
            start_item -= m_nItems;
        }
        else
        {
            start_item -= m_length;
        }

        for (i=start_item; i<m_maxLength; i++)
        {
            int j=i-start_item;
            S_HISTORY *pSrc = m_pHistory + i;

            switch (m_mode)
            {
            case MODE_RADEC:
                pRaOrDxLine[j] =wxPoint(xorig+(j*xmag),yorig + (int) (pSrc->ra * (double) ymag * sampling));
                pDecOrDyLine[j]=wxPoint(xorig+(j*xmag),yorig + (int) (pSrc->dec * (double) ymag * sampling));
                break;
            case MODE_DXDY:
                pRaOrDxLine[j]=wxPoint(xorig+(j*xmag),yorig + (int) (pSrc->dx * (double) ymag * sampling));
                pDecOrDyLine[j]=wxPoint(xorig+(j*xmag),yorig + (int) (pSrc->dy * (double) ymag * sampling));
                break;
            }
        }

        wxPen raOrDxPen(m_raOrDxColor);
        wxPen decOrDyPen(m_decOrDyColor);

        int plot_length = m_length;

        if (m_length > m_nItems)
        {
            plot_length = m_nItems;
        }
        dc.SetPen(raOrDxPen);
        dc.DrawLines(plot_length,pRaOrDxLine);
        dc.SetPen(decOrDyPen);
        dc.DrawLines(plot_length,pDecOrDyLine);

        // Figure oscillation score
        int same_sides = 0;
        double mean = 0.0;
        for (i = start_item + 1 ; i < m_maxLength ; i++)
        {
            if ( (m_pHistory[i].ra * m_pHistory[i-1].ra) > 0.0)
                same_sides++;
            mean = mean + m_pHistory[i].ra;
        }
        if (m_nItems != start_item)
            mean = mean / (double) (m_nItems - start_item);
        else
            mean = 0.0;
        double RMS = 0.0;
        for (i = start_item + 1; i < m_maxLength; i++)
        {
            double ra = m_pHistory[i].ra;
            RMS = RMS + (ra-mean)*(ra-mean);
        }
        if (m_nItems != start_item)
            RMS = sqrt(RMS/(double) (m_maxLength - start_item));
        else
            RMS = 0.0;

        if (sampling != 1)
            m_pOscRMS->SetLabel(wxString::Format("RMS: %4.2f (%.2f'')", RMS, RMS * sampling));
        else
            m_pOscRMS->SetLabel(wxString::Format("RMS: %4.2f", RMS));

        double osc_index = 0.0;
        if (m_nItems != start_item)
            osc_index= 1.0 - (double) same_sides / (double) (m_maxLength - start_item);

        if ((osc_index > 0.6) || (osc_index < 0.15))
        {
            m_pOscIndex->SetForegroundColour(wxColour(185,20,0));
        }
        else
        {
            m_pOscIndex->SetForegroundColour(*wxLIGHT_GREY);
        }

        if (sampling != 1)
            m_pOscIndex->SetLabel(wxString::Format("Osc: %4.2f (%.2f)", osc_index, osc_index * sampling));
        else
            m_pOscIndex->SetLabel(wxString::Format("Osc: %4.2f", osc_index));

        delete [] pRaOrDxLine;
        delete [] pDecOrDyLine;
    }
}

GraphControlPane::GraphControlPane(wxWindow *pParent, wxString label)
:wxWindow(pParent, wxID_ANY)
{
    m_pParent = pParent;
    m_pControlSizer = new wxBoxSizer(wxHORIZONTAL);
    //m_pControlSizer = new wxStaticBoxSizer(wxHORIZONTAL, this, label);
    //m_pControlSizer->GetStaticBox()->SetBackgroundColour(*wxBLACK);
    //m_pControlSizer->GetStaticBox()->SetForegroundColour(*wxWHITE);
    //m_pControlSizer->GetStaticBox()->SetOwnBackgroundColour(*wxBLACK);

    SetBackgroundColour(*wxBLACK);

    int width  = StringWidth(label);
    wxStaticText *pLabel = new wxStaticText(this,wxID_ANY,label, wxDefaultPosition, wxSize(width + 5, -1));
    wxFont f = pLabel->GetFont();
    f.SetWeight(wxBOLD);
    pLabel->SetFont(f);
#ifdef __WINDOWS__
    pLabel->SetOwnForegroundColour(*wxWHITE);
#else
    pLabel->SetOwnBackgroundColour(*wxBLACK);
#endif

    m_pControlSizer->Add(pLabel, wxSizerFlags().Right());
    SetSizer(m_pControlSizer);
}

GraphControlPane::~GraphControlPane(void)
{
}

int GraphControlPane::StringWidth(wxString string)
{
    int width, height;

    m_pParent->GetTextExtent(string, &width, &height);

    return width;
}

void GraphControlPane::DoAdd(wxControl *pCtrl, wxString lbl)
{
    wxStaticText *pLabel = new wxStaticText(this,wxID_ANY,lbl);
#ifdef __WINDOWS__
    pLabel->SetOwnForegroundColour(*wxWHITE);
#else
    pLabel->SetOwnBackgroundColour(*wxBLACK);
#endif

    m_pControlSizer->Add(pLabel, wxSizerFlags().Right());
    m_pControlSizer->AddSpacer(5);
    m_pControlSizer->Add(pCtrl, wxSizerFlags().Left());
    m_pControlSizer->AddSpacer(10);
}