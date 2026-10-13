#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "Character/SliceCharacter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

static FAutoConsoleCommandWithWorld GSurfaceDiagnostics(TEXT("WTG.SurfaceDiagnostics"),
 TEXT("Identify actual start-area primitives, instances and screen-ray hits; read only."),
 FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World){
    if(!World || !World->IsGameWorld())return;
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak=TWeakObjectPtr<UWorld>(World),Elapsed=0.f,Selected=false](float Dt)mutable{
        auto* W=Weak.Get();if(!W)return false;Elapsed+=Dt;
        auto* PC=UGameplayStatics::GetPlayerController(W,0);auto* P=UGameplayStatics::GetPlayerPawn(W,0);
        if(Elapsed<50 || !PC || !P || W->IsPaused())return true;
        if(!Selected){
            for(TActorIterator<ACameraActor> It(W);It;++It)if(It->GetActorNameOrLabel()==TEXT("StartLocation_Start_Street")){PC->SetViewTarget(*It);break;}
            Selected=true;Elapsed=50;return true;
        }
        if(Elapsed<54)return true;
        auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Objects,Hits;
        const FVector Anchor=P->GetActorLocation();
        for(TActorIterator<AActor> It(W);It;++It){TArray<UPrimitiveComponent*> Components;It->GetComponents(Components);
            for(auto* Component:Components){if(!Component->IsVisible())continue;
                auto Add=[&](FBoxSphereBounds Bounds,FString Asset,int32 Instance,FTransform Transform){
                    if(Bounds.GetBox().ComputeSquaredDistanceToPoint(Anchor)>FMath::Square(15000.))return;
                    auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("actor"),It->GetActorNameOrLabel());Item->SetStringField(TEXT("class"),Component->GetClass()->GetName());
                    Item->SetStringField(TEXT("component"),Component->GetName());Item->SetStringField(TEXT("asset"),Asset);Item->SetNumberField(TEXT("instance"),Instance);
                    Item->SetStringField(TEXT("bounds"),Bounds.ToString());Item->SetStringField(TEXT("transform"),Transform.ToString());Item->SetNumberField(TEXT("collision"),int(Component->GetCollisionEnabled()));
                    FVector2D Screen=FVector2D::ZeroVector;const bool Projected=PC->ProjectWorldLocationToScreen(Bounds.Origin,Screen);Item->SetBoolField(TEXT("projected"),Projected);Item->SetNumberField(TEXT("screen_x"),Screen.X);Item->SetNumberField(TEXT("screen_y"),Screen.Y);
                    Objects.Add(MakeShared<FJsonValueObject>(Item));
                };
                if(auto* Instances=Cast<UInstancedStaticMeshComponent>(Component)){
                    if(UStaticMesh* Mesh=Instances->GetStaticMesh())for(int32 I=0;I<Instances->GetInstanceCount();++I){FTransform Transform;Instances->GetInstanceTransform(I,Transform,true);Add(Mesh->GetBounds().TransformBy(Transform),Mesh->GetPathName(),I,Transform);}
                }else if(auto* Static=Cast<UStaticMeshComponent>(Component))Add(Component->Bounds,GetPathNameSafe(Static->GetStaticMesh()),INDEX_NONE,Component->GetComponentTransform());
                else if(auto* Skel=Cast<USkeletalMeshComponent>(Component))Add(Component->Bounds,GetPathNameSafe(Skel->GetSkeletalMeshAsset()),INDEX_NONE,Component->GetComponentTransform());
                else Add(Component->Bounds,Component->GetClass()->GetPathName(),INDEX_NONE,Component->GetComponentTransform());
            }
        }
        int32 Width,Height;PC->GetViewportSize(Width,Height);
        for(int32 Y=20;Y<65;Y+=3)for(int32 X=3;X<97;X+=3){
            FVector Start,Direction;PC->DeprojectScreenPositionToWorld(Width*X/100.f,Height*Y/100.f,Start,Direction);
            FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(SurfaceScreen),true,P);
            if(W->LineTraceSingleByChannel(Hit,Start,Start+Direction*30000,ECC_Visibility,Params)){
                auto Item=MakeShared<FJsonObject>();Item->SetNumberField(TEXT("screen_x"),Width*X/100.f);Item->SetNumberField(TEXT("screen_y"),Height*Y/100.f);
                Item->SetStringField(TEXT("actor"),Hit.GetActor()?Hit.GetActor()->GetActorNameOrLabel():TEXT("none"));Item->SetStringField(TEXT("component"),GetNameSafe(Hit.GetComponent()));
                Item->SetStringField(TEXT("point"),Hit.ImpactPoint.ToString());Item->SetNumberField(TEXT("instance"),Hit.Item);Hits.Add(MakeShared<FJsonValueObject>(Item));
            }
        }
        Report->SetArrayField(TEXT("objects"),Objects);Report->SetArrayField(TEXT("screen_hits"),Hits);
        Report->SetStringField(TEXT("map"),W->GetOutermost()->GetName());Report->SetStringField(TEXT("camera"),PC->GetViewTarget()->GetActorNameOrLabel());
        FString Json;auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(Report,Writer);
        FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Realism/surface-runtime.json")));
        UE_LOG(LogTemp,Display,TEXT("WTG_SURFACE_DIAGNOSTICS_DONE objects=%d hits=%d"),Objects.Num(),Hits.Num());FPlatformMisc::RequestExit(false);return false;
    }));
 }));
#endif
