#include "Autonomus.h"
#include "IControls.h"
#include "IPlug_include_in_plug_src.h"
#include "click.hpp"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

IVKnobControl* CustomKnob(const IRECT &bounds, int paramIdx, const IVStyle &style)
{
    IVKnobControl* knob = new IVKnobControl(bounds, paramIdx, "", style, true, false, 
        -135.0f, 135.0f, -135.0f, EDirection::Vertical, 5.5, 2.f);

    knob->SetInnerPointerFrac(0.17f);
    knob->SetOuterPointerFrac(0.95f);
    knob->SetPointerThickness(5.f);

    return knob;
} 

Autonomus::Autonomus(const InstanceInfo& info) : iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
    const int MIN_BPM = 1;
    const int MAX_BPM = 300;

    GetParam(kTempo)->InitInt("Current BPM", 120, MIN_BPM, MAX_BPM, "BPM");
    GetParam(kEndBpm)->InitInt("End BPM", MAX_BPM, MIN_BPM, MAX_BPM, "BPM");
    GetParam(kIsPlaying)->InitBool("Play", false);
    GetParam(kIsBpmFrozen)->InitBool("Freeze BPM", false);
    GetParam(kSecondsPerIncrement)->InitDouble("Time per Beat Increase", 1., 0.1, 10., 0.1, "s");

    unsigned int channels = 0;
    unsigned int sampleRate = 0;
    drwav_uint64 totalFrames = 0;

    float* pData = drwav_open_memory_and_read_pcm_frames_f32((void*) CLICK, CLICK_length, &channels, &sampleRate, &totalFrames, nullptr);
    if (pData)
    {
        mClickNumChannels = static_cast<int>(channels);
        mClickSampleRate = static_cast<double>(sampleRate); 
        mClickBuffer.assign(pData, pData + totalFrames * channels);
        drwav_free(pData, nullptr); 
    }
    else 
    {
        DBGMSG("Click audio file does not exist.\n");
    }

#if IPLUG_EDITOR
    mMakeGraphicsFunc = [&]() { return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT)); };
    mLayoutFunc = [&](IGraphics* pGraphics) {
        pGraphics->LoadFont("Roboto-Regular", READEX_FN);
        pGraphics->AttachBackground(BACKGROUND_FN);
        pGraphics->EnableMouseOver(true);
        
        const int switchWidth = 90;
        const int switchHeight = 50;
        const int ledSize = 20;
        const int knobWidth = 180;
        const int knobHeight = 140;
        
        const IColor FG_COLOR = IColor::FromColorCodeStr("#092635");
        const IColor PR_COLOR = FG_COLOR.WithOpacity(0.9f);
        const IColor FR_COLOR = IColor::FromColorCodeStr("#5C8374");
        const IColor FR_COLOR_TRANSLUCENT = FR_COLOR.WithOpacity(0.5f);
        const IColor FONT_COLOR = IColor::FromColorCodeStr("#9EC8B9");
        const IText aboutBoxText = IText(16, EAlign::Near, FONT_COLOR);

        pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false, COLOR_TRANSLUCENT, FG_COLOR, PR_COLOR, 30.f);

        const IVColorSpec baseColors = {
            COLOR_TRANSPARENT, // Background
            FG_COLOR, // Foreground
            PR_COLOR, // Pressed
            FR_COLOR_TRANSLUCENT, // Frame
            COLOR_TRANSPARENT, // Highlight
            FG_COLOR, // Shadow
            FR_COLOR_TRANSLUCENT, // Extra 1
        };

        const IVColorSpec switchColors = {
            COLOR_TRANSPARENT, // Background
            FR_COLOR, // Foreground
            FR_COLOR, // Pressed
            COLOR_TRANSPARENT, // Frame
            COLOR_TRANSPARENT, // Highlight
            FG_COLOR, // Shadow
        };

        const IVStyle style {
            true, // Show label
            true, // Show value
            baseColors,
            IText(16, EVAlign::Top, FONT_COLOR), // Label text
            IText(18, EVAlign::Bottom, FONT_COLOR), // Value text
        };

        const IRECT innerBounds = pGraphics->GetBounds().GetPadded(-10.f);

        const IVStyle switchesStyle = style.WithRoundness(0.9f).WithShowValue(false).WithDrawShadows(false).WithFrameThickness(9).WithColors(switchColors);
        const IRECT switches = innerBounds.GetCentredInside(300, switchHeight).GetVShifted(-40);
        pGraphics->AttachControl(new IVSlideSwitchControl(switches.GetGridCell(0, 1, 3).GetCentredInside(switchWidth, switchHeight), kIsPlaying, "", switchesStyle, false));
        pGraphics->AttachControl(new CustomLED(switches.GetGridCell(1, 1, 3).GetCentredInside(ledSize), FG_COLOR), kLedTag);
        pGraphics->AttachControl(new IVSlideSwitchControl(switches.GetGridCell(2, 1, 3).GetCentredInside(switchWidth, switchHeight), kIsBpmFrozen, "", switchesStyle, false));
        
        const IVStyle knobsStyle = style.WithDrawShadows(false).WithWidgetFrac(0.95f).WithDrawFrame(false).WithColor(EVColor::kFR, FR_COLOR);
        const IRECT knobs = innerBounds.GetCentredInside(460, knobHeight).GetVShifted(100);
        pGraphics->AttachControl(CustomKnob(knobs.GetGridCell(0, 1, 3).GetCentredInside(knobWidth, knobHeight), kTempo, knobsStyle));
        pGraphics->AttachControl(CustomKnob(knobs.GetGridCell(1, 1, 3).GetCentredInside(knobWidth, knobHeight), kEndBpm, knobsStyle));
        pGraphics->AttachControl(CustomKnob(knobs.GetGridCell(2, 1, 3).GetCentredInside(knobWidth, knobHeight), kSecondsPerIncrement, knobsStyle));

        pGraphics->AttachControl(new IVButtonControl(innerBounds.GetFromTRHC(16, 16).GetPadded(2.f), SplashClickActionFunc, "i", style.WithDrawShadows(false).WithLabelText(aboutBoxText.WithAlign(EAlign::Center)), true, true, EVShape::Ellipse))
        ->SetAnimationEndActionFunction([pGraphics](IControl* pCaller) {
            pGraphics->GetControlWithTag(kAboutBoxTag)->As<IAboutBoxControl>()->Show();
        });

        pGraphics->AttachControl(new IAboutBoxControl(pGraphics->GetBounds(), FG_COLOR.WithOpacity(0.3f),
            [&](IContainerBase* pParent, const IRECT& r) {
                pParent->AddChildControl(new IVPanelControl(IRECT(), "", style.WithDrawShadows(false).WithRoundness(0.03f)));

                WDL_String infoStr {"Autonomus v"};
                infoStr.Append(PLUG_VERSION_STR);

                pParent->AddChildControl(new ITextControl(IRECT(), infoStr.Get(), aboutBoxText.WithSize(30)));
                pParent->AddChildControl(new ITextControl(IRECT(), PLUG_COPYRIGHT_STR, aboutBoxText.WithVAlign(EVAlign::Top)));
                pParent->AddChildControl(new IURLControl(IRECT(), PLUG_URL_STR, PLUG_URL_STR, aboutBoxText.WithSize(18), COLOR_TRANSPARENT, FONT_COLOR.WithOpacity(0.8f)));
            },
            [](IContainerBase* pParent, const IRECT& r) {
                const auto boxRECT = r.GetCentredInside(r.W() / 2.1f, r.H() / 3.f);
                pParent->GetChild(0)->SetTargetAndDrawRECTs(boxRECT);
                const auto infoRECT = boxRECT.GetHPadded(-12.f);
                pParent->GetChild(1)->SetTargetAndDrawRECTs(infoRECT.GetGridCell(0, 3, 1));
                pParent->GetChild(2)->SetTargetAndDrawRECTs(infoRECT.GetGridCell(1, 3, 1));
                pParent->GetChild(3)->SetTargetAndDrawRECTs(infoRECT.GetGridCell(2, 3, 1));
            }, 
            50), kAboutBoxTag)->Hide(true);
    };
#endif
}

#if IPLUG_EDITOR
void Autonomus::FlashLED()
{
    if (GetUI())
        GetUI()->GetControlWithTag(kLedTag)->As<CustomLED>()->TriggerWithDecay(1000); 
};
#endif

#if IPLUG_DSP
void Autonomus::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
    IParam* isPlayingParam = GetParam(kIsPlaying);
    IParam* tempoParam = GetParam(kTempo);
    const bool isBpmFrozen = GetParam(kIsBpmFrozen)->Bool();
    const double sampleRate = GetSampleRate();
    const double clickPlaybackRate = mClickSampleRate / sampleRate;
    const int samplesPerClick = static_cast<int>((sampleRate * 60.0) / tempoParam->Value());
    const int samplesPerTempoIncrement = static_cast<int>(sampleRate * GetParam(kSecondsPerIncrement)->Value());
    
    for (int s = 0; s < nFrames; s++)
    {
        if (isPlayingParam->Bool())
        {
            if (!isBpmFrozen) 
            {
                mTempoIncrementCounter++;
                if (samplesPerTempoIncrement > 0 && mTempoIncrementCounter >= samplesPerTempoIncrement)
                {
                    mTempoIncrementCounter -= samplesPerTempoIncrement;
                    int currentTempo = tempoParam->Int();
                    int endBpm = GetParam(kEndBpm)->Int();
    
                    if (currentTempo < endBpm)
                    {
                        tempoParam->Set(currentTempo + 1);
                        SendParameterValueFromDelegate(kTempo, tempoParam->GetNormalized(), true);
                    }
                    else if (currentTempo > endBpm)
                    {
                        tempoParam->Set(currentTempo - 1);
                        SendParameterValueFromDelegate(kTempo, tempoParam->GetNormalized(), true);
                    }                    
                    else
                    {
                        isPlayingParam->Set(0.);
                        SendParameterValueFromDelegate(kIsPlaying, isPlayingParam->GetNormalized(), true);
                    }
                }
            }

            mSampleCounter++;
            if (mSampleCounter >= samplesPerClick)
            {
                mSampleCounter -= samplesPerClick;
                mClickPlayhead = 0;
            }
        }
        
        sample clickSample = 0.;
        if (mClickPlayhead >= 0.0 && mClickPlayhead < (double)mClickBuffer.size())
        {
            if (this != nullptr && mClickPlayhead == 0.0)
                this->FlashLED();

            int idx0 = static_cast<int>(mClickPlayhead);
            int idx1 = std::min(idx0 + 1, (int)mClickBuffer.size() - 1);
            double frac = mClickPlayhead - idx0;
            clickSample = mClickBuffer[idx0] * (1.0 - frac) + mClickBuffer[idx1] * frac;

            mClickPlayhead += clickPlaybackRate;
        }
        else
        {
            mClickPlayhead = -1.0;
        }

        for (int c = 0; c < NOutChansConnected(); c++)
            outputs[c][s] = clickSample;        
    }
}  
#endif
