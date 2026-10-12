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
    virtual FReply OnPreviewMouseButtonDown(const FGeometry& G,const FPointerEvent& E) override;
    void SelectPage(int32 Page);
private:
    TWeakObjectPtr<UMenuFlowSubsystem> Flow;
    TSharedPtr<class SOverlay> Layers;
    TSharedPtr<class SVerticalBox> Navigation,Content;
    TArray<TSharedPtr<SWidget>> FocusOrder;
    int32 Page=0,Focused=0;
    int32 SettingsCategory=0;
    FName CapturingBinding;
    bool CapturingGamepad=false;
    double VideoConfirmDeadline=0;
    bool VideoPending=false;
    FIntPoint PendingResolution;
    int32 PendingWindowMode=1;
    virtual void Tick(const FGeometry& G,double Time,float Dt) override;
    void BeginVideoChange();
    void RevertVideoChange();
    TSharedRef<SWidget> Choice(const FString& Label,const TArray<FString>& Values,int32 Selected,TFunction<void(int32)> Changed);
    TSharedRef<SWidget> ValueSlider(const FString& Label,TAttribute<float> Value,TFunction<void(float)> Changed);
    TSharedRef<SWidget> ValueToggle(const FString& Label,TAttribute<bool> Value,TFunction<void(bool)> Changed);
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
    void BuildMap();
    void BuildCharacter();
    void BuildStats();
    TSharedPtr<SWidget> MapView;
    void FocusFirst();
};
