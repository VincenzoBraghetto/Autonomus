#pragma once
#include "IPlug_include_in_plug_hdr.h"
#include "ILEDControl.h"
#include <vector>

const int kNumPresets = 1;

enum EParams
{
    kTempo,
    kIsPlaying,
    kSecondsPerIncrement,
    kEndBpm,
    kIsBpmFrozen,
    kNumParams
};

enum ETags 
{ 
    kLedTag, 
    kLedMsgTag, 
    kAboutBoxTag 
};

using namespace iplug;
using namespace igraphics;

class CustomLED : public ILEDControl
{
    public:
        CustomLED(const IRECT& bounds, const IColor& innerColor) 
        : ILEDControl(bounds, innerColor)
        , innerColor(innerColor) 
        {}

        void Draw(IGraphics& g) override
        {
            const float v = static_cast<float>(GetValue() * 0.9f);
            const IColor c = IColor::FromHSLA(0.45f, 1.f, v);
            IRECT flare = mRECT.GetScaledAboutCentre(1.f);
            g.FillEllipse(innerColor, mRECT, nullptr);
            g.PathEllipse(flare);
            IBlend b = {EBlend::Default, v};
            g.PathFill(IPattern::CreateRadialGradient(mRECT.MW(), mRECT.MH(), mRECT.W()/2.f, {{c, 0.f}, {COLOR_TRANSPARENT, 1.f}}), {}, &b);
        }
        
        void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
        {
            if (msgTag == kLedMsgTag)
                TriggerWithDecay(1000);
        }
    
    private:
        IColor innerColor;
};

class Autonomus final : public Plugin
{
    private:
        int mSampleCounter = 0;
        int mTempoIncrementCounter = 0;
        double mClickPlayhead = -1.0;
        double mClickSampleRate = 48000.0;
        std::vector<sample> mClickBuffer;
        int mClickNumChannels = 1;

    public:
        Autonomus(const InstanceInfo& info);

    #if IPLUG_DSP
        void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
    #endif
};
