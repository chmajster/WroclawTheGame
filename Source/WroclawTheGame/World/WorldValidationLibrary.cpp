#include "World/WorldValidationLibrary.h"
#include "Engine/World.h"
#include "WorldPartition/WorldPartition.h"
#include "Core/WTGLog.h"
bool UWorldValidationLibrary::ValidatePartition(UWorld *World)
{
    const bool Valid =
        World && World->GetWorldPartition() && World->GetWorldPartition()->IsStreamingEnabled();
    UE_LOG(LogWTGWorld, Log, TEXT("World Partition streaming: %s"),
           Valid ? TEXT("enabled") : TEXT("missing"));
    return Valid;
}
