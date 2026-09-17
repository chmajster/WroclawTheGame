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
    PrimaryActorTick.TickInterval = .5;
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
    Sun->GetLightComponent()->SetIntensity(2.5f);
    auto *Sky = GetWorld()->SpawnActor<ASkyLight>();
    Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->SetIntensity(.55);
    Fog = GetWorld()->SpawnActor<AExponentialHeightFog>();
    AudioLoop(RoomAudio, TEXT("Apartment"), .22);
    AudioLoop(StreetAudio, TEXT("Street"), .05);
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
    RoomAudio->SetVolumeMultiplier(Room ? .22 : 0);
    StreetAudio->SetVolumeMultiplier(Room ? .05 : .3);
    ChaseAudio->SetVolumeMultiplier(M->Threat() ? .38 : 0);
    const auto *W = M->WorldState.Weather();
    if (Fog && W)
        Fog->GetComponent()->SetFogDensity(W->fog);
    if (Sun)
    {
        Sun->SetActorRotation(FRotator((M->WorldState.hour - 6) * -15, -35, 0));
        Sun->GetLightComponent()->SetIntensity(M->WorldState.hour > 6 && M->WorldState.hour < 20 ? 2.5 : .08);
    }
}
