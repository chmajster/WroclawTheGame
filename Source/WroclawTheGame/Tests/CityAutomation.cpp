#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Systems/CityGameplaySubsystem.h"
#include "Kismet/GameplayStatics.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWTGCitySaveRoundTrip,"WTG.City.SaveMemoryRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWTGCitySaveRoundTrip::RunTest(const FString &Parameters)
{
    auto *Source=NewObject<UCityProgressSave>();
    Wroclaw::CityProgress State;
    State.completed={"city.huby.clue1","city.future.secret"};
    Source->Payload=UTF8_TO_TCHAR(State.Serialize().c_str());
    Source->Anchor=FVector(12000,24000,-19706);
    Source->HasVehicle=true;Source->VehicleAnchor=FVector(13000,24000,150);
    Source->VehicleYaw=80;Source->VehicleHealth=72;
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Serialize city save"),UGameplayStatics::SaveGameToMemory(Source,Bytes))) return false;
    auto *Loaded=Cast<UCityProgressSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("City save type"),Loaded)) return false;
    TestEqual(TEXT("Save version"),Loaded->Version,1);
    TestEqual(TEXT("Interior anchor"),Loaded->Anchor,Source->Anchor);
    TestEqual(TEXT("Vehicle location"),Loaded->VehicleAnchor,Source->VehicleAnchor);
    TestEqual(TEXT("Vehicle health"),Loaded->VehicleHealth,72.f);
    TestTrue(TEXT("Vehicle presence"),Loaded->HasVehicle);
    Wroclaw::CityProgress Restored;
    if (TestTrue(TEXT("Payload validation"),Wroclaw::CityProgress::Deserialize(TCHAR_TO_UTF8(*Loaded->Payload),Restored)))
        TestTrue(TEXT("Unknown IDs survive sector updates"),Restored.completed==State.completed);
    return true;
}
#endif
