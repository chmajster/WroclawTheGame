#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Containers/Ticker.h"
#include "Camera/CameraActor.h"
#include "Character/SliceCharacter.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

static FAutoConsoleCommandWithWorld GCivilianHeroSmoke(TEXT("WTG.CivilianHeroSmoke"),
 TEXT("Inspect the equipped civilian and capture front, back, face, garment and pack on the real start map; exit."),
 FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World){
    if(!World || !World->IsGameWorld())return;
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Realism/Hero");
    IFileManager::Get().MakeDirectory(*Folder,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
      [Weak=TWeakObjectPtr<UWorld>(World),Elapsed=0.f,Next=50.f,Stage=0,
       Camera=TWeakObjectPtr<ACameraActor>(),Folder](float Dt)mutable{
        auto* W=Weak.Get();if(!W)return false;Elapsed+=Dt;
        auto* PC=UGameplayStatics::GetPlayerController(W,0);
        auto* P=Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(W,0));
        if(Elapsed<Next || !PC || !P || !P->Appearance || P->Appearance->bLoading || W->IsPaused())return true;
        const TCHAR* Names[]={TEXT("front"),TEXT("back"),TEXT("face"),TEXT("garment"),TEXT("backpack")};
        if(Stage<10 && Stage%2==0){
            if(!Camera.IsValid())Camera=W->SpawnActor<ACameraActor>();
            const int32 View=Stage/2;
            const float Distance[]={350,-350,125,150,-120};
            const float Height[]={20,20,65,15,35};
            const FVector Target=P->GetActorLocation()+FVector(0,0,Height[View]);
            const FVector Location=Target+P->GetActorForwardVector()*Distance[View];
            Camera->SetActorLocation(Location);Camera->SetActorRotation((Target-Location).Rotation());
            PC->SetViewTarget(Camera.Get());W->GetGameViewport()->SetViewMode(VMI_Lit);
            Next=Elapsed+5;Stage++;return true;
        }
        if(Stage<10){
            FScreenshotRequest::RequestScreenshot(Folder/(FString(Names[Stage/2])+TEXT(".png")),true,false);
            Next=Elapsed+1;Stage++;return true;
        }
        auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Meshes;
        TArray<USkeletalMeshComponent*> Components;P->GetComponents(Components);int32 Visible=0;
        for(auto* Component:Components){
            auto* Mesh=Component->GetSkeletalMeshAsset();if(!Mesh || !Component->IsVisible())continue;
            auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("mesh"),Mesh->GetPathName());
            Item->SetStringField(TEXT("leader"),Component->LeaderPoseComponent.IsValid()?Component->LeaderPoseComponent->GetName():TEXT("self"));
            Item->SetNumberField(TEXT("lod"),Component->GetPredictedLODLevel());
            Item->SetStringField(TEXT("bounds"),Component->Bounds.ToString());
            Meshes.Add(MakeShared<FJsonValueObject>(Item));
            if(Mesh->GetPathName().StartsWith(TEXT("/Game/Wroclaw/Hero/")))++Visible;
        }
        const bool OK=Visible==7 && !P->Appearance->bUsingPlaceholder;
        Report->SetStringField(TEXT("status"),OK?TEXT("PASS_REQUIRES_VISUAL_REVIEW"):TEXT("FAIL"));
        Report->SetStringField(TEXT("map"),W->GetOutermost()->GetName());Report->SetArrayField(TEXT("meshes"),Meshes);
        Report->SetStringField(TEXT("body_rotation"),P->Appearance->BodyMesh->GetComponentRotation().ToString());
        Report->SetStringField(TEXT("player_rotation"),P->GetActorRotation().ToString());
        FString Json;auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(Report,Writer);
        FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("integration.json")));
        UE_LOG(LogTemp,Display,TEXT("WTG_CIVILIAN_HERO_DONE visible=%d result=%s"),Visible,OK?TEXT("PASS"):TEXT("FAIL"));
        FPlatformMisc::RequestExit(false);return false;
      }));
 }));
#endif
