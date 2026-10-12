#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
class UMenuFlowSubsystem;
class SMenuShell : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMenuShell) {} SLATE_ARGUMENT(UMenuFlowSubsystem*,Flow) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    void Refresh();
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E) override;
    void SelectPage(int32 Page);
private:
    TWeakObjectPtr<UMenuFlowSubsystem> Flow;
    TSharedPtr<class SOverlay> Layers;
    TSharedPtr<class SVerticalBox> Navigation,Content;
    TArray<TSharedPtr<SWidget>> FocusOrder;
    int32 Page=0,Focused=0;
    FString DialogTitle,DialogBody;
    TFunction<void()> Pending;
    TSharedRef<SWidget> Action(const FString& Label,TFunction<void()> Click,bool Enabled=true,const FString& Hint=FString());
    void Confirm(const FString& Title,const FString& Body,TFunction<void()> Action);
    void DrawDialog();
    void BuildMain();
    void BuildGame();
    void BuildSaves();
    void BuildSettings();
    void BuildControls();
    void BuildAbout();
    void FocusFirst();
};
