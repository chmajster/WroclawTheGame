#include "Save/SaveMigrationPolicy.h"

namespace Wroclaw::SaveMigration
{
bool CanAttemptCampaignSave(int32 SaveVersion, int32 CurrentVersion,
                            const FString &SaveSpace, bool bLegacySlot,
                            const FString &ActiveSpace)
{
    const bool VersionSupported =
        SaveVersion == CurrentVersion || (bLegacySlot && SaveVersion == 2);
    if (!VersionSupported)
        return false;

    if (SaveSpace == ActiveSpace)
        return true;

    // The only currently implemented coordinate migration is the authored
    // campaign BlockoutV1 -> WroclawGISV1 transform. Additional spaces must
    // be registered deliberately rather than silently accepted.
    return SaveSpace == TEXT("BlockoutV1") &&
           ActiveSpace == TEXT("WroclawGISV1");
}
}
