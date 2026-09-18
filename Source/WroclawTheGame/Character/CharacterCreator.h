#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CharacterCreator.generated.h"

UCLASS(Blueprintable)
class WROCLAWTHEGAME_API AWTG_CharacterCreator : public AActor
{
    GENERATED_BODY()
public:
    AWTG_CharacterCreator();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<class UCharacterAppearanceComponent> Appearance;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<class USceneCaptureComponent2D> Capture;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class USceneComponent> ModelRoot;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UPointLightComponent> KeyLight;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UPointLightComponent> FillLight;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UPointLightComponent> RimLight;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<class UTextureRenderTarget2D> RenderTarget;
    UFUNCTION(BlueprintCallable) void SetView(FName View);
    UFUNCTION(BlueprintCallable) void SetLighting(FName Profile);
    UFUNCTION(BlueprintCallable) void Rotate(float Degrees);
    UFUNCTION(BlueprintCallable) void Zoom(float Amount);
    UFUNCTION() void RefreshAppearance();
private:
    float Distance=290, TargetDistance=290, TargetHeight=92, CameraHeight=92, Yaw=0;
};
