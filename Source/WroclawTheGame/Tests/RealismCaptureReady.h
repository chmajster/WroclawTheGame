#pragma once
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
