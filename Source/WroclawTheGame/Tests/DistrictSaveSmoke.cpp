#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "Save/DistrictSaveSubsystem.h"
#include "Save/DistrictSaveGame.h"
#include "Systems/DistrictGameplaySubsystem.h"
#include "Systems/WTGEconomySubsystem.h"
#include "Mission/DistrictDeliverySubsystem.h"
#include "Character/SliceCharacter.h"
#include "Components/GameplayComponents.h"
#include "Vehicles/DriveableVehicle.h"
#include "Geography/DistrictRegistry.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

static FAutoConsoleCommandWithWorld GDistrictSaveSmoke(TEXT("WTG.DistrictSaveSmoke"),TEXT("Verify persistence in a real district world using an isolated slot."),
 FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World){
    if(!World || !World->IsGameWorld())return;
    auto* Save=World->GetSubsystem<UDistrictSaveSubsystem>();Save->Slot=TEXT("WroclawDistrict_SaveSmoke");UGameplayStatics::DeleteGameInSlot(Save->Slot,0);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak=TWeakObjectPtr<UWorld>(World),Elapsed=0.f,Stage=0,OK=true,Position=FVector::ZeroVector](float Dt)mutable{
        auto* W=Weak.Get();if(!W)return false;Elapsed+=Dt;
        auto* Save=W->GetSubsystem<UDistrictSaveSubsystem>();auto* G=W->GetSubsystem<UDistrictGameplaySubsystem>();auto* R=G->GetRegistry();
        if(Elapsed>30){UGameplayStatics::DeleteGameInSlot(Save->Slot,0);UE_LOG(LogTemp,Display,TEXT("WTG_DISTRICT_SAVE_DONE result=FAIL timeout"));FPlatformMisc::RequestExit(false);return false;}
        if(!R)return true;auto* Car=R->DeliveryVehicle.Get();if(!IsValid(Car))return true;auto* E=W->GetGameInstance()->GetSubsystem<UWTGEconomySubsystem>();
        auto Check=[&](bool Result,const TCHAR* Name){OK&=Result;UE_LOG(LogTemp,Display,TEXT("WTG_SAVE_CHECK %s=%s status=%s"),Name,Result?TEXT("PASS"):TEXT("FAIL"),*Save->Status);};
        if(Stage==0 && Elapsed>5){
            auto* P=Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(W,0));if(!P || P->GetCharacterMovement()->MovementMode!=MOVE_Walking)return true;
            Position=P->GetActorLocation();Check(Save->Save(false),TEXT("foot_write"));
            E->ApplyTransaction(TEXT("save-smoke-change"),500,TEXT("test"));P->AddActorWorldOffset(FVector(100,0,0),true);
            Check(Save->LoadSaved(),TEXT("foot_read"));Check(E->BalanceCents==25000 && FVector::Dist2D(P->GetActorLocation(),Position)<10,TEXT("foot_roundtrip"));
            Car->Interact(P);Check(Car->Driver==P,TEXT("enter"));Stage=1;
        }
        if(Stage==1 && Elapsed>9){Check(Save->Save(false),TEXT("vehicle_write"));Check(Car->Exit(),TEXT("exit"));Stage=2;}
        if(Stage==2 && Elapsed>10){Check(Save->LoadSaved(),TEXT("vehicle_read"));Check(UGameplayStatics::GetPlayerPawn(W,0)==Car && Car->Driver!=nullptr,TEXT("vehicle_roundtrip"));Stage=3;}
        if(Stage==3 && Elapsed>12){
            Car->Speed=0;Check(Car->Exit(),TEXT("reexit"));
            auto* D=W->GetSubsystem<UDistrictDeliverySubsystem>();D->State.Stage=Wroclaw::DeliveryStage::Complete;
            E->ApplyTransaction(UDistrictDeliverySubsystem::RewardId,18000,TEXT("test reward"));Check(Save->Save(false),TEXT("completed_write"));
            E->ResetEconomy();D->State.Stage=Wroclaw::DeliveryStage::Meet;Check(Save->LoadSaved(),TEXT("completed_read"));
            E->ApplyTransaction(UDistrictDeliverySubsystem::RewardId,18000,TEXT("duplicate"));
            Check(E->BalanceCents==43000 && E->Ledger.Num()==1 && D->State.Stage==Wroclaw::DeliveryStage::Complete,TEXT("reward_once"));
            auto* Future=Cast<UDistrictSaveGame>(UGameplayStatics::LoadGameFromSlot(Save->Slot,0));if(Future){Future->Version=2;UGameplayStatics::SaveGameToSlot(Future,Save->Slot,0);}
            Check(!Save->LoadSaved() && Save->IsWriteBlocked() && !Save->Save(false),TEXT("future_preserved"));
            auto* Preserved=Cast<UDistrictSaveGame>(UGameplayStatics::LoadGameFromSlot(Save->Slot,0));Check(Preserved && Preserved->Version==2,TEXT("future_unchanged"));
            Stage=4;
        }
        if(Stage==4 || Elapsed>25){UGameplayStatics::DeleteGameInSlot(Save->Slot,0);UE_LOG(LogTemp,Display,TEXT("WTG_DISTRICT_SAVE_DONE result=%s"),OK&&Stage==4?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExit(false);return false;}
        return true;
    }));
 }));
#endif
