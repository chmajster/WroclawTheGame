#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Save/DistrictSaveGame.h"
#include "Mission/DistrictDeliverySubsystem.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDistrictSaveValidation,"WTG.District.SaveValidation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDistrictSaveValidation::RunTest(const FString&)
{
    auto* D=NewObject<UDistrictSaveGame>();D->DistrictId=TEXT("test");D->PlayerPosition=FVector(500,500,100);D->CarPosition=FVector(800,500,100);
    auto Valid=[&](){return D->Validate(FVector(0,0,0),FVector(1000,1000,500),TEXT("test"));};
    TestTrue(TEXT("Fresh data"),Valid());D->Version=2;TestFalse(TEXT("Future version"),Valid());D->Version=1;
    D->PlayerPosition.X=1001;TestFalse(TEXT("Out of bounds"),Valid());D->PlayerPosition.X=500;
    D->Economy.BalanceCents=43000;TestFalse(TEXT("Unbacked balance"),Valid());
    FWTGEconomyTransaction T;T.TransactionId=UDistrictDeliverySubsystem::RewardId;T.AmountCents=18000;T.BalanceAfterCents=43000;
    D->Economy.Ledger.Add(T);TestFalse(TEXT("Reward requires completed mission"),Valid());D->DeliveryStage=4;TestTrue(TEXT("Completed and rewarded"),Valid());
    D->Economy.Ledger.Add(T);TestFalse(TEXT("Duplicate reward"),Valid());D->Economy.Ledger.Pop();
    D->bInVehicle=true;D->CarHealth=0;TestFalse(TEXT("Destroyed occupied car"),Valid());D->CarHealth=100;
    D->WantedHeat=91;TestFalse(TEXT("Invalid wanted"),Valid());D->WantedHeat=0;
    D->Remaining=421;TestFalse(TEXT("Invalid timer"),Valid());D->Remaining=420;
    D->PlayerHealth=0;TestFalse(TEXT("Dead save"),Valid());D->PlayerHealth=100;
    TestTrue(TEXT("Valid again"),Valid());return true;
}
#endif
