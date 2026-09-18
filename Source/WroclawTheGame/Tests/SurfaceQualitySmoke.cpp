#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "World/SurfaceQualityDirector.h"

// Explicit render-only harness. Never starts a campaign, teleports the pawn or writes saves.
static FAutoConsoleCommandWithWorld GSurfaceQualitySmoke(
    TEXT("WTG.SurfaceSmoke"),
    TEXT("Capture dry/wet PBR swatches and existing world; then exit without saving."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld *World) {
        if (!World || !World->IsGameWorld())
            return;
        TWeakObjectPtr<UWorld> WeakWorld(World);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
            [WeakWorld, Elapsed = 0.f, Stage = 0, Camera = TWeakObjectPtr<ACameraActor>(),
             Materials = TArray<TWeakObjectPtr<UMaterialInstanceDynamic>>()](float Dt) mutable {
                UWorld *W = WeakWorld.Get();
                if (!W)
                    return false;
                Elapsed += Dt;
                auto *PC = UGameplayStatics::GetPlayerController(W, 0);
                if (!PC)
                    return true;
                if (Stage == 0 && Elapsed > 2)
                {
                    PC->SetPause(true);
                    if (PC->GetHUD())
                        PC->GetHUD()->bShowHUD = false;
                    Camera = W->SpawnActor<ACameraActor>(FVector(-650, 0, 20170), FRotator(0, 0, 0));
                    Camera->GetCameraComponent()->SetFieldOfView(64);
                    auto &P = Camera->GetCameraComponent()->PostProcessSettings;
                    P.bOverride_AutoExposureMethod = true;
                    P.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
                    P.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
                    P.AutoExposureApplyPhysicalCameraExposure = true;
                    P.bOverride_CameraISO = true;
                    P.CameraISO = 100;
                    P.bOverride_CameraShutterSpeed = true;
                    P.CameraShutterSpeed = 125;
                    P.bOverride_DepthOfFieldFstop = true;
                    P.DepthOfFieldFstop = 8;
                    PC->SetViewTarget(Camera.Get());
                    const TCHAR *Names[] = {TEXT("Brick"), TEXT("Plaster"), TEXT("Concrete"),
                                            TEXT("Wood"),  TEXT("Asphalt"), TEXT("Cobble"),
                                            TEXT("Metal"), TEXT("Fabric")};
                    for (int32 I = 0; I < 8; ++I)
                    {
                        auto *Actor = W->SpawnActor<AStaticMeshActor>(
                            FVector(0, (I % 4 - 1.5f) * 170, 20080 + (I < 4 ? 170 : 0)),
                            FRotator::ZeroRotator);
                        auto *Mesh = Actor->GetStaticMeshComponent();
                        Mesh->SetMobility(EComponentMobility::Movable);
                        Mesh->SetStaticMesh(
                            LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
                        Actor->SetActorScale3D(FVector(.2f, 1.5f, 1.5f));
                        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                        const FString Path = FString::Printf(
                            TEXT("/Game/SurfaceQuality/Instances/MI_%s.MI_%s"), Names[I], Names[I]);
                        if (auto *Source = LoadObject<UMaterialInterface>(nullptr, *Path))
                        {
                            auto *Material = UMaterialInstanceDynamic::Create(Source, Actor);
                            Material->SetScalarParameterValue(TEXT("WeatherExposure"), 1);
                            Material->SetScalarParameterValue(TEXT("Wetness"), 0);
                            Mesh->SetMaterial(0, Material);
                            Materials.Add(Material);
                        }
                    }
                    UE_LOG(LogTemp, Display, TEXT("WTG_SURFACE_SMOKE_OPEN samples=%d"), Materials.Num());
                    for (const TCHAR *Name :
                         {TEXT("r.TextureStreaming"), TEXT("r.Streaming.LimitPoolSizeToVRAM"),
                          TEXT("r.MaxAnisotropy")})
                        if (auto *Variable = IConsoleManager::Get().FindConsoleVariable(Name))
                            UE_LOG(LogTemp, Display, TEXT("WTG_SURFACE_SETTING %s=%d"), Name,
                                   Variable->GetInt());
                    Stage = 1;
                }
                if (Stage == 1 && Elapsed > 12)
                {
                    FScreenshotRequest::RequestScreenshot(
                        FPaths::ProjectSavedDir() / TEXT("Tests/SurfaceDry.png"), false, false);
                    Stage = 2;
                }
                if (Stage == 2 && Elapsed > 14)
                {
                    for (auto Material : Materials)
                        if (Material.IsValid())
                            Material->SetScalarParameterValue(TEXT("Wetness"), .9f);
                    Stage = 3;
                }
                if (Stage == 3 && Elapsed > 18)
                {
                    FScreenshotRequest::RequestScreenshot(
                        FPaths::ProjectSavedDir() / TEXT("Tests/SurfaceWet.png"), false, false);
                    Stage = 4;
                }
                if (Stage == 4 && Elapsed > 20)
                {
                    Camera->SetActorLocationAndRotation(FVector(1600, -700, 800), FRotator(-10, 65, 0));
                    Camera->GetCameraComponent()->PostProcessBlendWeight = 0;
                    Stage = 5;
                }
                if (Stage == 5 && Elapsed > 25)
                {
                    for (TActorIterator<ASurfaceQualityDirector> It(W); It; ++It)
                        UE_LOG(LogTemp, Display, TEXT("WTG_SURFACE_SMOKE_WORLD migrated=%d wetness=%.2f"),
                               It->ReplacedSlots, It->Wetness);
                    FScreenshotRequest::RequestScreenshot(
                        FPaths::ProjectSavedDir() / TEXT("Tests/SurfaceWorld.png"), false, false);
                    Stage = 6;
                }
                if (Stage == 6 && Elapsed > 27)
                {
                    for (TActorIterator<ADirectionalLight> It(W); It; ++It)
                        It->GetLightComponent()->SetIntensity(.2f);
                    Stage = 7;
                }
                if (Stage == 7 && Elapsed > 34)
                {
                    FScreenshotRequest::RequestScreenshot(
                        FPaths::ProjectSavedDir() / TEXT("Tests/SurfaceNight.png"), false, false);
                    Stage = 8;
                }
                if (Stage == 8 && Elapsed > 36)
                {
                    for (TActorIterator<ADirectionalLight> It(W); It; ++It)
                        It->GetLightComponent()->SetIntensity(25000);
                    Camera->SetActorLocationAndRotation(FVector(300, 450, 510), FRotator(-6, 0, 0));
                    Stage = 9;
                }
                if (Stage == 9 && Elapsed > 43)
                {
                    FScreenshotRequest::RequestScreenshot(
                        FPaths::ProjectSavedDir() / TEXT("Tests/SurfaceInterior.png"), false, false);
                    Stage = 10;
                }
                if (Elapsed > 46)
                {
                    UE_LOG(LogTemp, Display, TEXT("WTG_SURFACE_SMOKE_DONE"));
                    FPlatformMisc::RequestExit(false);
                    return false;
                }
                return true;
            }));
    }));
#endif
