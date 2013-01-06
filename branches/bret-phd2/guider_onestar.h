/*
 *  guider_onestar.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  All rights reserved.
 *
 *  Refactored by Bret McKee
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

#ifndef GUIDER_ONESTAR_H_INCLUDED
#define GUIDER_ONESTAR_H_INCLUDED

// Canvas area for image -- can take events
class GuiderOneStar: public Guider
{
protected:
    class GuiderOneStarConfigDialogPane : public GuiderConfigDialogPane
    {
        GuiderOneStar *m_pGuiderOneStar;
        wxSpinCtrl *m_pSearchRegion;
        wxSpinCtrlDouble *m_pMassChangeThreshold;

        public:
        GuiderOneStarConfigDialogPane(wxWindow *pParent, GuiderOneStar *pGuider);
        ~GuiderOneStarConfigDialogPane(void);

        virtual void LoadValues(void);
        virtual void UnloadValues(void);
    };

    Star m_star;

    // parameters
    double m_raAggression;
    double m_massChangeThreshold;
    int m_badMassCount;
    int m_autoSelectTries;
public:
	GuiderOneStar(wxWindow *parent);
    virtual ~GuiderOneStar(void);

	virtual void OnPaint(wxPaintEvent& evt);
    virtual bool UpdateGuideState(usImage *pImage, bool bStopping=false);
    virtual void ResetGuideState(void);
    virtual void StartGuiding(void);

    virtual bool IsLocked(void);
    virtual bool SetLockPosition(double x, double y, bool bExact=false);
    virtual bool AutoSelect(usImage *pImage);
    virtual Point &CurrentPosition(void);

    virtual bool SetRaGuideAlgorithm(int raGuideAlgorithm);
    virtual bool SetDecGuideAlgorithm(int decGuideAlgorithm);

    virtual double GetMassChangeThreshold(void);
    virtual bool SetMassChangeThreshold(double starMassChangeThreshold);
    virtual int GetSearchRegion(void);
    virtual bool SetSearchRegion(int searchRegion);
    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);

protected:
    void OnLClick(wxMouseEvent& evt);
    virtual bool SetState(GUIDER_STATE newState);

    void SaveStarFITS();

	DECLARE_EVENT_TABLE()
};

#endif /* GUIDER_ONESTAR_H_INCLUDED */
