
#include <TargetConditionals.h>
#if TARGET_OS_IOS == 1 || TARGET_OS_VISION == 1
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#define IPLUG_AUVIEWCONTROLLER IPlugAUViewController_vAutonomus
#define IPLUG_AUAUDIOUNIT IPlugAUAudioUnit_vAutonomus
#import <AutonomusAU/IPlugAUViewController.h>
#import <AutonomusAU/IPlugAUAudioUnit.h>

//! Project version number for AutonomusAU.
FOUNDATION_EXPORT double AutonomusAUVersionNumber;

//! Project version string for AutonomusAU.
FOUNDATION_EXPORT const unsigned char AutonomusAUVersionString[];

@class IPlugAUViewController_vAutonomus;
