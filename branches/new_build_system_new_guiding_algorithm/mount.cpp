/*
 *  mount.cpp
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2012 Bret McKee
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
 *    Neither the name of Bret McKee, Dad Dog Development,
 *     Craig Stark, Stark Labs nor the names of its
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

// enable dec compensation when calibration declination is less than this
const double Mount::DEC_COMP_LIMIT = M_PI / 2.0 * 2.0 / 3.0;

inline static bool
IsOppositeSide(PierSide a, PierSide b)
{
    return (a == PIER_SIDE_EAST && b == PIER_SIDE_WEST) ||
        (a == PIER_SIDE_WEST && b == PIER_SIDE_EAST);
}

inline static const char *PierSideStr(PierSide p, const char *unknown = _("Unknown"))
{
    switch (p) {
    case PIER_SIDE_EAST: return _("East");
    case PIER_SIDE_WEST: return _("West");
    default:             return unknown;
    }
}

inline static PierSide OppositeSide(PierSide p)
{
    switch (p) {
    case PIER_SIDE_EAST: return PIER_SIDE_WEST;
    case PIER_SIDE_WEST: return PIER_SIDE_EAST;
    default:             return PIER_SIDE_UNKNOWN;
    }
}

static ConfigDialogPane *GetGuideAlgoDialogPane(GuideAlgorithm *algo, wxWindow *parent)
{
    // we need to force the guide alogorithm config pane to be large enough for
    // any of the guide algorithms
    ConfigDialogPane *pane = algo->GetConfigDialogPane(parent);
    pane->SetMinSize(-1, 110);
    return pane;
}

Mount::MountConfigDialogPane::MountConfigDialogPane(wxWindow *pParent, const wxString& title, Mount *pMount)
    : ConfigDialogPane(wxString::Format(_("%s Settings"),title), pParent)
{
    int width;
    m_pMount = pMount;

    wxBoxSizer *chkSizer = new wxBoxSizer(wxHORIZONTAL);

    m_pRecalibrate = new wxCheckBox(pParent ,wxID_ANY,_("Force calibration"));
    m_pRecalibrate->SetToolTip(_("Check to clear any previous calibration and force PHD to recalibrate"));

    m_pEnableGuide = new wxCheckBox(pParent, wxID_ANY,_("Enable Guide Output"));
    m_pEnableGuide->SetToolTip(_("Keep this checked for guiding. Un-check to disable all mount guide commands and allow the mount to run un-guided"));

    chkSizer->Add(m_pRecalibrate);
    chkSizer->Add(m_pEnableGuide);
    DoAdd(chkSizer);

    wxString xAlgorithms[] = {
        _("None"),_("Hysteresis"),_("Lowpass"),_("Lowpass2"), _("Resist Switch"),_("Gaussian Process")
    };

    width = StringArrayWidth(xAlgorithms, WXSIZEOF(xAlgorithms));
    m_pXGuideAlgorithmChoice = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
                                    wxSize(width+35, -1), WXSIZEOF(xAlgorithms), xAlgorithms);
    DoAdd(_("RA Algorithm"), m_pXGuideAlgorithmChoice,
          _("Which Guide Algorithm to use for Right Ascension"));

    m_pParent->Connect(m_pXGuideAlgorithmChoice->GetId(), wxEVT_COMMAND_CHOICE_SELECTED,
        wxCommandEventHandler(Mount::MountConfigDialogPane::OnXAlgorithmSelected), 0, this);

    if (!m_pMount->m_pXGuideAlgorithm)
    {
        m_pXGuideAlgorithmConfigDialogPane  = NULL;
    }
    else
    {
        m_pXGuideAlgorithmConfigDialogPane = GetGuideAlgoDialogPane(m_pMount->m_pXGuideAlgorithm, pParent);
        DoAdd(m_pXGuideAlgorithmConfigDialogPane);
    }

    wxString yAlgorithms[] = {
        _("None"),_("Hysteresis"),_("Lowpass"),_("Lowpass2"), _("Resist Switch"),_("Gaussian Proces")
    };

    width = StringArrayWidth(yAlgorithms, WXSIZEOF(yAlgorithms));
    m_pYGuideAlgorithmChoice = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
                                    wxSize(width+35, -1), WXSIZEOF(yAlgorithms), yAlgorithms);
    DoAdd(_("Declination Algorithm"), m_pYGuideAlgorithmChoice,
          _("Which Guide Algorithm to use for Declination"));

    m_pParent->Connect(m_pYGuideAlgorithmChoice->GetId(), wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(Mount::MountConfigDialogPane::OnYAlgorithmSelected), 0, this);

    if (!pMount->m_pYGuideAlgorithm)
    {
        m_pYGuideAlgorithmConfigDialogPane  = NULL;
    }
    else
    {
        m_pYGuideAlgorithmConfigDialogPane  = GetGuideAlgoDialogPane(pMount->m_pYGuideAlgorithm, pParent);
        DoAdd(m_pYGuideAlgorithmConfigDialogPane);
    }
}

Mount::MountConfigDialogPane::~MountConfigDialogPane(void)
{
}

void Mount::MountConfigDialogPane::OnXAlgorithmSelected(wxCommandEvent& evt)
{
    ConfigDialogPane *oldpane = m_pXGuideAlgorithmConfigDialogPane;
    oldpane->Clear(true);
    m_pMount->SetXGuideAlgorithm(m_pXGuideAlgorithmChoice->GetSelection());
    ConfigDialogPane *newpane = GetGuideAlgoDialogPane(m_pMount->m_pXGuideAlgorithm, m_pParent);
    Replace(oldpane, newpane);
    m_pXGuideAlgorithmConfigDialogPane = newpane;
    m_pXGuideAlgorithmConfigDialogPane->LoadValues();
    m_pParent->Layout();
    m_pParent->Update();
    m_pParent->Refresh();
}

void Mount::MountConfigDialogPane::OnYAlgorithmSelected(wxCommandEvent& evt)
{
    ConfigDialogPane *oldpane = m_pYGuideAlgorithmConfigDialogPane;
    oldpane->Clear(true);
    m_pMount->SetYGuideAlgorithm(m_pYGuideAlgorithmChoice->GetSelection());
    ConfigDialogPane *newpane = GetGuideAlgoDialogPane(m_pMount->m_pYGuideAlgorithm, m_pParent);
    Replace(oldpane, newpane);
    m_pYGuideAlgorithmConfigDialogPane = newpane;
    m_pYGuideAlgorithmConfigDialogPane->LoadValues();
    m_pParent->Layout();
    m_pParent->Update();
    m_pParent->Refresh();
}

void Mount::MountConfigDialogPane::LoadValues(void)
{
    m_pRecalibrate->SetValue(!m_pMount->IsCalibrated());
    m_pRecalibrate->Enable(!pFrame->CaptureActive);

    m_initXGuideAlgorithmSelection = m_pMount->GetXGuideAlgorithm();
    m_pXGuideAlgorithmChoice->SetSelection(m_initXGuideAlgorithmSelection);
    m_pXGuideAlgorithmChoice->Enable(!pFrame->CaptureActive);
    m_initYGuideAlgorithmSelection = m_pMount->GetYGuideAlgorithm();
    m_pYGuideAlgorithmChoice->SetSelection(m_initYGuideAlgorithmSelection);
    m_pYGuideAlgorithmChoice->Enable(!pFrame->CaptureActive);
    m_pEnableGuide->SetValue(m_pMount->GetGuidingEnabled());

    if (m_pXGuideAlgorithmConfigDialogPane)
    {
        m_pXGuideAlgorithmConfigDialogPane->LoadValues();
    }

    if (m_pYGuideAlgorithmConfigDialogPane)
    {
        m_pYGuideAlgorithmConfigDialogPane->LoadValues();
    }
}

void Mount::MountConfigDialogPane::UnloadValues(void)
{
    if (m_pRecalibrate->GetValue())
    {
        m_pMount->ClearCalibration();
    }

    m_pMount->SetGuidingEnabled(m_pEnableGuide->GetValue());

    // note these two have to be before the SetXxxAlgorithm calls, because if we
    // changed the algorithm, the current one will get freed, and if we make
    // these two calls after that, bad things happen
    if (m_pXGuideAlgorithmConfigDialogPane)
    {
        m_pXGuideAlgorithmConfigDialogPane->UnloadValues();
    }

    if (m_pYGuideAlgorithmConfigDialogPane)
    {
        m_pYGuideAlgorithmConfigDialogPane->UnloadValues();
    }

    m_pMount->SetXGuideAlgorithm(m_pXGuideAlgorithmChoice->GetSelection());
    m_pMount->SetYGuideAlgorithm(m_pYGuideAlgorithmChoice->GetSelection());
}

// Restore the guide algorithms - all the UI controls will follow correctly if the actual algorithm choices are correct
void Mount::MountConfigDialogPane::Undo(void)
{
    if (m_pXGuideAlgorithmConfigDialogPane)
    {
        m_pXGuideAlgorithmConfigDialogPane->Undo();
    }

    if (m_pYGuideAlgorithmConfigDialogPane)
    {
        m_pYGuideAlgorithmConfigDialogPane->Undo();
    }
    m_pMount->SetXGuideAlgorithm(m_initXGuideAlgorithmSelection);
    m_pMount->SetYGuideAlgorithm(m_initYGuideAlgorithmSelection);
}

GUIDE_ALGORITHM Mount::GetXGuideAlgorithm(void)
{
    return GetGuideAlgorithm(m_pXGuideAlgorithm);
}

void Mount::SetXGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm)
{
    if (!m_pXGuideAlgorithm || m_pXGuideAlgorithm->Algorithm() != guideAlgorithm)
    {
        delete m_pXGuideAlgorithm;

        if (CreateGuideAlgorithm(guideAlgorithm, this, GUIDE_X, &m_pXGuideAlgorithm))
        {
            CreateGuideAlgorithm(defaultAlgorithm, this, GUIDE_X, &m_pXGuideAlgorithm);
            guideAlgorithm = defaultAlgorithm;
        }

        pConfig->Profile.SetInt("/" + GetMountClassName() + "/XGuideAlgorithm", guideAlgorithm);
    }
}

GUIDE_ALGORITHM Mount::GetYGuideAlgorithm(void)
{
    return GetGuideAlgorithm(m_pYGuideAlgorithm);
}

void Mount::SetYGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm)
{
    if (!m_pYGuideAlgorithm || m_pYGuideAlgorithm->Algorithm() != guideAlgorithm)
    {
        delete m_pYGuideAlgorithm;

        if (CreateGuideAlgorithm(guideAlgorithm, this, GUIDE_Y, &m_pYGuideAlgorithm))
        {
            CreateGuideAlgorithm(defaultAlgorithm, this, GUIDE_Y, &m_pYGuideAlgorithm);
            guideAlgorithm = defaultAlgorithm;
        }

        pConfig->Profile.SetInt("/" + GetMountClassName() + "/YGuideAlgorithm", guideAlgorithm);
    }
}

bool Mount::GetGuidingEnabled(void)
{
    return m_guidingEnabled;
}

void Mount::SetGuidingEnabled(bool guidingEnabled)
{
    m_guidingEnabled = guidingEnabled;
}

GUIDE_ALGORITHM Mount::GetGuideAlgorithm(GuideAlgorithm *pAlgorithm)
{
    GUIDE_ALGORITHM ret = GUIDE_ALGORITHM_NONE;

    if (pAlgorithm)
    {
        ret = pAlgorithm->Algorithm();
    }
    return ret;
}

bool Mount::CreateGuideAlgorithm(int guideAlgorithm, Mount *mount, GuideAxis axis, GuideAlgorithm** ppAlgorithm)
{
    bool bError = false;

    try
    {
        switch (guideAlgorithm)
        {
            case GUIDE_ALGORITHM_IDENTITY:
            case GUIDE_ALGORITHM_HYSTERESIS:
            case GUIDE_ALGORITHM_LOWPASS:
            case GUIDE_ALGORITHM_LOWPASS2:
            case GUIDE_ALGORITHM_RESIST_SWITCH:
            case GUIDE_ALGORITHM_GAUSSIAN_PROCESS:
                break;
            case GUIDE_ALGORITHM_NONE:
            default:
                throw ERROR_INFO("invalid guideAlgorithm");
                break;
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        guideAlgorithm = GUIDE_ALGORITHM_IDENTITY;
    }

    switch (guideAlgorithm)
    {
        case GUIDE_ALGORITHM_IDENTITY:
            *ppAlgorithm = (GuideAlgorithm *) new GuideAlgorithmIdentity(mount, axis);
            break;
        case GUIDE_ALGORITHM_HYSTERESIS:
            *ppAlgorithm = (GuideAlgorithm *) new GuideAlgorithmHysteresis(mount, axis);
            break;
        case GUIDE_ALGORITHM_LOWPASS:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmLowpass(mount, axis);
            break;
        case GUIDE_ALGORITHM_LOWPASS2:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmLowpass2(mount, axis);
            break;
        case GUIDE_ALGORITHM_RESIST_SWITCH:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmResistSwitch(mount, axis);
            break;
        case GUIDE_ALGORITHM_GAUSSIAN_PROCESS:
            *ppAlgorithm = (GuideAlgorithm *)new GuideGaussianProcess(mount,axis);
            break;
        case GUIDE_ALGORITHM_NONE:
        default:
            assert(false);
            break;
    }

    return bError;
}

#ifdef TEST_TRANSFORMS
/*
 * TestTransforms() is a routine to do some sanity checking on the transform routines, which
 * have been a source of a huge number of headaches involving getting the reverse transforms
 * to work
 *
 */
void Mount::TestTransforms(void)
{
    static bool bTested = false;

    if (!bTested)
    {
        for(int inverted=0;inverted<2;inverted++)
        {
            // we test every 15 degrees, starting at -195 degrees and
            // ending at 375 degrees

            for(int i=-13;i<14;i++)
            {
                double xAngle = ((double)i) * M_PI/12.0;
                double yAngle;

                yAngle = xAngle+M_PI/2.0;

                if (inverted)
                {
                    yAngle -= M_PI;
                }

                // normalize the angles since real callers of setCalibration
                // will get the angle from atan2().

                xAngle = atan2(sin(xAngle), cos(xAngle));
                yAngle = atan2(sin(yAngle), cos(yAngle));

                Debug.AddLine("xidx=%.2f, yIdx=%.2f", xAngle/M_PI*180.0/15, yAngle/M_PI*180.0/15);

                SetCalibration(xAngle, yAngle, 1.0, 1.0);

                for(int j=-13;j<14;j++)
                {
                    double cameraAngle = ((double)i) * M_PI/12.0;
                    cameraAngle = atan2(sin(cameraAngle), cos(cameraAngle));

                    PHD_Point p0(cos(cameraAngle), sin(cameraAngle));
                    assert(fabs((p0.X*p0.X + p0.Y*p0.Y) - 1.00) < 0.01);

                    double p0Angle = p0.Angle();
                    assert(fabs(cameraAngle - p0Angle) < 0.01);

                    PHD_Point p1, p2;

                    if (TransformCameraCoordinatesToMountCoordinates(p0, p1))
                    {
                        assert(false);
                    }

                    assert(fabs((p1.X*p1.X + p1.Y*p1.Y) - 1.00) < 0.01);

                    double adjustedCameraAngle = cameraAngle - xAngle;

                    double cosAdjustedCameraAngle = cos(adjustedCameraAngle);

                    // check that the X value is correct
                    assert(fabs(cosAdjustedCameraAngle - p1.X) < 0.01);

                    double sinAdjustedCameraAngle = sin(adjustedCameraAngle);

                    if (inverted)
                    {
                        sinAdjustedCameraAngle *= -1;
                    }
                    // and check that the Y value is correct;
                    assert(fabs(sinAdjustedCameraAngle - p1.Y) < 0.01);

                    if (TransformMountCoordinatesToCameraCoordinates(p1, p2))
                    {
                        assert(false);
                    }

                    double p2Angle = p2.Angle();
                    //assert(fabs(p0Angle - p2Angle) < 0.01);
                    assert(fabs((p2.X*p2.X + p2.Y*p2.Y) - 1.00) < 0.01);
                    assert(fabs(p0.X - p2.X) < 0.01);
                    assert(fabs(p0.Y - p2.Y) < 0.01);
                }
            }
        }

        bTested = true;
        ClearCalibration();
    }
}
#endif

Mount::Mount(void)
{
    m_connected = false;
    m_requestCount = 0;

    m_pYGuideAlgorithm = NULL;
    m_pXGuideAlgorithm = NULL;
    m_guidingEnabled = true;

    ClearCalibration();

#ifdef TEST_TRANSFORMS
    TestTransforms();
#endif
}

Mount::~Mount()
{
    delete m_pXGuideAlgorithm;
    delete m_pYGuideAlgorithm;
}

double Mount::yAngle()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_xAngle - m_yAngleError + M_PI/2;
    }

    return dReturn;
}

double Mount::yRate()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_yRate;
    }

    return dReturn;
}

double Mount::xAngle()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_xAngle;
    }

    return dReturn;
}

double Mount::xRate()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_xRate;
    }

    return dReturn;
}

bool Mount::FlipCalibration(void)
{
    bool bError = false;

    try
    {
        if (!IsCalibrated())
        {
            throw ERROR_INFO("cannot flip if not calibrated");
        }

        double origX = xAngle();
        double origY = yAngle();

        bool decFlipRequired = CalibrationFlipRequiresDecFlip();

        Debug.AddLine(wxString::Format("FlipCalibration before: x=%.2f, y=%.2f decFlipRequired=%d sideOfPier=%s",
            origX, origY, decFlipRequired, PierSideStr(m_calPierSide)));

        double newX = origX + M_PI;
        double newY = origY;

        if (decFlipRequired)
        {
            newY += M_PI;
        }

        Debug.AddLine("FlipCalibration pre-normalize: x=%.2f, y=%.2f", newX, newY);

        // normalize
        newX = atan2(sin(newX), cos(newX));
        newY = atan2(sin(newY), cos(newY));

        PierSide priorPierSide = m_calPierSide;
        PierSide newPierSide = OppositeSide(m_calPierSide);

        Debug.AddLine(wxString::Format("FlipCalibration after: x=%.2f, y=%.2f sideOfPier=%s",
            newX, newY, PierSideStr(newPierSide)));

        SetCalibration(newX, newY, m_calXRate, m_yRate, m_calDeclination, newPierSide);

        pFrame->SetStatusText(wxString::Format(_("CAL: %s(%.f,%.f)->%s(%.f,%.f)"),
            PierSideStr(priorPierSide, ""), origX * 180. / M_PI, origY * 180. / M_PI,
            PierSideStr(newPierSide, ""), newX * 180. / M_PI, newY * 180. / M_PI), 0);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

Mount::MOVE_RESULT Mount::Move(const PHD_Point& cameraVectorEndpoint, bool normalMove)
{
    MOVE_RESULT result = MOVE_OK;

    try
    {
        PHD_Point mountVectorEndpoint;

        if (TransformCameraCoordinatesToMountCoordinates(cameraVectorEndpoint, mountVectorEndpoint))
        {
            throw ERROR_INFO("Unable to transform camera coordinates");
        }

        double xDistance = mountVectorEndpoint.X;
        double yDistance = mountVectorEndpoint.Y;

        Debug.AddLine(wxString::Format("Moving (%.2f, %.2f) raw xDistance=%.2f yDistance=%.2f",
            cameraVectorEndpoint.X, cameraVectorEndpoint.Y, xDistance, yDistance));

        if (normalMove)
        {
            // Feed the raw distances to the guide algorithms

            if (m_pXGuideAlgorithm)
            {
                
                // Put Matlab Interaction into a Subclass of GuideAlgorithm
                
                xDistance = m_pXGuideAlgorithm->result(xDistance);
            }

            if (m_pYGuideAlgorithm)
            {
                yDistance = m_pYGuideAlgorithm->result(yDistance);
            }
        }

        // Figure out the guide directions based on the (possibly) updated distances
        GUIDE_DIRECTION xDirection = xDistance > 0.0 ? LEFT : RIGHT;
        GUIDE_DIRECTION yDirection = yDistance > 0.0 ? DOWN : UP;

        int requestedXAmount = (int) floor(fabs(xDistance / m_xRate) + 0.5);
        int actualXAmount = 0;
        result = Move(xDirection, requestedXAmount, normalMove, &actualXAmount);

        wxString msg;

        if (actualXAmount > 0)
        {
            msg = wxString::Format(_("%s %5.2f px %3d ms"), xDirection == EAST ? _("East") : _("West"),
                fabs(xDistance), actualXAmount);
        }

        int actualYAmount = 0;
        if (result == MOVE_OK || result == MOVE_ERROR)
        {
            int requestedYAmount = (int) floor(fabs(yDistance / m_yRate) + 0.5);
            result = Move(yDirection, requestedYAmount, normalMove, &actualYAmount);

            if (actualYAmount > 0)
            {
                msg = wxString::Format(_("%s%*s%s %.2f px %d ms"), msg,
                    msg.IsEmpty() ? 42 : msg.Len() < 30 ? 30 - msg.Len() : 1, "",
                    yDirection == SOUTH ? _("South") : _("North"),
                    fabs(yDistance), actualYAmount);
            }
        }

        if (!msg.IsEmpty())
        {
            pFrame->SetStatusText(msg, 1, wxMax(actualXAmount, actualYAmount));
            Debug.AddLine(msg);
        }

        GuideStepInfo info;
        info.mount = this;
        info.time = (wxDateTime::UNow() - pFrame->m_guidingStarted).GetMilliseconds().ToDouble() / 1000.0;
        info.cameraOffset = &cameraVectorEndpoint;
        info.mountOffset = &mountVectorEndpoint;
        info.guideDistanceRA = xDistance;
        info.guideDistanceDec = yDistance;
        info.durationRA = actualXAmount;
        info.directionRA = xDirection;
        info.durationDec = actualYAmount;
        info.directionDec = yDirection;
        info.aoPos = GetAoPos();
        info.starMass = pFrame->pGuider->StarMass();
        info.starSNR = pFrame->pGuider->SNR();

        GuideLog.GuideStep(info);
        EvtServer.NotifyGuideStep(info);

        if (normalMove)
        {
            pFrame->pGraphLog->AppendData(info);
            pFrame->pTarget->AppendData(info);
        }
    }
    catch (wxString errMsg)
    {
        POSSIBLY_UNUSED(errMsg);
        result = MOVE_ERROR;
    }

    return result;
}

/*
 * The transform code has proven really tricky to get right.  For future generations
 * (and for me the next time I try to work on it), I'm going to put some notes here.
 *
 * The goal of TransformCameraCoordinatesToMountCoordinates is to transform a camera
 * pixel coordinate into an x and y, and TransformMountCoordinatesToCameraCoordinates
 * does the reverse, converting a mount x and y into pixels coordinates.
 *
 * Note: If a mount's x and y axis are not perfectly perpendicular, the reverse transform
 * will not be able to accuratey reverse the forward transform. The amount of inaccuracy
 * depends upon the perpendicular error.
 *
 * The mount is calibrated but moving 1 axis, then the other, watching where the mount
 * started and stopped.  Looking at the coordinates of the start and stop point, we
 * can compute the angle and the speed.
 *
 * The original PHD code used cos() against both the angles, but that code had issues
 * with sign reversal for some of the reverse transforms.  I spent some time looking
 * at that code (OK, a lot of time) and I never managed to get it to work, so I
 * rewrote it.
 *
 * After calibration, we use the x axis angle, and compute the y axis error by
 * subtracting 90 degrees.  If the mount was perfectly perpendicular, the error
 * will either be 0 or 180, depending on whether the axis motion was reversed
 * during calibration.
 *
 * I put this quote in when I was trying to fix the cos/cos code.  Hopefully
 * since  completely rewrote that code, I underand the new code. But I'm going
 * to leave it hear as it still seems relevant, given the amount of trouble
 * I had getting this code to work.
 *
 * In the words of Stevie Wonder, noted Computer Scientist and singer/songwriter
 * of some repute:
 *
 * "When you believe in things that you don't understand
 * Then you suffer
 * Superstition ain't the way"
 *
 * As part of trying to get the transforms to work, I wrote a routine called TestTransforms()
 * which does a bunch of forward/reverse trasnform pairs, checking the results as it goes.
 *
 * If you ever have change the transform functions, it would be wise to #define TEST_TRANSFORMS
 * to make sure that you (at least) didn't break anything that TestTransforms() checks for.
 *
 */

bool Mount::TransformCameraCoordinatesToMountCoordinates(const PHD_Point& cameraVectorEndpoint,
                                                         PHD_Point& mountVectorEndpoint)
{
    bool bError = false;

    try
    {
        if (!cameraVectorEndpoint.IsValid())
        {
            throw ERROR_INFO("invalid cameraVectorEndPoint");
        }

        double hyp   = cameraVectorEndpoint.Distance();
        double cameraTheta = cameraVectorEndpoint.Angle();

        double xAngle = cameraTheta - m_xAngle;
        double yAngle = cameraTheta - (m_xAngle + m_yAngleError);

        // Convert theta and hyp into X and Y

        mountVectorEndpoint.SetXY(
            cos(xAngle) * hyp,
            sin(yAngle) * hyp
            );

        Debug.AddLine("CameraToMount -- cameraTheta (%.2f) - m_xAngle (%.2f) = xAngle (%.2f = %.2f)",
                cameraTheta, m_xAngle, xAngle, atan2(sin(xAngle), cos(xAngle)));
        Debug.AddLine("CameraToMount -- cameraTheta (%.2f) - (m_xAngle (%.2f) + m_yAngleError (%.2f)) = yAngle (%.2f = %.2f)",
                cameraTheta, m_xAngle, m_yAngleError, yAngle, atan2(sin(yAngle), cos(yAngle)));
        Debug.AddLine("CameraToMount -- cameraX=%.2f cameraY=%.2f hyp=%.2f cameraTheta=%.2f mountX=%.2f mountY=%.2f, mountTheta=%.2f",
                cameraVectorEndpoint.X, cameraVectorEndpoint.Y, hyp, cameraTheta, mountVectorEndpoint.X, mountVectorEndpoint.Y,
                mountVectorEndpoint.Angle());
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        mountVectorEndpoint.Invalidate();
    }

    return bError;
}

bool Mount::TransformMountCoordinatesToCameraCoordinates(const PHD_Point& mountVectorEndpoint,
                                                        PHD_Point& cameraVectorEndpoint)
{
    bool bError = false;

    try
    {
        if (!mountVectorEndpoint.IsValid())
        {
            throw ERROR_INFO("invalid mountVectorEndPoint");
        }

        double hyp = mountVectorEndpoint.Distance();
        double mountTheta = mountVectorEndpoint.Angle();

        if (fabs(m_yAngleError) > M_PI/2)
        {
            mountTheta = -mountTheta;
        }

        double xAngle = mountTheta + m_xAngle;

        cameraVectorEndpoint.SetXY(
                cos(xAngle) * hyp,
                sin(xAngle) * hyp
                );

        Debug.AddLine("MountToCamera -- mountTheta (%.2f) + m_xAngle (%.2f) = xAngle (%.2f = %.2f)",
                mountTheta, m_xAngle, xAngle, atan2(sin(xAngle), cos(xAngle)));
        Debug.AddLine("MountToCamera -- mountX=%.2f mountY=%.2f hyp=%.2f mountTheta=%.2f cameraX=%.2f, cameraY=%.2f cameraTheta=%.2f",
                mountVectorEndpoint.X, mountVectorEndpoint.Y, hyp, mountTheta, cameraVectorEndpoint.X, cameraVectorEndpoint.Y,
                cameraVectorEndpoint.Angle());

    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        cameraVectorEndpoint.Invalidate();
    }

    return bError;
}

GraphControlPane *Mount::GetXGuideAlgorithmControlPane(wxWindow *pParent)
{
    return m_pXGuideAlgorithm->GetGraphControlPane(pParent, _("RA:"));
}

GraphControlPane *Mount::GetYGuideAlgorithmControlPane(wxWindow *pParent)
{
    return m_pYGuideAlgorithm->GetGraphControlPane(pParent, _("DEC:"));
}

GraphControlPane *Mount::GetGraphControlPane(wxWindow *pParent, const wxString& label)
{
    return NULL;
};

/*
 * Adjust the calibration data for the scope's current coordinates.
 *
 * This includes adjusting the xRate to compensate for changes in declination
 * relative to the declination where calibration was done, and possibly flipping
 * the calibration data if the mount is known to be on the other side of the
 * pier from where calibration was done.
 */
void Mount::AdjustCalibrationForScopePointing(void)
{
    double newDeclination = pPointingSource->GetGuidingDeclination();
    PierSide newPierSide = pPointingSource->SideOfPier();

    Debug.AddLine(wxString::Format("AdjustCalibrationForScopePointing (%s): current dec=%.1f pierSide=%d, cal dec=%.1f pierSide=%d",
        GetMountClassName(), newDeclination * 180. / M_PI, newPierSide, m_calDeclination * 180. / M_PI, m_calPierSide));

    if (newDeclination != m_calDeclination)             // Compensation required
    {
        // avoid division by zero and gross errors.  If the user didn't calibrate
        // somewhere near the celestial equator, we don't do this
        if (fabs(m_calDeclination) > DEC_COMP_LIMIT)
        {
            Debug.AddLine("skipping declination adjustment: initial calibration too far from equator");
        }
        else
        {
            m_xRate = (m_calXRate / cos(m_calDeclination)) * cos(newDeclination);
            m_currentDeclination = newDeclination;
            Debug.AddLine("Dec comp: XRate %.4f -> %.4f for dec %.2f -> dec %.2f",
                m_calXRate, m_xRate, m_calDeclination, newDeclination);
            if (pFrame) pFrame->UpdateCalibrationStatus();
        }
    }

    if (IsOppositeSide(newPierSide, m_calPierSide))
    {
        Debug.AddLine(wxString::Format("Guiding starts on opposite side of pier: calibration data "
            "side is %s, current side is %s", PierSideStr(m_calPierSide), PierSideStr(newPierSide)));
        FlipCalibration();
    }
}

bool Mount::IsBusy(void)
{
    return m_requestCount > 0;
}

void Mount::IncrementRequestCount(void)
{
    m_requestCount++;

    // for the moment we never enqueue requests if the mount is busy, but we can
    // enqueue them two at a time.  There is no reason we can't, it's just that
    // right now we don't, and this might catch an error
    assert(m_requestCount <= 2);

}

void Mount::DecrementRequestCount(void)
{
    assert(m_requestCount > 0);
    m_requestCount--;
}

bool Mount::HasNonGuiMove(void)
{
    return false;
}

bool Mount::SynchronousOnly(void)
{
    return false;
}

bool Mount::HasSetupDialog(void) const
{
    return false;
}

void Mount::SetupDialog(void)
{
}

const wxString& Mount::Name(void) const
{
    return m_Name;
}

bool Mount::IsStepGuider(void) const
{
    return false;
}

wxPoint Mount::GetAoPos(void) const
{
    return wxPoint();
}

wxPoint Mount::GetAoMaxPos(void) const
{
    return wxPoint();
}

const char *Mount::DirectionStr(GUIDE_DIRECTION d)
{
    // these are used internally in the guide log and event server and are not translated
    switch (d) {
    case NONE:  return "None";
    case NORTH: return "North";
    case SOUTH: return "South";
    case EAST:  return "East";
    case WEST:  return "West";
    default:    return "?";
    }
}

const char *Mount::DirectionChar(GUIDE_DIRECTION d)
{
    // these are used internally in the guide log and event server and are not translated
    switch (d) {
    case NONE:  return "-";
    case NORTH: return "N";
    case SOUTH: return "S";
    case EAST:  return "E";
    case WEST:  return "W";
    default:    return "?";
    }
}

bool Mount::IsCalibrated()
{
    bool bReturn = false;

    if (IsConnected())
    {
        bReturn = m_calibrated;
    }

    return bReturn;
}

void Mount::ClearCalibration(void)
{
    m_calibrated = false;
    if (pFrame) pFrame->UpdateCalibrationStatus();
}

void Mount::SetCalibration(double xAngle, double yAngle, double xRate, double yRate, double declination, PierSide pierSide)
{
    Debug.AddLine(wxString::Format("Mount::SetCalibration (%s) -- xAngle=%.2f yAngle=%.2f xRate=%.4f yRate=%.4f dec=%.2f pierSide=%d",
        GetMountClassName(), xAngle, yAngle, xRate, yRate, declination, pierSide));

    // we do the rates first, since they just get stored
    m_yRate  = yRate;
    m_calXRate = xRate;
    m_calDeclination = declination;
    m_calPierSide = pierSide;
    m_xRate  = m_calXRate;
    m_currentDeclination = declination;

    // the angles are more difficult because we have to turn yAngle into a yError.
    m_xAngle = xAngle;
    m_yAngleError = (xAngle-yAngle) + M_PI/2;

    m_yAngleError = atan2(sin(m_yAngleError), cos(m_yAngleError));

    Debug.AddLine(wxString::Format("Mount::SetCalibration (%s) -- sets m_xAngle=%.2f m_yAngleError=%.2f",
        GetMountClassName(), m_xAngle, m_yAngleError));

    m_calibrated = true;
    if (pFrame) pFrame->UpdateCalibrationStatus();

    // store calibration data
    wxString prefix = "/" + GetMountClassName() + "/calibration/";
    pConfig->Profile.SetString(prefix + "timestamp", wxDateTime::Now().Format());
    pConfig->Profile.SetDouble(prefix + "xAngle", xAngle);
    pConfig->Profile.SetDouble(prefix + "yAngle", yAngle);
    pConfig->Profile.SetDouble(prefix + "xRate", xRate);
    pConfig->Profile.SetDouble(prefix + "yRate", yRate);
    pConfig->Profile.SetDouble(prefix + "declination", declination);
    pConfig->Profile.SetInt(prefix + "pierSide", pierSide);
}

bool Mount::IsConnected()
{
    bool bReturn = m_connected;

    return bReturn;
}

bool Mount::Connect(void)
{
    m_connected = true;

    if (pFrame)
    {
        pFrame->UpdateCalibrationStatus();
    }

    return false;
}

bool Mount::Disconnect(void)
{
    m_connected = false;
    if (pFrame) pFrame->UpdateCalibrationStatus();

    return false;
}

void Mount::ClearHistory(void)
{
    if (m_pXGuideAlgorithm)
    {
        m_pXGuideAlgorithm->reset();
    }

    if (m_pYGuideAlgorithm)
    {
        m_pYGuideAlgorithm->reset();
    }
}

// Return a default guiding declination that will "do no harm" in terms of RA rate adjustments - either the Dec
// where the calibration was done or zero
double Mount::GetDefGuidingDeclination()
{
    return m_calibrated ? m_calDeclination : 0.0;
}

// Get a value of declination, in radians, that can be used for adjusting the RA guide rate.  Normally, this will be gotten
// from the ASCOM scope subclass, but it could also come from the 'aux' mount connection.  If this method in the base class is
// called, we don't have any pointing info, so return a default that will do no harm.
// Don't force clients to catch exceptions.  Callers who want the traditional ASCOM
// dec value should use GetCoordinates().
double Mount::GetGuidingDeclination(void)
{

    return GetDefGuidingDeclination();
}

// Baseline implementations for non-ASCOM subclasses.  Methods will
// return a sensible default or an error (true)
bool Mount::GetGuideRates(double *pRAGuideRate, double *pDecGuideRate)
{
    return true; // error, not implemented
}

bool Mount::GetCoordinates(double *ra, double *dec, double *siderealTime)
{
    return true; // error
}

bool Mount::GetSiteLatLong(double *latitude, double *longitude)
{
    return true; // error
}

bool Mount::CanSlew(void)
{
    return false;
}

bool Mount::CanReportPosition()
{
    return false;
}

bool Mount::SlewToCoordinates(double ra, double dec)
{
    return true; // error
}

bool Mount::CanCheckSlewing(void)
{
    return false;
}

bool Mount::Slewing(void)
{
    return false;
}

PierSide Mount::SideOfPier(void)
{
    return PIER_SIDE_UNKNOWN;
}

wxString Mount::GetSettingsSummary()
{
    // return a loggable summary of current mount settings
    wxString algorithms[] = {
        _T("None"),_T("Hysteresis"),_T("Lowpass"),_T("Lowpass2"), _T("Resist Switch")
    };

    double cur_ra, cur_dec, cur_st;
    wxString dec_str;
    if (!pPointingSource->GetCoordinates(&cur_ra, &cur_dec, &cur_st))
    {
        dec_str = wxString::Format("%0.1f deg", cur_dec);
    }
    else
    {
        dec_str = "Unknown";
    }
    return wxString::Format("Mount = %s,%s connected, guiding %s, %s, Dec = %s, Pier side = %s\n",
        m_Name,
        IsConnected() ? " " : " not",
        m_guidingEnabled ? "enabled" : "disabled",
        IsCalibrated() ? wxString::Format("xAngle = %.3f, xRate = %.4f, yAngle = %.3f, yRate = %.4f",
                xAngle(), xRate(), yAngle(), yRate()) : "not calibrated",
                dec_str,
                PierSideStr(pPointingSource->SideOfPier())
    ) + wxString::Format("X guide algorithm = %s, %s",
        algorithms[GetXGuideAlgorithm()],
        m_pXGuideAlgorithm->GetSettingsSummary()
    ) + wxString::Format("Y guide algorithm = %s, %s",
        algorithms[GetYGuideAlgorithm()],
        m_pYGuideAlgorithm->GetSettingsSummary()
    );
}

bool Mount::CalibrationFlipRequiresDecFlip(void)
{
    return false;
}

void Mount::StartDecDrift(void)
{
}

void Mount::EndDecDrift(void)
{
}

bool Mount::IsDecDrifting(void) const
{
    return false;
}
