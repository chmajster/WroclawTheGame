#include "World/SliceWorld.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Mission/SliceMission.h"
#include "Components/GameplayComponents.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
ASliceWorld::ASliceWorld()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .5f;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Power = CreateDefaultSubobject<UPowerConsumerComponent>(TEXT("ApartmentPower"));
    PuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PuzzleLight"));
    PuzzleLight->SetupAttachment(RootComponent);
    PuzzleLight->SetRelativeLocation(FVector(700, 400, 620));
    PuzzleLight->SetAttenuationRadius(1200);
}
void ASliceWorld::AudioLoop(TObjectPtr<UAudioComponent> &C, const TCHAR *Name, float Volume)
{
    C = NewObject<UAudioComponent>(this);
    C->SetupAttachment(RootComponent);
    AddInstanceComponent(C);
    const FString Path = FString::Printf(TEXT("/Game/Generated/Audio/%s.%s"), Name, Name);
    C->SetSound(LoadObject<USoundBase>(nullptr, *Path));
    C->bAutoActivate = false;
    C->SetVolumeMultiplier(Volume);
    C->RegisterComponent();
    C->Play();
}
void ASliceWorld::BeginPlay()
{
    Super::BeginPlay();
    Power->RequiredPower = FGameplayTag::RequestGameplayTag(TEXT("Power.Apartment.On"));
    auto *Atmosphere = NewObject<USkyAtmosphereComponent>(this);
    Atmosphere->SetupAttachment(RootComponent);
    AddInstanceComponent(Atmosphere);
    Atmosphere->RegisterComponent();
    Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 2000), FRotator(-32, -35, 0));
    Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sun->GetLightComponent()->SetIntensity(25000.f);
    auto *Sky = GetWorld()->SpawnActor<ASkyLight>();
    Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->SetIntensity(.55f);
    Fog = GetWorld()->SpawnActor<AExponentialHeightFog>();
    AudioLoop(RoomAudio, TEXT("Apartment"), .22f);
    AudioLoop(StreetAudio, TEXT("Street"), .05f);
    AudioLoop(ChaseAudio, TEXT("Chase"), 0);
}
void ASliceWorld::Tick(float Dt)
{
    Super::Tick(Dt);
    auto *M = GetGameInstance()->GetSubsystem<USliceMission>();
    PuzzleLight->SetIntensity(Power->Powered() ? 4200 : 0);
    const Wroclaw::DistrictDef *Active = nullptr;
    for (const auto &D : Wroclaw::Districts())
        if (D.id == M->WorldState.district)
            Active = &D;
    const bool Room = Active && Active->ambient == "Apartment";
    RoomAudio->SetVolumeMultiplier(Room ? .22f : 0.f);
    StreetAudio->SetVolumeMultiplier(Room ? .05f : .3f);
    ChaseAudio->SetVolumeMultiplier(M->Threat() ? .38f : 0.f);
    const auto *W = M->WorldState.Weather();
    if (Fog && W)
        Fog->GetComponent()->SetFogDensity(W->fog);
    if (Sun)
    {
        Sun->SetActorRotation(FRotator((M->WorldState.hour - 6) * -15, -35, 0));
        const bool Day = M->WorldState.hour > 6 && M->WorldState.hour < 20;
        const float Cloud = W && (W->id == "Cloudy" || W->id == "Rain" || W->id == "Storm") ? .3f : 1.f;
        Sun->GetLightComponent()->SetIntensity(Day ? 25000.f * Cloud : .2f);
    }
}
