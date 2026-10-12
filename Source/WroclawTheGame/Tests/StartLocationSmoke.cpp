#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Containers/Ticker.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Character/SliceCharacter.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Geography/DistrictRegistry.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

// Uses ordinary character movement and the real renderer. No teleports,
// gameplay transactions, save writes or permanent graphics settings changes.
static FAutoConsoleCommandWithWorld GStartLocationSmoke(TEXT("WTG.StartLocationSmoke"),
 TEXT("Verify the actual start, walk, wall/camera collision and steady rendered frames; exit."),
 FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World){
    if(!World || !World->IsGameWorld())return;
    const FString Output=FPaths::ProjectSavedDir()/TEXT("StartLocation/gameplay-report.json");
    IFileManager::Get().Delete(*Output);
    IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("Tests")),true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
      [Weak=TWeakObjectPtr<UWorld>(World),Elapsed=0.f,ReadyAt=-1.f,Stage=0,WalkShot=false,
       OK=true,FloorOK=false,CameraOK=false,Walls=0,WalkDistance=0.f,
       Initial=FVector::ZeroVector,Frames=TArray<float>(),Output](float Dt) mutable {
        auto* W=Weak.Get();if(!W)return false;Elapsed+=Dt;
        auto* PC=UGameplayStatics::GetPlayerController(W,0);
        auto* P=Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(W,0));
        AWTGDistrictRegistry* R=nullptr;
        for(TActorIterator<AWTGDistrictRegistry> It(W);It;++It){R=*It;break;}
        auto CheckFloor=[&](){
            if(!P)return false;
            FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(StartFloor),false,P);
            const FVector Position=P->GetActorLocation();
            return W->LineTraceSingleByChannel(Hit,Position,Position-FVector(0,0,500),ECC_Visibility,Params)
                && Hit.ImpactNormal.Z>=P->GetCharacterMovement()->GetWalkableFloorZ()
                && FMath::Abs(Position.Z-P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-Hit.ImpactPoint.Z)<25
                && P->GetCharacterMovement()->IsMovingOnGround();
        };
        auto CheckCamera=[&](){
            if(!P || !P->Camera || !P->Boom)return false;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(StartCamera),false,P);
            return P->Boom->bDoCollisionTest
                && FVector::Distance(P->Camera->GetComponentLocation(),P->GetActorLocation())>60
                && !W->OverlapBlockingTestByChannel(P->Camera->GetComponentLocation(),FQuat::Identity,
                    ECC_Camera,FCollisionShape::MakeSphere(8),Params);
        };
        auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Tests")/Name,true,false);};
        auto SetView=[&](const TCHAR* Label){
            for(TActorIterator<ACameraActor> It(W);It;++It)
                if(It->GetActorNameOrLabel()==Label){PC->SetViewTarget(*It);return true;}
            return false;
        };
        if(Stage==0 && Elapsed>50 && PC && P && R && P->Appearance && !P->Appearance->bLoading
            && !P->Appearance->bUsingPlaceholder && P->GetCharacterMovement()->IsMovingOnGround() && !W->IsPaused()){
            // Override console variables in this process only, after menu settings.
            for(const TCHAR* Name:{TEXT("r.VSync"),TEXT("t.MaxFPS"),TEXT("Slate.bAllowThrottling"),TEXT("t.IdleWhenNotForeground")})
                if(auto* Value=IConsoleManager::Get().FindConsoleVariable(Name))Value->Set(0,ECVF_SetByConsole);
            Initial=P->GetActorLocation();FloorOK=CheckFloor();CameraOK=CheckCamera();
            for(TActorIterator<AStaticMeshActor> It(W);It;++It){
                if(!It->GetActorNameOrLabel().StartsWith(TEXT("StartLocation_Building_")))continue;
                auto* Mesh=It->GetStaticMeshComponent();const auto B=Mesh->Bounds;
                const FVector Center(B.Origin.X,B.Origin.Y,B.Origin.Z-B.BoxExtent.Z+150);
                FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(StartWall),false);
                if(Mesh->LineTraceComponent(Hit,Center-FVector(B.BoxExtent.X+100,0,0),Center+FVector(B.BoxExtent.X+100,0,0),Params))++Walls;
            }
            OK=FloorOK && CameraOK && Walls==8 && FVector::Dist2D(Initial,R->PlayerStart)<15;
            ReadyAt=Elapsed;Stage=1;Shot(TEXT("Start_GamePlayer.png"));
            UE_LOG(LogTemp,Display,TEXT("WTG_START_GAME_READY floor=%d camera=%d walls=%d paused=%d"),FloorOK,CameraOK,Walls,W->IsPaused());
        }
        const float Since=ReadyAt>=0?Elapsed-ReadyAt:0;
        if(Stage==1 && P && R){
            if(Since>.5f && Since<20){Frames.Add(Dt*1000);OK&=!W->IsPaused();}
            if(Since>1 && Since<3)P->AddMovementInput((R->MeetingPoint-R->PlayerStart).GetSafeNormal2D(),.4f,true);
            if(Since>4 && !WalkShot){
                WalkDistance=FVector::Dist2D(Initial,P->GetActorLocation());
                FloorOK&=CheckFloor();CameraOK&=CheckCamera();
                OK&=WalkDistance>80 && WalkDistance<450 && FloorOK && CameraOK;
                Shot(TEXT("Start_GameWalk.png"));WalkShot=true;
                UE_LOG(LogTemp,Display,TEXT("WTG_START_GAME_WALK distance_cm=%.2f floor=%d camera=%d"),WalkDistance,FloorOK,CameraOK);
            }
            if(Since>23){OK&=SetView(TEXT("StartLocation_Start_Street"));Stage=2;}
        }
        if(Stage==2 && Since>29){Shot(TEXT("Start_GameStreet.png"));Stage=3;}
        if(Stage==3 && Since>31){OK&=SetView(TEXT("StartLocation_Start_Park"));Stage=4;}
        if(Stage==4 && Since>37){Shot(TEXT("Start_GamePark.png"));Stage=5;}
        if((Stage==5 && Since>40) || Elapsed>240){
            OK&=Stage==5 && Frames.Num()>60;
            double Total=0;for(float Value:Frames)Total+=Value;Frames.Sort();
            const double Mean=Frames.IsEmpty()?0:Total/Frames.Num();
            auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("status"),OK?TEXT("PASS"):TEXT("FAIL"));
            Report->SetStringField(TEXT("captured_utc"),FDateTime::UtcNow().ToIso8601());
            Report->SetNumberField(TEXT("frames"),Frames.Num());Report->SetNumberField(TEXT("mean_ms"),Mean);
            Report->SetNumberField(TEXT("mean_fps"),Mean>0?1000/Mean:0);
            Report->SetNumberField(TEXT("p95_ms"),Frames.IsEmpty()?0:Frames[FMath::Min(Frames.Num()-1,int32(Frames.Num()*.95f))]);
            Report->SetNumberField(TEXT("walk_distance_cm"),WalkDistance);Report->SetNumberField(TEXT("walls_blocking"),Walls);
            Report->SetBoolField(TEXT("floor_valid"),FloorOK);Report->SetBoolField(TEXT("camera_clear"),CameraOK);
            Report->SetBoolField(TEXT("paused"),W->IsPaused());
            Report->SetBoolField(TEXT("character_placeholder"),!P || !P->Appearance || P->Appearance->bUsingPlaceholder);
            const FIntPoint Size=W->GetGameViewport()->Viewport->GetSizeXY();
            Report->SetNumberField(TEXT("width"),Size.X);Report->SetNumberField(TEXT("height"),Size.Y);
            Report->SetStringField(TEXT("measurement"),TEXT("actual game, 50s warmup, uncapped VSync-off wall-frame intervals; includes walk and one capture, not isolated GPU timing"));
            FString Json;auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(Report,Writer);
            if(!FFileHelper::SaveStringToFile(Json,*Output))OK=false;
            UE_LOG(LogTemp,Display,TEXT("WTG_START_GAME_DONE result=%s %s"),OK?TEXT("PASS"):TEXT("FAIL"),*Json);
            FPlatformMisc::RequestExit(false);return false;
        }
        return true;
    }));
 }));
#endif
