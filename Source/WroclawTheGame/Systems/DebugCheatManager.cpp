#include "Systems/DebugCheatManager.h"
#include "Systems/OpenWorldSubsystem.h"
#include "Systems/NoiseSystem.h"
#include "Mission/SliceMission.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Core/SliceGameMode.h"
USliceMission *UDebugCheatManager::Session()
{
#if !UE_BUILD_SHIPPING
    auto *M = GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>();
    M->bDebugSession = true;
    M->Notify(TEXT("Sesja debug — zapis checkpointów wyłączony."));
    return M;
#else
    return nullptr;
#endif
}
void UDebugCheatManager::SetHeatLevel(int32 Level)
{
    if (auto *M = Session())
        M->WorldState.heat = FMath::Clamp(Level, 0, 5) * 20;
}
void UDebugCheatManager::GiveItem(const FString &Id, int32 Count)
{
    if (auto *M = Session())
    {
        const std::string Key = TCHAR_TO_UTF8(*Id);
        const auto *Def = Wroclaw::Progress::Item(Key);
        if (Def)
            M->State.inventory[Key] = FMath::Clamp(Count, 0, Def->stack);
    }
}
void UDebugCheatManager::AddEvidence(const FString &Id)
{
    if (auto *M = Session())
        for (const auto &E : Wroclaw::EvidenceDefinitions())
            if (Id == UTF8_TO_TCHAR(E.id.c_str()))
                M->State.evidence.insert(E.id);
}
void UDebugCheatManager::SetTime(float Hour)
{
    if (auto *M = Session())
        M->WorldState.SetTime(Hour);
}
void UDebugCheatManager::SetWeather(const FString &Id)
{
    if (auto *M = Session())
        M->WorldState.SetWeather(TCHAR_TO_UTF8(*Id));
}
void UDebugCheatManager::ToggleAI()
{
    if (Session())
    {
        auto *S = GetWorld()->GetSubsystem<UOpenWorldSubsystem>();
        S->bAIEnabled = !S->bAIEnabled;
    }
}
void UDebugCheatManager::ShowNoiseEvents()
{
    if (Session())
    {
        auto *S = GetWorld()->GetSubsystem<UNoiseSystem>();
        S->bDebug = !S->bDebug;
    }
}
void UDebugCheatManager::ShowAIPerception()
{
    if (Session())
    {
        auto *S = GetWorld()->GetSubsystem<UOpenWorldSubsystem>();
        S->bPerceptionDebug = !S->bPerceptionDebug;
    }
}
void UDebugCheatManager::TeleportToDistrict(const FString &Id)
{
    if (auto *M = Session())
        for (const auto &D : Wroclaw::Districts())
            if (Id == UTF8_TO_TCHAR(D.id.c_str()))
            {
                if (auto *GM = Cast<ASliceGameMode>(GetWorld()->GetAuthGameMode()))
                    GM->Relocate(FVector(D.bounds[0] + 600, D.bounds[1] + 600, 96));
                return;
            }
}
void UDebugCheatManager::CompleteQuest(const FString &Id)
{
    if (auto *M = Session())
    {
        const Wroclaw::QuestDef *Target = nullptr;
        for (const auto &Q : Wroclaw::Quests())
            if (Id == UTF8_TO_TCHAR(Q.id.c_str()))
                Target = &Q;
        for (const auto &Q : Wroclaw::SideQuests())
            if (Id == UTF8_TO_TCHAR(Q.id.c_str()))
                Target = &Q;
        if (!Target)
            return;
        for (const auto &Action : Target->all)
            M->State.completed.insert(Action);
        if (!Target->any.empty())
            M->State.completed.insert(Target->any.front());
    }
}
void UDebugCheatManager::StartQuest(const FString &Id)
{
    if (auto *M = Session())
    {
        M->State.tags.erase("Quest." + std::string(TCHAR_TO_UTF8(*Id)) + ".Suspended");
        M->State.tags.insert("Quest." + std::string(TCHAR_TO_UTF8(*Id)) + ".Active");
    }
}
