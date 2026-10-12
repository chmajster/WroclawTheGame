#include "UI/Menu/MenuStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SCheckBox.h"
#include "UI/PerformanceSettings.h"

namespace WTGMenu
{
const FLinearColor Ink(.014f,.018f,.022f,1), Paper(.94f,.93f,.88f,1),
    Muted(.52f,.55f,.56f,1), Accent(.78f,.67f,.43f,1), Panel(.028f,.035f,.040f,.94f);
FSlateFontInfo Font(int32 Size,bool Bold){return FCoreStyle::GetDefaultFontStyle(Bold?"Bold":"Regular",Size);}
TSharedRef<SWidget> Text(const FString& Value,int32 Size,FLinearColor Color,bool Bold)
{
    return SNew(STextBlock).Text(FText::FromString(Value)).Font(Font(Size,Bold))
        .ColorAndOpacity(Color).AutoWrapText(true);
}
TSharedRef<SWidget> Card(TSharedRef<SWidget> Content,FMargin Padding)
{
    return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(Panel).Padding(Padding)[Content];
}
TSharedRef<SWidget> Slider(const FString& Label,TAttribute<float> Value,TFunction<void(float)> Changed,const FString& Tooltip)
{
    return SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight().Padding(0,10)[Text(Label,20)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,16)
        [SNew(SSlider).Value(Value).StepSize(.05f).SliderBarColor(Muted).SliderHandleColor(Accent)
            .ToolTipText(FText::FromString(Tooltip)).OnValueChanged_Lambda([Changed](float V){Changed(V);})];
}
TSharedRef<SWidget> Toggle(const FString& Label,TAttribute<bool> Value,TFunction<void(bool)> Changed)
{
    return SNew(SCheckBox).IsChecked_Lambda([Value]{return Value.Get()?ECheckBoxState::Checked:ECheckBoxState::Unchecked;})
        .OnCheckStateChanged_Lambda([Changed](ECheckBoxState V){Changed(V==ECheckBoxState::Checked);})
        .Padding(FMargin(8,14))[Text(Label,20)];
}
}
void SWTGMenuButton::Construct(const FArguments& Args)
{
    Selected=Args._Selected;
    ChildSlot[SAssignNew(Button,SButton).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
        .ContentPadding(FMargin(0)).OnClicked(Args._OnClicked).OnHovered(Args._OnHovered).ToolTipText(FText::FromString(Args._Hint))
        [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor_Lambda([this]{return FLinearColor(.82f,.79f,.70f,.025f+.19f*Highlight);})
            .Padding(FMargin(0))
            [SNew(SHorizontalBox)
                +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(4)
                    [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .BorderBackgroundColor_Lambda([this]{auto C=WTGMenu::Accent;C.A=Highlight;return C;})]]
                +SHorizontalBox::Slot().FillWidth(1).Padding(22,15)
                    [SNew(STextBlock).Text(FText::FromString(Args._Label)).Font(WTGMenu::Font(22,true))
                        .ColorAndOpacity_Lambda([this]{return FSlateColor(FMath::Lerp(WTGMenu::Paper,WTGMenu::Accent,Highlight));})]
            ]]];
}
void SWTGMenuButton::Tick(const FGeometry& G,double T,float Dt)
{
    SCompoundWidget::Tick(G,T,Dt);
    const bool Active=Selected.Get() || IsHovered() || HasKeyboardFocus() || HasFocusedDescendants();
    Highlight=UWTGPerformanceSettings::Get()->bReduceUIMotion?(Active?1.f:0.f):FMath::FInterpTo(Highlight,Active?1.f:0.f,Dt,14.f);
}
