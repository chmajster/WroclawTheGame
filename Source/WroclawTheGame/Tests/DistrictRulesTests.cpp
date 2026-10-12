#include "Core/WantedState.h"
#include "Misc/AutomationTest.h"
#include <limits>
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDistrictWantedTest,"WTG.District.WantedDecay",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDistrictWantedTest::RunTest(const FString&)
{
    Wroclaw::WantedState A,B;
    A.Report(35);B.Report(35);
    TestEqual(TEXT("Pedestrian impact escalates to two levels"),A.Level(),2);
    A.Tick(30,false);
    for (int32 I=0;I<300;++I) B.Tick(.1f,false);
    TestTrue(TEXT("Decay independent of timestep"),FMath::IsNearlyEqual(A.Heat,B.Heat,.001f));
    A.Report(90);TestEqual(TEXT("Wanted capped at three"),A.Level(),3);
    A.Tick(100,true);TestEqual(TEXT("Sight prevents decay"),A.Heat,90.f);
    A.Tick(17,false);TestEqual(TEXT("Search grace period"),A.Heat,90.f);
    A.Tick(100,false);TestEqual(TEXT("Successful evasion clears wanted"),A.Level(),0);
    A.Report(-20);A.Report(std::numeric_limits<float>::quiet_NaN());
    TestEqual(TEXT("Invalid offence ignored"),A.Level(),0);
    return true;
}
#endif
