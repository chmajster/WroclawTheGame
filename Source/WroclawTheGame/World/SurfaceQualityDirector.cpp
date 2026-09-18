#include "World/SurfaceQualityDirector.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/DecalComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Mission/SliceMission.h"
#include "TimerManager.h"

ASurfaceQualityDirector::ASurfaceQualityDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .5f;
}

void ASurfaceQualityDirector::BeginPlay()
{
    Super::BeginPlay();
    const TCHAR *Names[] = {TEXT("Asphalt"),      TEXT("Concrete"),    TEXT("Plaster"),   TEXT("Brick"),
                            TEXT("Cobble"),       TEXT("Stone"),       TEXT("Wood"),      TEXT("Metal"),
                            TEXT("PaintedMetal"), TEXT("Glass"),       TEXT("Fabric"),    TEXT("Denim"),
                            TEXT("Player"),       TEXT("Enemy"),       TEXT("Paper"),     TEXT("SignBlue"),
                            TEXT("Water"),        TEXT("SignRed"),     TEXT("RoadPaint"), TEXT("Soil"),
                            TEXT("PlasticMatte"), TEXT("PlasticGloss")};
    for (const TCHAR *Name : Names)
    {
        const FString Path = FString::Printf(TEXT("/Game/SurfaceQuality/Instances/MI_%s.MI_%s"), Name, Name);
        if (auto *Material = LoadObject<UMaterialInterface>(nullptr, *Path))
            Surfaces.Add(FName(Name), UMaterialInstanceDynamic::Create(Material, this));
    }
    for (const TCHAR *Name :
         {TEXT("RoadPatchFresh"), TEXT("RoadPatchOld"), TEXT("RoadCrack"), TEXT("RoadSeal")})
    {
        const FString Path = FString::Printf(TEXT("/Game/SurfaceQuality/Decals/MI_%s.MI_%s"), Name, Name);
        if (auto *Material = LoadObject<UMaterialInterface>(nullptr, *Path))
            PatchMaterials.Add(Material);
    }
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
        ApplyActor(*It);
    SpawnHandle = GetWorld()->AddOnActorSpawnedHandler(
        FOnActorSpawned::FDelegate::CreateUObject(this, &ASurfaceQualityDirector::ActorSpawned));
    LevelHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(this, &ASurfaceQualityDirector::LevelAdded);
    Exposure = GetWorld()->SpawnActor<APostProcessVolume>();
    Exposure->bUnbound = true;
    Exposure->Priority = 0; // Authored local volumes may override this baseline.
    auto &P = Exposure->Settings;
    P.bOverride_AutoExposureMethod = true;
    P.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
    P.bOverride_AutoExposureMinBrightness = true;
    P.AutoExposureMinBrightness = 0;
    P.bOverride_AutoExposureMaxBrightness = true;
    P.AutoExposureMaxBrightness = 16;
    P.bOverride_AutoExposureBias = true;
    P.AutoExposureBias = 0;
    P.bOverride_AutoExposureSpeedUp = true;
    P.AutoExposureSpeedUp = 3;
    P.bOverride_AutoExposureSpeedDown = true;
    P.AutoExposureSpeedDown = 1;
    P.bOverride_MotionBlurAmount = true;
    P.MotionBlurAmount = 0;
    P.bOverride_BloomIntensity = true;
    P.BloomIntensity = .15f;
    P.bOverride_ColorSaturation = true;
    P.ColorSaturation = FVector4(.96f, .96f, .96f, 1);
    for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
    {
        auto *Light = Cast<UDirectionalLightComponent>(It->GetLightComponent());
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetAtmosphereSunLight(true);
        Light->SetIntensity(25000);
    }
    for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
    {
        It->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        It->GetLightComponent()->SetIntensity(1);
        It->GetLightComponent()->SetRealTimeCaptureEnabled(true);
    }
    UE_LOG(LogTemp, Display, TEXT("WTG_SURFACES_READY profiles=%d migrated_slots=%d"), Surfaces.Num(),
           ReplacedSlots);
}

void ASurfaceQualityDirector::ActorSpawned(AActor *Actor)
{
    const TWeakObjectPtr<AActor> WeakActor(Actor);
    const TWeakObjectPtr<ASurfaceQualityDirector> WeakSelf(this);
    GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakSelf, WeakActor]() {
        if (WeakSelf.IsValid() && WeakActor.IsValid())
            WeakSelf->ApplyActor(WeakActor.Get());
    }));
}

void ASurfaceQualityDirector::LevelAdded(ULevel *Level, UWorld *World)
{
    if (World != GetWorld() || !Level)
        return;
    for (AActor *Actor : Level->Actors)
        if (IsValid(Actor))
            ApplyActor(Actor);
}

void ASurfaceQualityDirector::ApplyActor(AActor *Actor)
{
    if (!IsValid(Actor) || Actor == this)
        return;
    TInlineComponentArray<UMeshComponent *> Meshes(Actor);
    for (auto *Mesh : Meshes)
    {
        bool Road = false;
        for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
        {
            auto *Old = Mesh->GetMaterial(Slot);
            if (!Old)
                continue;
            const FString Package = Old->GetOutermost()->GetName();
            FString Name = Old->GetName();
            if (Package.StartsWith(TEXT("/Game/Generated/M_")))
                Name.RemoveFromStart(TEXT("M_"));
            else if (Package.StartsWith(TEXT("/Game/SurfaceQuality/Instances/MI_")))
                Name.RemoveFromStart(TEXT("MI_"));
            else
                continue; // Never override an authored/character-specific material.
            if (auto *Static = Cast<UStaticMeshComponent>(Mesh))
                if (UStaticMesh *Asset = Static->GetStaticMesh())
                    if (Asset->GetOutermost()->GetName().StartsWith(TEXT("/Game/Generated/GIS/")))
                    {
                        if (Asset->GetName().StartsWith(TEXT("green_")) && Name == TEXT("Wood"))
                            Name = TEXT("Soil");
                        if (Asset->GetName().StartsWith(TEXT("water_")) && Name == TEXT("Metal"))
                            Name = TEXT("Water");
                    }
            if (auto *Replacement = Surfaces.FindRef(FName(Name)).Get())
            {
                Mesh->SetMaterial(Slot, Replacement);
                ++ReplacedSlots;
                Road |= Name == TEXT("Asphalt");
            }
        }
        if (Road)
            AddRoadPatch(Mesh);
    }
}

void ASurfaceQualityDirector::AddRoadPatch(UMeshComponent *Mesh)
{
    Decals.RemoveAll([](const auto &Item) { return !Item.IsValid(); });
    if (Decals.Num() >= 128 || PatchMaterials.IsEmpty() || Mesh->ComponentHasTag(TEXT("WTG_RoadDetail")))
        return;
    const auto Bounds = Mesh->Bounds;
    if (FMath::Min(Bounds.BoxExtent.X, Bounds.BoxExtent.Y) < 120 || Bounds.BoxExtent.Z > 100)
        return;
    FRandomStream Random(FCrc::StrCrc32(*Mesh->GetPathName()));
    const FVector Point =
        Bounds.Origin + FVector(Random.FRandRange(-.55f, .55f) * Bounds.BoxExtent.X,
                                Random.FRandRange(-.55f, .55f) * Bounds.BoxExtent.Y, Bounds.BoxExtent.Z + 50);
    FHitResult Hit;
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Point, Point - FVector(0, 0, 250), ECC_Visibility) ||
        Hit.GetComponent() != Mesh || Hit.ImpactNormal.Z < .9f)
        return;
    auto *Decal = NewObject<UDecalComponent>(Mesh->GetOwner());
    Decal->SetupAttachment(Mesh);
    Decal->SetAbsolute(true, true, true);
    Decal->DecalSize = FVector(6, Random.FRandRange(35, 80), Random.FRandRange(65, 150));
    Decal->SetDecalMaterial(PatchMaterials[Random.RandRange(0, PatchMaterials.Num() - 1)]);
    Decal->SetFadeScreenSize(.003f);
    Mesh->GetOwner()->AddInstanceComponent(Decal);
    Decal->RegisterComponent();
    Decal->SetWorldLocationAndRotation(Hit.ImpactPoint + Hit.ImpactNormal * 2,
                                       FRotator(-90, Random.FRandRange(0, 360), 0));
    Mesh->ComponentTags.Add(TEXT("WTG_RoadDetail"));
    Decals.Add(Decal);
}

void ASurfaceQualityDirector::SetSurfaceWetness(float Value)
{
    Wetness = FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.f, 1.f) : 0;
    for (const auto &Pair : Surfaces)
        Pair.Value->SetScalarParameterValue(TEXT("Wetness"), Wetness);
}

void ASurfaceQualityDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!FollowWeather)
        return;
    const auto *Profile = GetGameInstance()->GetSubsystem<USliceMission>()->WorldState.Weather();
    const float Target = Profile && (Profile->id == "Rain" || Profile->id == "Storm") ? .85f : 0;
    SetSurfaceWetness(
        FMath::FInterpConstantTo(Wetness, Target, DeltaSeconds, Target > Wetness ? .12f : .018f));
}

void ASurfaceQualityDirector::EndPlay(const EEndPlayReason::Type Reason)
{
    if (GetWorld())
        GetWorld()->RemoveOnActorSpawnedHandler(SpawnHandle);
    FWorldDelegates::LevelAddedToWorld.Remove(LevelHandle);
    if (IsValid(Exposure))
        Exposure->Destroy();
    Super::EndPlay(Reason);
}
