#pragma once
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "EngineUtils.h"
#include "World/WTGDayNightEnvironment.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif

// Captures must show completed textures, mesh cards and distance fields.
// Poll during normal ticks; do not suppress the editor's diagnostic messages.
inline bool WTGRealismCaptureReady()
{
#if WITH_EDITOR
    return FAssetCompilingManager::Get().GetNumRemainingAssets()==0;
#else
    return true;
#endif
}

// Explicit capture-only flag. Ordinary games retain the advancing day/night
// cycle. Fix before warmup so all views use the same sun, rather than sunset
// reached while the editor builds distance fields/cards.
inline void WTGRealismCaptureTime(UWorld* World)
{
    float Hours=0;
    if(!World || !FParse::Value(FCommandLine::Get(),TEXT("WTGDiagnosticTime="),Hours))return;
    for(TActorIterator<AWTGDayNightEnvironment> It(World);It;++It){
        It->bAdvanceTime=false;It->SetTimeOfDay(Hours);
    }
}
