#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerMenuWidget.generated.h"

UCLASS()
class WROCLAWTHEGAME_API UPlayerMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;
    void Refresh();

protected:
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;

private:
    UPROPERTY() TObjectPtr<class UOverlay> RootOverlay;
    UPROPERTY() TObjectPtr<class UBorder> ConfirmationOverlay;
    UPROPERTY() TObjectPtr<class UBorder> AmbientGlowA;
    UPROPERTY() TObjectPtr<class UBorder> AmbientGlowB;
    UPROPERTY() TObjectPtr<class UTextBlock> ConfirmationCountdown;
    UPROPERTY() TObjectPtr<class UVerticalBox> ActionColumn;
    UPROPERTY() TObjectPtr<class UVerticalBox> CenterColumn;
    UPROPERTY() TObjectPtr<class UVerticalBox> RightColumn;
    UPROPERTY() TObjectPtr<class UTextBlock> PageTitle;
    UPROPERTY() TArray<TObjectPtr<class UButton>> TabButtons;
    UPROPERTY() TArray<TObjectPtr<class UButton>> ActionButtons;
    UPROPERTY() TObjectPtr<class AWTG_CharacterCreator> Studio;

    int32 ActiveTab = 0;
    int32 PendingConfirmation = 0;
    float ConfirmationSecondsRemaining = 0.0f;
    float PageAnimationTime = 0.22f;
    float AmbientAnimationTime = 0.0f;
    bool bRotatingPreview = false;
    bool bCollectActionButtons = false;

    class UTextBlock* MakeText(const FString& Value, int32 Size = 16, bool bBold = false,
                               FLinearColor Color = FLinearColor::White);
    class UButton* MakeButton(const FString& Label, bool bAccent = false);
    class UBorder* MakeCard(const FMargin& Padding = FMargin(18));
    class UBorder* MakeInfoRow(const FString& Title, const FString& Subtitle, bool bHighlighted = false);
    void BuildShell();
    void BuildGameTab();
    void BuildCharacterTab();
    void BuildInventoryTab();
    void BuildJournalTab();
    void BuildMapTab();
    void BuildStatsTab();
    void BuildSettingsTab();
    void AddPreview();
    void AddTextPage(const FString& Heading, const FString& Body);
    void AddPlayerStatus();
    void SelectTab(int32 Index);
    void UpdateTabStyle();
    void FocusPrimaryAction();
    void ShowConfirmation(int32 Action, const FString& Title, const FString& Body, const FString& ConfirmLabel);
    void ClearConfirmation();

    UFUNCTION() void PlayUIHover();
    UFUNCTION() void PlayUIClick();
    UFUNCTION() void ConfirmPendingAction();
    UFUNCTION() void CancelConfirmation();
    UFUNCTION() void TabGame();
    UFUNCTION() void TabCharacter();
    UFUNCTION() void TabInventory();
    UFUNCTION() void TabJournal();
    UFUNCTION() void TabMap();
    UFUNCTION() void TabStats();
    UFUNCTION() void TabSettings();

    UFUNCTION() void Resume();
    UFUNCTION() void NewGame();
    UFUNCTION() void LoadGame();
    UFUNCTION() void QuitGame();

    UFUNCTION() void PreviewFullBody();
    UFUNCTION() void PreviewUpperBody();
    UFUNCTION() void PreviewFace();
    UFUNCTION() void PreviewRotateLeft();
    UFUNCTION() void PreviewRotateRight();
    UFUNCTION() void PreviewLightingModern();
    UFUNCTION() void PreviewLightingDaylight();
    UFUNCTION() void PreviewLightingNight();
    UFUNCTION() void PreviewReset();

    UFUNCTION() void ToggleFPSCounter();
    UFUNCTION() void ToggleVSync();
    UFUNCTION() void ToggleDynamicResolution();
    UFUNCTION() void CycleWindowMode();
    UFUNCTION() void CycleResolution();
    UFUNCTION() void CycleQuality();
    UFUNCTION() void CycleResolutionScale();
    UFUNCTION() void CycleFPSLimit();
    UFUNCTION() void FPSUnlimited();
    UFUNCTION() void FPS30();
    UFUNCTION() void FPS60();
    UFUNCTION() void FPS90();
    UFUNCTION() void FPS120();
    UFUNCTION() void FPS144();
    UFUNCTION() void FPS165();
    UFUNCTION() void FPS240();
    void SetFPSLimit(int32 Limit);
};
