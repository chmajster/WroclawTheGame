#pragma once
#include "CoreMinimal.h"

namespace Wroclaw::SaveMigration
{
bool CanAttemptCampaignSave(int32 SaveVersion, int32 CurrentVersion,
                            const FString &SaveSpace, bool bLegacySlot,
                            const FString &ActiveSpace);
}
