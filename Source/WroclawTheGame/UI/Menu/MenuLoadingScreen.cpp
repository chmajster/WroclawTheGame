#include "UI/Menu/MenuLoadingScreen.h"
#include "UI/Menu/MenuStyle.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBox.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

// Original vector illustration: river, tenement silhouettes and a bridge.
// No world/UObject reads: MoviePlayer can paint this while the game thread loads.
class SMenuRiverIllustration : public SLeafWidget
{
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1920,1080); }
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle&,bool) const override
    {
        const auto Size=G.GetLocalSize();const FVector2D Scale=Size/FVector2D(1920,1080);
        auto Rect=[&](FVector2D P,FVector2D S,FLinearColor C){FSlateDrawElement::MakeBox(Out,L,G.ToPaintGeometry(S*Scale,FSlateLayoutTransform(P*Scale)),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,C);};
        auto Line=[&](TArray<FVector2D> P,FLinearColor C,float Width=2){for(auto& V:P)V*=Scale;FSlateDrawElement::MakeLines(Out,L+1,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,Width);};
        Rect(FVector2D::ZeroVector,FVector2D(1920,1080),WTGMenu::Ink);
        for(int32 I=0;I<22;++I){const float X=I*100,Height=100+(I*37%170);
            Rect(FVector2D(X,640-Height),FVector2D(88,Height),FLinearColor(.045,.058,.069,1));
            for(int32 Y=0;Y<4;++Y)for(int32 J=0;J<3;++J)Rect(FVector2D(X+14+J*23,650-Height+Y*35),FVector2D(8,14),FLinearColor(.09,.105,.11,1));}
        Line({{0,735},{360,725},{720,790},{1100,845},{1460,825},{1920,760}},FLinearColor(.10,.22,.27,1),70);
        Line({{0,780},{360,770},{720,835},{1100,890},{1460,870},{1920,805}},FLinearColor(.14,.30,.34,1),2);
        Line({{1050,685},{1790,685}},WTGMenu::Accent,3);
        Line({{1150,685},{1150,605},{1210,570},{1630,570},{1700,605},{1700,685}},WTGMenu::Muted,2);
        for(int32 I=0;I<16;++I)Line({{1160.+I*34,575},{1160.+I*34,680}},FLinearColor(.25,.28,.28,1),1);
        const float T=FMath::Fmod(FPlatformTime::Seconds()*.35,1.);
        Rect(FVector2D(100,940),FVector2D(520,2),WTGMenu::Muted);
        Rect(FVector2D(100+T*440,938),FVector2D(80,6),WTGMenu::Accent);
        return L+1;
    }
};
TSharedRef<SWidget> WTGMenu::LoadingScreen()
{
    return SNew(SOverlay)+SOverlay::Slot()[SNew(SMenuRiverIllustration)]
        +SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)
            [SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(100,120,100,20)[Text(TEXT("W R O C Ł A W"),68,Paper,true)]
                +SVerticalBox::Slot().AutoHeight().Padding(100,0)[Text(TEXT("T H E  G A M E"),22,Accent,true)]
                +SVerticalBox::Slot().FillHeight(1)
                +SVerticalBox::Slot().AutoHeight().Padding(100,0,100,80)[Text(TEXT("Przygotowywanie świata…"),28,Paper,true)]
                +SVerticalBox::Slot().AutoHeight().Padding(100,0,100,64)[Text(TEXT("Pauza daje Ci czas na zaplanowanie kolejnego kroku.\nW zakładce STEROWANIE możesz zmienić przypisania."),20,Muted)]
            ]]];
}
