#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/GameplayComponents.h"
#include "Character/SliceCharacter.h"
#include "Vehicles/DriveableVehicle.h"
#include "Vehicles/VehicleAIDriverComponent.h"
#include "Vehicles/VehiclePursuitSubsystem.h"
#include "Geography/DistrictRegistry.h"
#include "Systems/DistrictGameplaySubsystem.h"
#include "Systems/WTGEconomySubsystem.h"
#include "Mission/DistrictDeliverySubsystem.h"
#include "Mission/SliceMission.h"
#include "Save/DistrictSaveSubsystem.h"
#include "Save/DistrictSaveGame.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UI/SliceController.h"

struct FDistrictSessionProbe
{
    float Elapsed=0,StepAt=0,DriveDistance=0,NextDiagnostic=0;
    int Stage=0;bool OK=true;
    FVector Origin=FVector::ZeroVector,LastCar=FVector::ZeroVector,CameraBack=FVector::ZeroVector;
    float CameraDistance=0;
    TWeakObjectPtr<AActor> Wall,CameraWall;
    TWeakObjectPtr<ASliceCharacter> Player;
    TWeakObjectPtr<UVehicleAIDriverComponent> AI;
};
static FAutoConsoleCommandWithWorld GDistrictSessionSmoke(TEXT("WTG.DistrictSessionSmoke"),TEXT("Exercise movement, collision, camera, mission and physical driving; isolated persistence slot."),
 FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World){
    if(!World || !World->IsGameWorld())return;
    const bool Resume=FParse::Param(FCommandLine::Get(),TEXT("WTGDistrictSessionResume"));
    auto* Save=World->GetSubsystem<UDistrictSaveSubsystem>();Save->Slot=TEXT("WroclawDistrict_SessionSmoke");
    if(!Resume)UGameplayStatics::DeleteGameInSlot(Save->Slot,0);
    auto S=MakeShared<FDistrictSessionProbe>();
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak=TWeakObjectPtr<UWorld>(World),S,Resume](float Dt){
        auto* W=Weak.Get();if(!W)return false;S->Elapsed+=Dt;
        auto* Save=W->GetSubsystem<UDistrictSaveSubsystem>();auto* G=W->GetSubsystem<UDistrictGameplaySubsystem>();auto* R=G->GetRegistry();
        auto Check=[&](bool Value,const TCHAR* Name){S->OK&=Value;UE_LOG(LogTemp,Display,TEXT("WTG_SESSION_CHECK %s=%s"),Name,Value?TEXT("PASS"):TEXT("FAIL"));};
        auto Finish=[&](){if(Resume || !S->OK)UGameplayStatics::DeleteGameInSlot(Save->Slot,0);UE_LOG(LogTemp,Display,TEXT("WTG_DISTRICT_SESSION_DONE result=%s resume=%d elapsed=%.2f physical_distance_m=%.2f"),S->OK?TEXT("PASS"):TEXT("FAIL"),Resume,S->Elapsed,S->DriveDistance/100);FPlatformMisc::RequestExit(false);};
        if(S->Elapsed>330){Check(false,TEXT("timeout"));Finish();return false;}
        if(!R)return true;auto* Car=R->DeliveryVehicle.Get();if(!IsValid(Car))return true;
        auto* PC=Cast<ASliceController>(UGameplayStatics::GetPlayerController(W,0));if(!PC)return true;
        auto* D=W->GetSubsystem<UDistrictDeliverySubsystem>();auto* E=W->GetGameInstance()->GetSubsystem<UWTGEconomySubsystem>();
        auto Advance=[&](int Stage){S->Stage=Stage;S->StepAt=S->Elapsed;};
        auto Walk=[&](const FVector& Goal){if(auto* P=S->Player.Get())P->AddMovementInput((Goal-P->GetActorLocation()).GetSafeNormal2D(),1,true);};
        if(S->Stage==0 && S->Elapsed>5)
        {
            auto* P=Cast<ASliceCharacter>(PC->GetPawn());if(!P || P->GetCharacterMovement()->MovementMode!=MOVE_Walking)return true;
            P->DisableInput(PC);PC->DisableInput(PC);S->Player=P;
            if(Resume){Check(Save->LoadSaved(),TEXT("new_process_load"));Check(D->State.Stage==Wroclaw::DeliveryStage::Complete && E->BalanceCents==43000 && E->Ledger.Num()==1,TEXT("new_process_progress"));Finish();return false;}
            S->Origin=P->GetActorLocation();S->CameraDistance=FVector::Dist(P->Camera->GetComponentLocation(),S->Origin);
            S->CameraBack=(P->Camera->GetComponentLocation()-P->Boom->GetComponentLocation()).GetSafeNormal();
            UE_LOG(LogTemp,Display,TEXT("WTG_SESSION_CAMERA initial_distance=%.2f back=%s"),S->CameraDistance,*S->CameraBack.ToString());
            auto Block=[&](const FVector& Location,const FVector& Extent){auto* A=W->SpawnActor<AActor>();auto* B=NewObject<UBoxComponent>(A);A->SetRootComponent(B);B->SetBoxExtent(Extent);B->SetCollisionProfileName(TEXT("BlockAll"));B->RegisterComponent();A->SetActorLocation(Location);return A;};
            S->Wall=Block(S->Origin+FVector(240,0,0),FVector(20,120,160));
            S->CameraWall=Block((P->Camera->GetComponentLocation()+S->Origin)*.5,FVector(55,55,160));Advance(1);
        }
        if(S->Stage==1)
        {
            auto* P=S->Player.Get();P->AddMovementInput(FVector::ForwardVector,1,true);
            S->CameraWall->SetActorLocation(P->Boom->GetComponentLocation()+S->CameraBack*225);
            if(S->Elapsed-S->StepAt>1.2)
            {
                const float Moved=FVector::Dist2D(P->GetActorLocation(),S->Origin);
                Check(Moved>50 && Moved<220,TEXT("walk_wall_collision"));Check(P->GetCharacterMovement()->MovementMode==MOVE_Walking,TEXT("floor"));
                Check(FVector::Dist(P->Camera->GetComponentLocation(),P->GetActorLocation())<S->CameraDistance*.8,TEXT("camera_wall"));
                UE_LOG(LogTemp,Display,TEXT("WTG_SESSION_CAMERA obstructed_distance=%.2f collision=%d"),FVector::Dist(P->Camera->GetComponentLocation(),P->GetActorLocation()),P->Boom->bDoCollisionTest);
                S->Wall->Destroy();S->CameraWall->Destroy();P->Jump();Advance(2);
            }
        }
        if(S->Stage==2){if(S->Elapsed-S->StepAt>.15){Check(S->Player->GetCharacterMovement()->IsFalling(),TEXT("jump"));Advance(3);}}
        if(S->Stage==3)
        {
            Walk(R->MeetingPoint);
            if(FVector::Dist2D(S->Player->GetActorLocation(),R->MeetingPoint)<200)
            {D->Interact(false,S->Player.Get());Check(D->State.Stage==Wroclaw::DeliveryStage::GetVehicle,TEXT("giver_start"));Car->Health=0;Advance(4);}
        }
        if(S->Stage==4 && S->Elapsed-S->StepAt>.2)
        {Check(D->State.Stage==Wroclaw::DeliveryStage::Failed,TEXT("lost_vehicle_failure"));Check(D->Restart(),TEXT("restart"));Check(Car->Health==100 && D->State.Stage==Wroclaw::DeliveryStage::GetVehicle,TEXT("restart_restores"));Advance(5);}
        if(S->Stage==5)
        {
            Walk(Car->GetActorLocation()+Car->GetActorRightVector()*190);
            if(S->Elapsed>S->NextDiagnostic){S->NextDiagnostic=S->Elapsed+5;UE_LOG(LogTemp,Display,TEXT("WTG_SESSION_WALK player=%s car=%s"),*S->Player->GetActorLocation().ToString(),*Car->GetActorLocation().ToString());}
            if(FVector::Dist2D(S->Player->GetActorLocation(),Car->GetActorLocation())<340)
            {Check(S->Player->Interaction->Interact(Car),TEXT("vehicle_interaction_range"));Check(Car->Driver==S->Player.Get(),TEXT("enter"));Advance(6);}
        }
        if(S->Stage==6 && S->Elapsed-S->StepAt>.8)
        {
            Check(Car->Exit(),TEXT("safe_exit"));Check(D->State.Active(),TEXT("early_exit_preserves_mission"));Car->Interact(S->Player.Get());Check(Car->Driver==S->Player.Get(),TEXT("reenter"));
            auto* AI=NewObject<UVehicleAIDriverComponent>(Car);AI->RegisterComponent();AI->CruiseSpeed=600;AI->WaypointRadius=170;S->AI=AI;
            Check(AI->ConfigureRoute(R->DeliveryRoute,0,false),TEXT("physical_route_start"));S->LastCar=Car->GetActorLocation();Advance(7);
        }
        if(S->Stage==7)
        {
            S->DriveDistance+=FVector::Dist2D(Car->GetActorLocation(),S->LastCar);S->LastCar=Car->GetActorLocation();
            if(S->Elapsed>S->NextDiagnostic){S->NextDiagnostic=S->Elapsed+10;UE_LOG(LogTemp,Display,TEXT("WTG_SESSION_DRIVE position=%s speed=%.1f health=%.1f target_m=%.1f wanted=%d"),*Car->GetActorLocation().ToString(),Car->Speed,Car->Health,FVector::Dist2D(Car->GetActorLocation(),R->DeliveryPoint)/100,G->WantedLevel());}
            if(FVector::Dist2D(Car->GetActorLocation(),R->DeliveryPoint)<700){S->AI->StopAI();Car->SetAIControl(true,0,0,true);Advance(8);}
            if(Car->Health<=0 || !D->State.Active()){Check(false,TEXT("drive_survival"));Finish();return false;}
        }
        if(S->Stage==8 && FMath::Abs(Car->Speed)<50)
        {Car->SetAIControl(false);Check(S->DriveDistance>35000,TEXT("physical_drive_distance"));Check(Car->Exit(),TEXT("delivery_exit"));Advance(9);}
        if(S->Stage==9)
        {
            Walk(R->DeliveryPoint);
            if(FVector::Dist2D(S->Player->GetActorLocation(),R->DeliveryPoint)<200)
            {
                if(G->WantedLevel()>0){if(S->Elapsed>S->NextDiagnostic){S->NextDiagnostic=S->Elapsed+5;UE_LOG(LogTemp,Display,TEXT("WTG_SESSION_WAIT_POLICE heat=%.1f"),G->Wanted.Heat);}return true;}
                D->Interact(true,S->Player.Get());Check(D->State.Stage==Wroclaw::DeliveryStage::Complete && E->BalanceCents==43000,TEXT("delivery_reward_saved"));
                D->Interact(true,S->Player.Get());Check(E->BalanceCents==43000 && E->Ledger.Num()==1,TEXT("reward_once"));Check(Save->HasSave(),TEXT("save_exists"));
                G->ReportCrime(15,TEXT("Test reakcji policji"));Advance(10);
            }
        }
        if(S->Stage==10 && S->Elapsed-S->StepAt>9)
        {Check(W->GetSubsystem<UVehiclePursuitSubsystem>()->ActivePursuerCount()>0,TEXT("police_dispatch"));Check(Save->LoadSaved(),TEXT("saved_progress_restored"));Check(E->BalanceCents==43000 && D->State.Stage==Wroclaw::DeliveryStage::Complete,TEXT("saved_progress"));Finish();return false;}
        return true;
    }));
 }));
#endif
