#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WTGDayNightEnvironment.generated.h"

class USceneComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UVolumetricCloudComponent;
class UExponentialHeightFogComponent;
class UMaterialParameterCollection;

UCLASS(BlueprintType)
class WROCLAWTHEGAME_API AWTGDayNightEnvironment : public AActor
{
    GENERATED_BODY()

public:
    AWTGDayNightEnvironment();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Environment")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Environment")
    TObjectPtr<UDirectionalLightComponent> SunLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Environment")
    TObjectPtr<UDirectionalLightComponent> MoonLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Environment")
    TObjectPtr<USkyLightComponent> SkyLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Environment")
    TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Environment")
    TObjectPtr<UVolumetricCloudComponent> VolumetricClouds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Environment")
    TObjectPtr<UExponentialHeightFogComponent> HeightFog;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Time", meta=(ClampMin="0.0", ClampMax="24.0"))
    float TimeOfDayHours = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Time", meta=(ClampMin="0.1"))
    float DayLengthMinutes = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Time")
    bool bAdvanceTime = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lighting")
    float SunAzimuthDegrees = -35.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lighting", meta=(ClampMin="0.0"))
    float SunMaxLux = 110000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lighting", meta=(ClampMin="0.0"))
    float MoonMaxLux = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atmosphere", meta=(ClampMin="0.0"))
    float DayFogDensity = 0.0025f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atmosphere", meta=(ClampMin="0.0"))
    float NightFogDensity = 0.009f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials")
    TObjectPtr<UMaterialParameterCollection> MaterialParameters;

    UFUNCTION(BlueprintCallable, Category="Time")
    void SetTimeOfDay(float NewTimeOfDayHours);

    UFUNCTION(BlueprintPure, Category="Time")
    float GetTimeOfDay() const { return TimeOfDayHours; }

    UFUNCTION(BlueprintPure, Category="Time")
    float GetDaylightAlpha() const;

    UFUNCTION(BlueprintPure, Category="Time")
    float GetNightAlpha() const { return 1.0f - GetDaylightAlpha(); }

private:
    void ApplyEnvironmentState();
    void SetMaterialScalar(FName ParameterName, float Value) const;
};
