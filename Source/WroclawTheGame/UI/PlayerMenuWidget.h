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
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;

private:
    UPROPERTY() TObjectPtr<class UVerticalBox> ActionColumn;
    UPROPERTY() TObjectPtr<class UVerticalBox> CenterColumn;
    UPROPERTY() TObjectPtr<class UVerticalBox> RightColumn;
    UPROPERTY() TObjectPtr<class UTextBlock> PageTitle;
    UPROPERTY() TArray<TObjectPtr<class UButton>> TabButtons;
    UPROPERTY() TObjectPtr<class AWTG_CharacterCreator> Studio;

    int32 ActiveTab = 0;
    bool bRotatingPreview = false;

    class UTextBlock* MakeText(const FString& Value, int32 Size = 16, bool bBold = false,
                               FLinearColor Color = FLinearColor::White);
    class UButton* MakeButton(const FString& Label, bool bAccent = false);
    class UBorder* MakeCard(const FMargin& Padding = FMargin(18));
    void BuildShell();
    void BuildGameTab();
    void BuildCharacterTab();
    void BuildInventoryTab();
    void BuildJournalTab();
    void BuildMapTab();
    void BuildStatsTab();
    void AddPreview();
    void AddTextPage(const FString& Heading, const FString& Body);
    void AddPlayerStatus();
    void SelectTab(int32 Index);
    void UpdateTabStyle();

    UFUNCTION() void TabGame();
    UFUNCTION() void TabCharacter();
    UFUNCTION() void TabInventory();
    UFUNCTION() void TabJournal();
    UFUNCTION() void TabMap();
    UFUNCTION() void TabStats();

    UFUNCTION() void Resume();
    UFUNCTION() void NewGame();
    UFUNCTION() void LoadGame();
    UFUNCTION() void QuitGame();

    UFUNCTION() void PreviewFullBody();
    UFUNCTION() void PreviewUpperBody();
    UFUNCTION() void PreviewFace();
    UFUNCTION() void PreviewRotateLeft();
    UFUNCTION() void PreviewRotateRight();
};
