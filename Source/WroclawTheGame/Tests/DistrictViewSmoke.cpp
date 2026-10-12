#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Geography/DistrictRegistry.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

static FAutoConsoleCommandWithWorld GDistrictViewSmoke(TEXT("WTG.DistrictViewSmoke"),
 TEXT("Capture authored district views using the actual UE renderer; exit without saving."),
 FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World){
    if (!World || !World->IsGameWorld())return;
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
      [Weak=TWeakObjectPtr<UWorld>(World),Elapsed=0.f,Stage=0,Camera=TWeakObjectPtr<ACameraActor>(),Frames=TArray<float>()](float Dt) mutable {
        UWorld* W=Weak.Get();if(!W)return false;
        Elapsed+=Dt;
        auto* PC=UGameplayStatics::GetPlayerController(W,0);if(!PC)return true;
        AWTGDistrictRegistry* R=nullptr;
        for(TActorIterator<AWTGDistrictRegistry> It(W);It;++It){R=*It;break;}
        if(!R)return true;
        if(Stage==0 && Elapsed>8){
            Camera=W->SpawnActor<ACameraActor>();Camera->GetCameraComponent()->SetFieldOfView(70);
            const FVector P=R->PlayerStart+FVector(-650,-500,320);
            Camera->SetActorLocationAndRotation(P,(R->MeetingPoint+FVector(0,0,100)-P).Rotation());
            PC->SetViewTarget(Camera.Get());Stage=1;
        }
        if(Elapsed>52 && Elapsed<74)Frames.Add(Dt*1000);
        if(Stage==1 && Elapsed>60){
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Tests/DistrictStreet.png"),true,false);Stage=2;
        }
        if(Stage==2 && Elapsed>63){
            const FVector P=R->DeliveryPoint+FVector(-2200,-1300,900);
            Camera->SetActorLocationAndRotation(P,(R->DeliveryPoint+FVector(1200,1600,0)-P).Rotation());Stage=3;
        }
        if(Stage==3 && Elapsed>74){
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Tests/DistrictRiver.png"),true,false);Stage=4;
        }
        if(Stage==4 && Elapsed>77){
            if(!Frames.IsEmpty()){
                float Total=0;for(float V:Frames)Total+=V;Frames.Sort();
                UE_LOG(LogTemp,Display,TEXT("WTG_DISTRICT_RENDER_DONE frames=%d mean_ms=%.2f p95_ms=%.2f screenshots=DistrictStreet.png,DistrictRiver.png"),Frames.Num(),Total/Frames.Num(),Frames[FMath::Min(Frames.Num()-1,FMath::FloorToInt(Frames.Num()*.95f))]);
            }
            FPlatformMisc::RequestExit(false);return false;
        }
        return true;
    }));
 }));
#endif
