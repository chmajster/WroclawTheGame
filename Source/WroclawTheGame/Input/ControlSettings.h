#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ControlSettings.generated.h"

UCLASS(Config=GameUserSettings)
class WROCLAWTHEGAME_API UWTGControlSettings : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, BlueprintReadOnly, Category="Controls", meta=(ClampMin="0.25", ClampMax="3.0"))
    float MouseSensitivity = 1.0f;

    UPROPERTY(Config, BlueprintReadOnly, Category="Controls")
    bool bInvertY = false;

    static UWTGControlSettings* Get();

    UFUNCTION(BlueprintCallable, Category="Controls")
    void SetMouseSensitivity(float Sensitivity);

    UFUNCTION(BlueprintCallable, Category="Controls")
    void SetInvertY(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="Controls")
    void ResetToDefaults();
};
