#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WTGAudioSettings.generated.h"

UCLASS(Config=GameUserSettings)
class WROCLAWTHEGAME_API UWTGAudioSettings : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0", ClampMax="1.0"))
    float SFXVolume = 1.0f;

    UPROPERTY(Config, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0", ClampMax="1.0"))
    float UIVolume = 1.0f;

    static UWTGAudioSettings* Get();

    UFUNCTION(BlueprintCallable, Category="Audio")
    void SetSFXVolume(float Volume);

    UFUNCTION(BlueprintCallable, Category="Audio")
    void SetUIVolume(float Volume);

    UFUNCTION(BlueprintCallable, Category="Audio")
    void ResetToDefaults();
};
