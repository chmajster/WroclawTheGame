#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PerformanceSettings.generated.h"

UCLASS(Config=GameUserSettings)
class WROCLAWTHEGAME_API UWTGPerformanceSettings : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, BlueprintReadOnly, Category="Display")
    bool bShowFPS = false;

    // 0 means uncapped. VSync may still cap the effective frame rate to the display refresh rate.
    UPROPERTY(Config, BlueprintReadOnly, Category="Display", meta=(ClampMin="0", ClampMax="1000"))
    int32 FPSLimit = 60;

    UPROPERTY(Config, BlueprintReadOnly, Category="Interface")
    bool bReduceUIMotion = false;

    UPROPERTY(Config, BlueprintReadOnly, Category="Interface")
    bool bMenuBackgroundBlur = true;

    UPROPERTY(Config, BlueprintReadOnly, Category="Interface")
    bool bUISounds = true;

    static UWTGPerformanceSettings* Get();

    UFUNCTION(BlueprintCallable, Category="Display")
    void SetShowFPS(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="Display")
    void SetFPSLimit(int32 Limit);

    UFUNCTION(BlueprintCallable, Category="Interface")
    void SetReduceUIMotion(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="Interface")
    void SetMenuBackgroundBlur(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="Interface")
    void SetUISounds(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="Display")
    void Apply() const;

    UFUNCTION(BlueprintCallable, Category="Settings")
    void ResetToDefaults();
};
