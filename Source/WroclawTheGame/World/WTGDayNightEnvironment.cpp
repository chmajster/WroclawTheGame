#include "World/WTGDayNightEnvironment.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

AWTGDayNightEnvironment::AWTGDayNightEnvironment()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
    SunLight->SetupAttachment(SceneRoot);
    SunLight->SetMobility(EComponentMobility::Movable);
    SunLight->SetAtmosphereSunLight(true);
    SunLight->SetAtmosphereSunLightIndex(0);

    MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonLight"));
    MoonLight->SetupAttachment(SceneRoot);
    MoonLight->SetMobility(EComponentMobility::Movable);
    MoonLight->SetAtmosphereSunLight(true);
    MoonLight->SetAtmosphereSunLightIndex(1);

    SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
    SkyLight->SetupAttachment(SceneRoot);
    SkyLight->SetMobility(EComponentMobility::Movable);
    SkyLight->SetRealTimeCapture(true);

    SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
    SkyAtmosphere->SetupAttachment(SceneRoot);

    VolumetricClouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("VolumetricClouds"));
    VolumetricClouds->SetupAttachment(SceneRoot);

    HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
    HeightFog->SetupAttachment(SceneRoot);
    HeightFog->SetVolumetricFog(true);
}

void AWTGDayNightEnvironment::BeginPlay()
{
    Super::BeginPlay();
    ApplyEnvironmentState();
}

void AWTGDayNightEnvironment::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bAdvanceTime && DayLengthMinutes > KINDA_SMALL_NUMBER)
    {
        TimeOfDayHours = FMath::Fmod(TimeOfDayHours + DeltaSeconds * (24.0f / (DayLengthMinutes * 60.0f)), 24.0f);
        if (TimeOfDayHours < 0.0f)
        {
            TimeOfDayHours += 24.0f;
        }
    }

    ApplyEnvironmentState();
}

void AWTGDayNightEnvironment::SetTimeOfDay(float NewTimeOfDayHours)
{
    TimeOfDayHours = FMath::Fmod(NewTimeOfDayHours, 24.0f);
    if (TimeOfDayHours < 0.0f)
    {
        TimeOfDayHours += 24.0f;
    }
    ApplyEnvironmentState();
}

float AWTGDayNightEnvironment::GetDaylightAlpha() const
{
    const float SolarRadians = (TimeOfDayHours - 6.0f) / 24.0f * 2.0f * PI;
    const float Altitude = FMath::Sin(SolarRadians);
    const float TwilightAdjusted = FMath::Clamp((Altitude + 0.08f) / 0.28f, 0.0f, 1.0f);
    return FMath::InterpEaseInOut(0.0f, 1.0f, TwilightAdjusted, 2.0f);
}

void AWTGDayNightEnvironment::ApplyEnvironmentState()
{
    const float Daylight = GetDaylightAlpha();
    const float Night = 1.0f - Daylight;
    const float SunPitch = (TimeOfDayHours / 24.0f) * 360.0f - 90.0f;
    const float MoonPitch = FMath::Fmod(SunPitch + 180.0f, 360.0f);

    SunLight->SetWorldRotation(FRotator(SunPitch, SunAzimuthDegrees, 0.0f));
    MoonLight->SetWorldRotation(FRotator(MoonPitch, SunAzimuthDegrees + 180.0f, 0.0f));

    SunLight->SetIntensity(SunMaxLux * FMath::Pow(Daylight, 1.35f));
    MoonLight->SetIntensity(MoonMaxLux * FMath::Pow(Night, 1.5f));

    const FLinearColor HorizonSun(1.0f, 0.58f, 0.36f);
    const FLinearColor NoonSun(1.0f, 0.97f, 0.91f);
    SunLight->SetLightColor(FLinearColor::LerpUsingHSV(HorizonSun, NoonSun, Daylight));
    MoonLight->SetLightColor(FLinearColor(0.54f, 0.65f, 1.0f));

    SkyLight->SetIntensity(FMath::Lerp(0.06f, 1.1f, Daylight));
    HeightFog->SetFogDensity(FMath::Lerp(NightFogDensity, DayFogDensity, Daylight));

    SetMaterialScalar(TEXT("TimeOfDay01"), TimeOfDayHours / 24.0f);
    SetMaterialScalar(TEXT("SunAlpha"), Daylight);
    SetMaterialScalar(TEXT("NightAlpha"), Night);
    SetMaterialScalar(TEXT("StreetLight"), FMath::SmoothStep(0.35f, 0.8f, Night));
    SetMaterialScalar(TEXT("WindowEmissive"), FMath::Lerp(0.05f, 1.0f, Night));
}

void AWTGDayNightEnvironment::SetMaterialScalar(FName ParameterName, float Value) const
{
    if (!MaterialParameters || !GetWorld())
    {
        return;
    }

    if (UMaterialParameterCollectionInstance* Instance = GetWorld()->GetParameterCollectionInstance(MaterialParameters))
    {
        Instance->SetScalarParameterValue(ParameterName, Value);
    }
}
