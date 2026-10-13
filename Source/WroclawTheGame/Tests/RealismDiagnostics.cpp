#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Containers/Ticker.h"
#include "Camera/CameraActor.h"
#include "Character/SliceCharacter.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "StaticMeshResources.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Tests/RealismCaptureReady.h"

static FAutoConsoleCommandWithWorld GRealismDiagnostics(TEXT("WTG.RealismDiagnostics"),
 TEXT("Capture lit, unlit, wireframe, lighting, normals and collision on the real start map; exit."),
 FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) {
    if (!World || !World->IsGameWorld()) return;
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Realism/Diagnostics");
    IFileManager::Get().MakeDirectory(*Folder,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
      [Weak=TWeakObjectPtr<UWorld>(World),Elapsed=0.f,NextAt=50.f,Stage=0,Shots=TArray<FString>(),Folder](float Dt) mutable {
        auto* W=Weak.Get();if(!W)return false;Elapsed+=Dt;
        auto* PC=UGameplayStatics::GetPlayerController(W,0);
        auto* P=Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(W,0));
        if(Elapsed<NextAt || !PC || !P || !P->Appearance || P->Appearance->bLoading || W->IsPaused())return true;
        if(!WTGRealismCaptureReady()){NextAt=Elapsed+4;return true;}
        auto* Viewport=W->GetGameViewport();if(!Viewport)return true;
        const EViewModeIndex Modes[]={VMI_Lit,VMI_Unlit,VMI_Wireframe,VMI_LightingOnly,VMI_VisualizeBuffer,VMI_CollisionPawn};
        const TCHAR* Names[]={TEXT("lit"),TEXT("unlit"),TEXT("wireframe"),TEXT("lighting"),TEXT("normals"),TEXT("collision")};
        // A stage sets a mode/camera; the following tick after 4 seconds captures it.
        if(Stage%2==0 && Stage<36) {
            const int32 Index=Stage/2,Mode=Index%6;
            const TCHAR* Camera=Index<6?TEXT("StartLocation_Start_Street"):(Index<12?TEXT("StartLocation_Start_Park"):TEXT("StartLocation_Start_Facade"));
            for(TActorIterator<ACameraActor> It(W);It;++It)
                if(It->GetActorNameOrLabel()==Camera){PC->SetViewTarget(*It);break;}
            if(Mode==4)if(auto* CV=IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationTarget")))CV->Set(TEXT("WorldNormal"),ECVF_SetByConsole);
            Viewport->SetViewMode(Modes[Mode]);NextAt=Elapsed+4;Stage++;return true;
        }
        if(Stage<36) {
            const int32 Index=Stage/2,Mode=Index%6;
            const FString Name=FString::Printf(TEXT("%s_%s.png"),Index<6?TEXT("street"):(Index<12?TEXT("park"):TEXT("facade")),Names[Mode]);
            FScreenshotRequest::RequestScreenshot(Folder/Name,true,false);Shots.Add(Name);
            NextAt=Elapsed+1;Stage++;return true;
        }
        Viewport->SetViewMode(VMI_Lit);
        auto Report=MakeShared<FJsonObject>();
        Report->SetStringField(TEXT("map"),W->GetOutermost()->GetName());
        Report->SetStringField(TEXT("status"),TEXT("CAPTURED_REQUIRES_INSPECTION"));
        Report->SetStringField(TEXT("captured_utc"),FDateTime::UtcNow().ToIso8601());
        TArray<TSharedPtr<FJsonValue>> MeshBasis;
        for(TActorIterator<AActor> It(W);It;++It) {
            const FString Label=It->GetActorNameOrLabel();
            if(!Label.StartsWith(TEXT("StartLocation_Building_")) && !Label.StartsWith(TEXT("StartLocation_Context_")))continue;
            auto* Component=It->FindComponentByClass<UStaticMeshComponent>();
            UStaticMesh* Mesh=Component?Component->GetStaticMesh():nullptr;
            auto* Data=Mesh?Mesh->GetRenderData():nullptr;
            if(!Data || Data->LODResources.IsEmpty())continue;
            const auto& Buffer=Data->LODResources[0].VertexBuffers.StaticMeshVertexBuffer;
            int32 BadX=0,BadZ=0;
            TArray<TSharedPtr<FJsonValue>> Samples;
            for(uint32 Index=0;Buffer.GetTangentData() && Index<Buffer.GetNumVertices();++Index){
                const auto X=Buffer.VertexTangentX(Index),Z=Buffer.VertexTangentZ(Index);
                BadX+=(X.X*X.X+X.Y*X.Y+X.Z*X.Z<.0001f);
                BadZ+=(Z.X*Z.X+Z.Y*Z.Y+Z.Z*Z.Z<.0001f);
                if(X.X*X.X+X.Y*X.Y+X.Z*X.Z<.0001f && Samples.Num()<8){
                    const auto UV=Buffer.GetVertexUV(Index,0);
                    const auto Position=Data->LODResources[0].VertexBuffers.PositionVertexBuffer.VertexPosition(Index);
                    auto Sample=MakeShared<FJsonObject>();
                    Sample->SetStringField(TEXT("position_cm"),Position.ToString());Sample->SetStringField(TEXT("uv"),UV.ToString());
                    Samples.Add(MakeShared<FJsonValueObject>(Sample));
                }
            }
            auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("mesh"),Mesh->GetPathName());
            Item->SetBoolField(TEXT("basis_cpu_available"),Buffer.GetTangentData()!=nullptr);
            Item->SetNumberField(TEXT("vertices"),Buffer.GetNumVertices());
            Item->SetNumberField(TEXT("near_zero_tangents"),BadX);Item->SetNumberField(TEXT("near_zero_normals"),BadZ);
            Item->SetArrayField(TEXT("tangent_samples"),Samples);
            MeshBasis.Add(MakeShared<FJsonValueObject>(Item));
        }
        Report->SetArrayField(TEXT("architecture_render_basis"),MeshBasis);
        TArray<TSharedPtr<FJsonValue>> Values;for(const auto& Name:Shots)Values.Add(MakeShared<FJsonValueString>(Name));
        Report->SetArrayField(TEXT("screenshots"),Values);
        FString Json;auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(Report,Writer);
        FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("capture.json")));
        UE_LOG(LogTemp,Display,TEXT("WTG_REALISM_DIAGNOSTICS_CAPTURED %d"),Shots.Num());
        FPlatformMisc::RequestExit(false);return false;
      }));
 }));
#endif
