#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SButton.h"

// Owned original UI; all brushes/fonts come from the engine's runtime CoreStyle.
namespace WTGMenu
{
extern const FLinearColor Ink, Paper, Muted, Accent, Panel;
FSlateFontInfo Font(int32 Size, bool Bold=false);
TSharedRef<SWidget> Text(const FString& Value, int32 Size=22, FLinearColor Color=Paper, bool Bold=false);
TSharedRef<SWidget> Card(TSharedRef<SWidget> Content, FMargin Padding=FMargin(28));
TSharedRef<SWidget> Slider(const FString& Label, TAttribute<float> Value,
                         TFunction<void(float)> Changed, const FString& Tooltip=FString());
TSharedRef<SWidget> Toggle(const FString& Label, TAttribute<bool> Value, TFunction<void(bool)> Changed);
}

class SWTGMenuButton : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWTGMenuButton) : _Label(), _Selected(false) {}
        SLATE_ARGUMENT(FString, Label)
        SLATE_ARGUMENT(FString, Hint)
        SLATE_ATTRIBUTE(bool, Selected)
        SLATE_EVENT(FOnClicked, OnClicked)
        SLATE_EVENT(FSimpleDelegate, OnHovered)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual void Tick(const FGeometry& Geometry, double Time, float Dt) override;
    TSharedPtr<SButton> FocusTarget() const { return Button; }
private:
    TSharedPtr<SButton> Button;
    TAttribute<bool> Selected;
    float Highlight=0;
};
