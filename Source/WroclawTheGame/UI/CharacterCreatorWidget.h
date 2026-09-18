#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Character/CharacterAppearanceDefinition.h"
#include "CharacterCreatorWidget.generated.h"

UCLASS()
class WROCLAWTHEGAME_API UAppearanceControl : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<class UCharacterCreatorWidget> OwnerWidget;
    FName Field; TArray<FName> Values;
    void Button(const FString& Label);
    void Number(const FString& Label,float Value,float Min,float Max,float Step);
    void Choice(const FString& Label,const TArray<FName>& Options,FName Selected);
    void TextEntry(const FString& Label,const FString& Value);
    UFUNCTION() void Click();
    UFUNCTION() void Changed(float Value);
    UFUNCTION() void Selected(FString Value,ESelectInfo::Type Type);
    UFUNCTION() void TextChanged(const FText& Value,ETextCommit::Type Type);
};

UCLASS(Blueprintable)
class WROCLAWTHEGAME_API UCharacterCreatorWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<class AWTG_CharacterCreator> Studio;
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnMouseMove(const FGeometry& G,const FPointerEvent& E) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& G,const FPointerEvent& E) override;
    void Action(FName ID);
    void SetNumber(FName ID,float Value);
    void SetChoice(FName ID,FName Value);
    void SetText(FName ID,const FString& Value);
    void Refresh();
private:
    UPROPERTY() TObjectPtr<class UCharacterCreatorSubsystem> Creator;
    UPROPERTY() TObjectPtr<class UVerticalBox> Controls;
    UPROPERTY() TObjectPtr<class UVerticalBox> Commands;
    UPROPERTY() TObjectPtr<class UTextBlock> Heading;
    UPROPERTY() TObjectPtr<class UTextBlock> Status;
    bool Advanced=false, Debug=false, Dragging=false;
    EAppearanceCategory Category=EAppearanceCategory::Character;
    UAppearanceControl* Row(class UVerticalBox* Box,FName ID);
};
