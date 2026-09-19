#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WTGInputProfileSubsystem.generated.h"

USTRUCT(BlueprintType)
struct WROCLAWTHEGAME_API FWTGInputProfile
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) float MouseSensitivity = 1.0f;
    UPROPERTY(BlueprintReadWrite) float GamepadSensitivity = 1.0f;
    UPROPERTY(BlueprintReadWrite) float GamepadDeadzone = 0.15f;
    UPROPERTY(BlueprintReadWrite) float UIScale = 1.0f;
    UPROPERTY(BlueprintReadWrite) bool bSprintToggle = false;
    UPROPERTY(BlueprintReadWrite) bool bCrouchToggle = true;
    UPROPERTY(BlueprintReadWrite) FString GlyphFamily = TEXT("auto");
    UPROPERTY(BlueprintReadWrite) TMap<FName, FKey> KeyboardOverrides;
    UPROPERTY(BlueprintReadWrite) TMap<FName, FKey> GamepadOverrides;
};

UCLASS()
class WROCLAWTHEGAME_API UWTGInputProfileSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category="Input") FWTGInputProfile Profile;
    UFUNCTION(BlueprintCallable, Category="Input") void ResetToDefaults();
    UFUNCTION(BlueprintCallable, Category="Input") bool RebindKeyboard(FName ActionId, FKey Key);
    UFUNCTION(BlueprintCallable, Category="Input") bool RebindGamepad(FName ActionId, FKey Key);
    UFUNCTION(BlueprintCallable, Category="Input") void SetMouseSensitivity(float Value);
    UFUNCTION(BlueprintCallable, Category="Input") void SetGamepadSensitivity(float Value);
    UFUNCTION(BlueprintCallable, Category="Input") void SetGamepadDeadzone(float Value);
    UFUNCTION(BlueprintCallable, Category="Input") void SetUIScale(float Value);
private:
    bool Rebind(TMap<FName,FKey>& Map, FName ActionId, FKey Key);
};
