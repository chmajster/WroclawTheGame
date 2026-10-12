#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/DeliveryState.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeliveryRulesTest,"WTG.District.DeliveryRules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDeliveryRulesTest::RunTest(const FString&)
{
    Wroclaw::DeliveryState S;
    TestFalse(TEXT("Cannot finish without accepting"),S.Complete(true));
    TestTrue(TEXT("Accept"),S.Start(420));
    TestFalse(TEXT("Cannot accept twice"),S.Start(420));
    S.EnterCar();S.Tick(2,false,false,false,true);
    TestTrue(TEXT("Early exit does not cancel"),S.Stage==Wroclaw::DeliveryStage::Drive);
    S.Tick(2,false,false,true,true);
    TestFalse(TEXT("Cannot deliver from inside car"),S.CanDeliver(false,true,true,true,0));
    TestFalse(TEXT("Cannot deliver during pursuit"),S.CanDeliver(true,true,true,true,1));
    TestTrue(TEXT("Foot delivery with parked car"),S.CanDeliver(true,true,true,true,0));
    TestFalse(TEXT("Failed reward does not complete mission"),S.Complete(false));
    TestTrue(TEXT("Reward completes mission"),S.Complete(true));
    TestFalse(TEXT("Cannot reward completion twice"),S.Complete(true));
    TestFalse(TEXT("Cannot restart completed delivery"),S.Start(420));
    Wroclaw::DeliveryState Retry;Retry.Start(420);Retry.Tick(1,false,true,false,false);
    TestTrue(TEXT("Lost vehicle fails"),Retry.Stage==Wroclaw::DeliveryStage::Failed);
    TestTrue(TEXT("Failed mission can restart"),Retry.Start(420));
    Retry.Tick(420,false,false,false,false);
    TestTrue(TEXT("Timeout fails"),Retry.Stage==Wroclaw::DeliveryStage::Failed);
    Retry.Start(420);Retry.Tick(0,true,false,false,false);
    TestTrue(TEXT("Death fails even without elapsed game time"),Retry.Stage==Wroclaw::DeliveryStage::Failed);
    return true;
}
#endif
