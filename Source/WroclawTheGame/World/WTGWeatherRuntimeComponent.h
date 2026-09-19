#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WTGWeatherRuntimeComponent.generated.h"

class UMaterialParameterCollection;

UENUM(BlueprintType)
enum class EWTGWeatherType : uint8
{
    Clear,
    Cloudy,
    Rain,
    Fog,
    Storm
};

USTRUCT(BlueprintType)
struct WROCLAWTHEGAME_API FWTGWeatherRuntimeState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) float Traction = 1.0f;
    UPROPERTY(BlueprintReadOnly) float Visibility = 1.0f;
    UPROPERTY(BlueprintReadOnly) float AISight = 1.0f;
    UPROPERTY(BlueprintReadOnly) float CrowdMultiplier = 1.0f;
    UPROPERTY(BlueprintReadOnly) float TrafficSpeedMultiplier = 1.0f;
    UPROPERTY(BlueprintReadOnly) float StreetWetness = 0.0f;
    UPROPERTY(BlueprintReadOnly) float CloudCoverage = 0.0f;
    UPROPERTY(BlueprintReadOnly) float Precipitation = 0.0f;
    UPROPERTY(BlueprintReadOnly) float FogMultiplier = 1.0f;
    UPROPERTY(BlueprintReadOnly) float WindStrength = 0.1f;
    UPROPERTY(BlueprintReadOnly) float Lightning = 0.0f;
};

UCLASS(ClassGroup=(World), meta=(BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API UWTGWeatherRuntimeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWTGWeatherRuntimeComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather", meta=(ClampMin="0.0"))
    float TransitionSeconds = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather")
    TObjectPtr<UMaterialParameterCollection> MaterialParameters;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather")
    EWTGWeatherType CurrentWeather = EWTGWeatherType::Clear;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather")
    EWTGWeatherType TargetWeather = EWTGWeatherType::Clear;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather")
    FWTGWeatherRuntimeState RuntimeState;

    UFUNCTION(BlueprintCallable, Category="Weather")
    void SetWeather(EWTGWeatherType NewWeather, bool bInstant = false);

    UFUNCTION(BlueprintPure, Category="Weather")
    float GetTransitionAlpha() const { return TransitionAlpha; }

private:
    FWTGWeatherRuntimeState TransitionStartState;
    FWTGWeatherRuntimeState TransitionTargetState;
    float TransitionAlpha = 1.0f;

    static FWTGWeatherRuntimeState PresetFor(EWTGWeatherType Weather);
    static FWTGWeatherRuntimeState LerpState(const FWTGWeatherRuntimeState& A, const FWTGWeatherRuntimeState& B, float Alpha);
    void PublishMaterialParameters() const;
};
