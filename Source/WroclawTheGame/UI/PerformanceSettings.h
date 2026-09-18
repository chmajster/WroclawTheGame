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

    static UWTGPerformanceSettings* Get();

    UFUNCTION(BlueprintCallable, Category="Display")
    void SetShowFPS(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="Display")
    void SetFPSLimit(int32 Limit);

    UFUNCTION(BlueprintCallable, Category="Display")
    void Apply() const;
};
