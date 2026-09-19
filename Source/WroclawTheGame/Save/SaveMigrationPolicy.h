#pragma once
#include "CoreMinimal.h"

class USliceSave;

namespace Wroclaw::SaveMigration
{
struct WROCLAWTHEGAME_API FCampaignMigrationPlan
{
    bool bSupported = false;
    bool bRequiresBackup = false;
    TArray<FString> Steps;
};

FCampaignMigrationPlan BuildCampaignMigrationPlan(int32 SaveVersion, int32 CurrentVersion,
                                                  const FString& SaveSpace, bool bLegacySlot,
                                                  const FString& ActiveSpace);

bool CanAttemptCampaignSave(int32 SaveVersion, int32 CurrentVersion,
                            const FString& SaveSpace, bool bLegacySlot,
                            const FString& ActiveSpace);

bool EnsureCurrentSystemPayloads(USliceSave& Save);
FString RemapStableId(const FString& Category, const FString& Id);
}
