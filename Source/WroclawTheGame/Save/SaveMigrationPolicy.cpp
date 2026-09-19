#include "Save/SaveMigrationPolicy.h"
#include "Save/SliceSave.h"

namespace Wroclaw::SaveMigration
{
namespace
{
struct FMigrationStep
{
    int32 FromVersion;
    int32 ToVersion;
    const TCHAR* FromSpace;
    const TCHAR* ToSpace;
    const TCHAR* Name;
    bool bLegacyOnly;
};

const TArray<FMigrationStep>& CampaignSteps()
{
    static const TArray<FMigrationStep> Steps = {
        {2, 3, TEXT("BlockoutV1"), TEXT("BlockoutV1"), TEXT("legacy_history_replay"), true},
        {3, 3, TEXT("BlockoutV1"), TEXT("WroclawGISV1"), TEXT("CampaignMigrationDefinition"), false},
    };
    return Steps;
}

FString StateKey(int32 Version, const FString& Space)
{
    return FString::Printf(TEXT("%d|%s"), Version, *Space);
}

struct FSearchState
{
    int32 Version = 0;
    FString Space;
    TArray<FString> Steps;
};

bool EnsurePayload(USliceSave& Save, const FString& Key, int32 CurrentSchema, const FString& DefaultJson)
{
    if (FWTGSystemSavePayload* Existing = Save.SystemPayloads.Find(Key))
    {
        if (Existing->SchemaVersion > CurrentSchema)
        {
            return false;
        }

        if (Existing->SchemaVersion < CurrentSchema)
        {
            Existing->SchemaVersion = CurrentSchema;
        }

        if (Existing->Json.IsEmpty())
        {
            Existing->Json = DefaultJson;
        }

        return true;
    }

    FWTGSystemSavePayload Payload;
    Payload.SchemaVersion = CurrentSchema;
    Payload.Json = DefaultJson;
    Save.SystemPayloads.Add(Key, MoveTemp(Payload));
    return true;
}
}

FCampaignMigrationPlan BuildCampaignMigrationPlan(int32 SaveVersion, int32 CurrentVersion,
                                                  const FString& SaveSpace, bool bLegacySlot,
                                                  const FString& ActiveSpace)
{
    FCampaignMigrationPlan Plan;

    if (SaveVersion > CurrentVersion || SaveVersion <= 0 || SaveSpace.IsEmpty() || ActiveSpace.IsEmpty())
    {
        return Plan;
    }

    if (SaveVersion == CurrentVersion && SaveSpace == ActiveSpace)
    {
        Plan.bSupported = true;
        return Plan;
    }

    TArray<FSearchState> Queue;
    TSet<FString> Visited;
    Queue.Add({SaveVersion, SaveSpace, {}});
    Visited.Add(StateKey(SaveVersion, SaveSpace));

    for (int32 Index = 0; Index < Queue.Num(); ++Index)
    {
        const FSearchState Current = Queue[Index];

        for (const FMigrationStep& Step : CampaignSteps())
        {
            if (Step.FromVersion != Current.Version || Current.Space != Step.FromSpace)
            {
                continue;
            }

            if (Step.bLegacyOnly && !bLegacySlot)
            {
                continue;
            }

            FSearchState Next;
            Next.Version = Step.ToVersion;
            Next.Space = Step.ToSpace;
            Next.Steps = Current.Steps;
            Next.Steps.Add(Step.Name);

            if (Next.Version == CurrentVersion && Next.Space == ActiveSpace)
            {
                Plan.bSupported = true;
                Plan.bRequiresBackup = !Next.Steps.IsEmpty();
                Plan.Steps = MoveTemp(Next.Steps);
                return Plan;
            }

            const FString Key = StateKey(Next.Version, Next.Space);
            if (!Visited.Contains(Key))
            {
                Visited.Add(Key);
                Queue.Add(MoveTemp(Next));
            }
        }
    }

    return Plan;
}

bool CanAttemptCampaignSave(int32 SaveVersion, int32 CurrentVersion,
                            const FString& SaveSpace, bool bLegacySlot,
                            const FString& ActiveSpace)
{
    return BuildCampaignMigrationPlan(SaveVersion, CurrentVersion, SaveSpace, bLegacySlot, ActiveSpace).bSupported;
}

bool EnsureCurrentSystemPayloads(USliceSave& Save)
{
    if (!EnsurePayload(Save, TEXT("parking"), 1, TEXT("{\"owned\":[],\"parked\":[]}")))
    {
        return false;
    }
    if (!EnsurePayload(Save, TEXT("npc_schedule"), 1, TEXT("{\"npcs\":{}}")))
    {
        return false;
    }
    if (!EnsurePayload(Save, TEXT("quests"), 1, TEXT("{\"active\":null,\"stage\":null,\"world_mutations\":[]}")))
    {
        return false;
    }
    if (!EnsurePayload(Save, TEXT("season"), 1, TEXT("{\"date\":\"2026-06-01\",\"season\":\"summer\",\"transition_alpha\":0}")))
    {
        return false;
    }

    return true;
}

FString RemapStableId(const FString& Category, const FString& Id)
{
    // The versioned registry owns explicit remap tables. Until a concrete
    // rename is registered, preserving the stable ID is the only safe action.
    (void)Category;
    return Id;
}
}
