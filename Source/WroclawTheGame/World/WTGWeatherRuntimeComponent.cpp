#include "World/WTGWeatherRuntimeComponent.h"

#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

UWTGWeatherRuntimeComponent::UWTGWeatherRuntimeComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UWTGWeatherRuntimeComponent::BeginPlay()
{
    Super::BeginPlay();
    RuntimeState = PresetFor(CurrentWeather);
    TransitionStartState = RuntimeState;
    TransitionTargetState = RuntimeState;
    TargetWeather = CurrentWeather;
    TransitionAlpha = 1.0f;
    PublishMaterialParameters();
}

void UWTGWeatherRuntimeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (TransitionAlpha < 1.0f)
    {
        const float Duration = FMath::Max(TransitionSeconds, KINDA_SMALL_NUMBER);
        TransitionAlpha = FMath::Clamp(TransitionAlpha + DeltaTime / Duration, 0.0f, 1.0f);
        const float Smoothed = FMath::InterpEaseInOut(0.0f, 1.0f, TransitionAlpha, 2.0f);
        RuntimeState = LerpState(TransitionStartState, TransitionTargetState, Smoothed);

        if (TransitionAlpha >= 1.0f)
        {
            CurrentWeather = TargetWeather;
        }

        PublishMaterialParameters();
    }
}

void UWTGWeatherRuntimeComponent::SetWeather(EWTGWeatherType NewWeather, bool bInstant)
{
    TargetWeather = NewWeather;
    TransitionStartState = RuntimeState;
    TransitionTargetState = PresetFor(NewWeather);

    if (bInstant || TransitionSeconds <= KINDA_SMALL_NUMBER)
    {
        TransitionAlpha = 1.0f;
        RuntimeState = TransitionTargetState;
        CurrentWeather = NewWeather;
        PublishMaterialParameters();
        return;
    }

    TransitionAlpha = 0.0f;
}

FWTGWeatherRuntimeState UWTGWeatherRuntimeComponent::PresetFor(EWTGWeatherType Weather)
{
    FWTGWeatherRuntimeState State;

    switch (Weather)
    {
    case EWTGWeatherType::Cloudy:
        State.Visibility = 0.95f; State.AISight = 0.98f; State.CrowdMultiplier = 0.95f;
        State.TrafficSpeedMultiplier = 0.98f; State.StreetWetness = 0.05f; State.CloudCoverage = 0.72f;
        State.FogMultiplier = 1.1f; State.WindStrength = 0.25f;
        break;
    case EWTGWeatherType::Rain:
        State.Traction = 0.82f; State.Visibility = 0.75f; State.AISight = 0.8f; State.CrowdMultiplier = 0.55f;
        State.TrafficSpeedMultiplier = 0.78f; State.StreetWetness = 1.0f; State.CloudCoverage = 0.92f;
        State.Precipitation = 0.8f; State.FogMultiplier = 1.35f; State.WindStrength = 0.45f;
        break;
    case EWTGWeatherType::Fog:
        State.Traction = 0.95f; State.Visibility = 0.42f; State.AISight = 0.5f; State.CrowdMultiplier = 0.65f;
        State.TrafficSpeedMultiplier = 0.68f; State.StreetWetness = 0.25f; State.CloudCoverage = 0.65f;
        State.FogMultiplier = 3.0f; State.WindStrength = 0.08f;
        break;
    case EWTGWeatherType::Storm:
        State.Traction = 0.72f; State.Visibility = 0.55f; State.AISight = 0.62f; State.CrowdMultiplier = 0.25f;
        State.TrafficSpeedMultiplier = 0.58f; State.StreetWetness = 1.0f; State.CloudCoverage = 1.0f;
        State.Precipitation = 1.0f; State.FogMultiplier = 1.7f; State.WindStrength = 1.0f; State.Lightning = 1.0f;
        break;
    case EWTGWeatherType::Clear:
    default:
        break;
    }

    return State;
}

FWTGWeatherRuntimeState UWTGWeatherRuntimeComponent::LerpState(const FWTGWeatherRuntimeState& A, const FWTGWeatherRuntimeState& B, float Alpha)
{
    FWTGWeatherRuntimeState R;
    R.Traction = FMath::Lerp(A.Traction, B.Traction, Alpha);
    R.Visibility = FMath::Lerp(A.Visibility, B.Visibility, Alpha);
    R.AISight = FMath::Lerp(A.AISight, B.AISight, Alpha);
    R.CrowdMultiplier = FMath::Lerp(A.CrowdMultiplier, B.CrowdMultiplier, Alpha);
    R.TrafficSpeedMultiplier = FMath::Lerp(A.TrafficSpeedMultiplier, B.TrafficSpeedMultiplier, Alpha);
    R.StreetWetness = FMath::Lerp(A.StreetWetness, B.StreetWetness, Alpha);
    R.CloudCoverage = FMath::Lerp(A.CloudCoverage, B.CloudCoverage, Alpha);
    R.Precipitation = FMath::Lerp(A.Precipitation, B.Precipitation, Alpha);
    R.FogMultiplier = FMath::Lerp(A.FogMultiplier, B.FogMultiplier, Alpha);
    R.WindStrength = FMath::Lerp(A.WindStrength, B.WindStrength, Alpha);
    R.Lightning = FMath::Lerp(A.Lightning, B.Lightning, Alpha);
    return R;
}

void UWTGWeatherRuntimeComponent::PublishMaterialParameters() const
{
    if (!MaterialParameters || !GetWorld())
    {
        return;
    }

    if (UMaterialParameterCollectionInstance* Instance = GetWorld()->GetParameterCollectionInstance(MaterialParameters))
    {
        Instance->SetScalarParameterValue(TEXT("WeatherWetness"), RuntimeState.StreetWetness);
        Instance->SetScalarParameterValue(TEXT("WeatherCloudCoverage"), RuntimeState.CloudCoverage);
        Instance->SetScalarParameterValue(TEXT("WeatherPrecipitation"), RuntimeState.Precipitation);
        Instance->SetScalarParameterValue(TEXT("WeatherFogMultiplier"), RuntimeState.FogMultiplier);
        Instance->SetScalarParameterValue(TEXT("WeatherWind"), RuntimeState.WindStrength);
        Instance->SetScalarParameterValue(TEXT("WeatherLightning"), RuntimeState.Lightning);
    }
}
