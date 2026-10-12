#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Character/CharacterAppearanceDefinition.h"
#include "CharacterCreatorWidget.generated.h"
UCLASS(Blueprintable)
class WROCLAWTHEGAME_API UCharacterCreatorWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<class AWTG_CharacterCreator> Studio;
    UPROPERTY() TObjectPtr<class UCharacterCreatorSubsystem> Creator;
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;
    virtual void ReleaseSlateResources(bool Children) override;
    void Action(FName ID);
    void SetNumber(FName ID,float Value);
    void SetChoice(FName ID,FName Value);
    void SetText(FName ID,const FString& Value);
    void SelectCategory(int32 Index);
    int32 SelectedCategory() const { return CategoryIndex; }
    void Refresh();
    FSlateBrush PreviewBrush;
    FString Status;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    TSharedPtr<class SCharacterCreatorPanel> Panel;
    int32 CategoryIndex=0;
};
