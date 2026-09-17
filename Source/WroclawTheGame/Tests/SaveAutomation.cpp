#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Save/SliceSave.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/OpenWorldState.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWTGSaveRoundTrip, "WTG.Save.Version3MemoryRoundTrip",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWTGSaveRoundTrip::RunTest(const FString &Parameters)
{
    auto *Source = NewObject<USliceSave>();
    Source->Variant = 2;
    Source->Timeline = {TEXT("action:awake")};
    Source->History = {TEXT("awake")};
    Source->UsedItems.Add(TEXT("medkit"), 1);
    Source->ObjectiveCounters.Add(TEXT("survival"), 12);
    Wroclaw::WorldState World;
    World.AddHeat(43);
    World.Discover("park", true);
    Source->WorldPayload = UTF8_TO_TCHAR(World.Serialize().c_str());
    FWTGNPCSnapshot NPC;
    NPC.Transform.SetLocation(FVector(3000, 1800, 96));
    NPC.Health = 52;
    Source->NPCs.Add(TEXT("courtyard"), NPC);
    TArray<uint8> Bytes;
    TestTrue(TEXT("Serialize SaveGame"), UGameplayStatics::SaveGameToMemory(Source, Bytes));
    auto *Loaded = Cast<USliceSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Loaded type"), Loaded))
        return false;
    TestEqual(TEXT("Version"), Loaded->Version, 3);
    TestEqual(TEXT("Puzzle variant"), Loaded->Variant, 2);
    if (TestEqual(TEXT("Timeline length"), Loaded->Timeline.Num(), 1))
        TestEqual(TEXT("Timeline"), Loaded->Timeline[0], FString(TEXT("action:awake")));
    TestEqual(TEXT("World payload"), Loaded->WorldPayload, Source->WorldPayload);
    if (const auto *SavedNPC = Loaded->NPCs.Find(TEXT("courtyard")))
        TestEqual(TEXT("NPC health"), SavedNPC->Health, 52.f);
    else
        AddError(TEXT("Missing NPC snapshot"));
    Wroclaw::WorldState Restored;
    TestTrue(TEXT("World payload validation"),
             Wroclaw::WorldState::Deserialize(TCHAR_TO_UTF8(*Loaded->WorldPayload), Restored));
    TestEqual(TEXT("Heat survives"), Restored.heat, 43.0);
    return true;
}
#endif
