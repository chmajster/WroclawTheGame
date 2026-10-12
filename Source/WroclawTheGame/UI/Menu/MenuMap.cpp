#include "UI/Menu/MenuMap.h"
#include "UI/Menu/MenuFlowSubsystem.h"
#include "UI/Menu/MenuStyle.h"
#include "UI/SliceController.h"
#include "Geography/DistrictRegistry.h"
#include "Systems/DistrictGameplaySubsystem.h"
#include "Vehicles/DriveableVehicle.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Engine/World.h"

void SMenuMap::Construct(const FArguments& A)
{
    Flow=A._Flow;SetClipping(EWidgetClipping::ClipToBounds);
    if(Flow.IsValid() && Flow->MenuWorld())if(auto* R=Flow->MenuWorld()->GetSubsystem<UDistrictGameplaySubsystem>()->GetRegistry()){
        Center=FVector2D((R->BoundsMin+R->BoundsMax)*.5);Width=FMath::Max(1.,R->BoundsMax.X-R->BoundsMin.X)*1.08;
    }
}
FVector2D SMenuMap::Project(const FVector& P,const FVector2D& Size) const
{
    return Size*.5+FVector2D(P.X-Center.X,Center.Y-P.Y)*(Size.X/Width*Zoom);
}
int32 SMenuMap::OnPaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Clip,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const
{
    FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,WTGMenu::Ink);
    const FVector2D Size=G.GetLocalSize();auto Line=[&](const TArray<FVector2D>& Points,FLinearColor C,float Thick){
        FSlateDrawElement::MakeLines(Out,Layer+1,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,C,true,Thick);};
    for(int32 I=0;I<20;++I){const float X=Size.X*I/20,Y=Size.Y*I/20;
        Line({FVector2D(X,0),FVector2D(X,Size.Y)},FLinearColor(.08,.1,.12,1),1);
        Line({FVector2D(0,Y),FVector2D(Size.X,Y)},FLinearColor(.08,.1,.12,1),1);}
    if(!Flow.IsValid() || !Flow->MenuWorld())return Layer+1;
    auto* R=Flow->MenuWorld()->GetSubsystem<UDistrictGameplaySubsystem>()->GetRegistry();if(!R)return Layer+1;
    for(const auto& Street:R->Streets){TArray<FVector2D> Points;for(const auto& P:Street.Points)Points.Add(Project(P,Size));
        if(Points.Num()>1){Line(Points,Street.bWalkway?FLinearColor(.18,.21,.23,1):FLinearColor(.46,.48,.46,1),Street.bWalkway?2.f:7.f);}}
    auto Mark=[&](const FVector& World,const FString& Name,FLinearColor Color){
        const FVector2D P=Project(World,Size);
        FSlateDrawElement::MakeBox(Out,Layer+3,G.ToPaintGeometry(FVector2D(12),FSlateLayoutTransform(P-FVector2D(6))),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,Color);
        FSlateDrawElement::MakeText(Out,Layer+3,G.ToPaintGeometry(FVector2D(240,25),FSlateLayoutTransform(P+FVector2D(12,-10))),Name,WTGMenu::Font(16,true),ESlateDrawEffect::None,Color);
    };
    Mark(R->MeetingPoint,TEXT("MAJA / PRACOWNIA"),WTGMenu::Accent);
    Mark(R->DeliveryPoint,TEXT("OLEK / BULWAR"),WTGMenu::Accent);
    if(R->DeliveryVehicle)Mark(R->DeliveryVehicle->GetActorLocation(),TEXT("SAMOCHÓD"),FLinearColor(.42,.63,.70,1));
    if(Flow->Controller() && Flow->Controller()->GetPawn())Mark(Flow->Controller()->GetPawn()->GetActorLocation(),TEXT("TY"),WTGMenu::Paper);
    if(Flow->MapWaypoint.IsSet())Mark(Flow->MapWaypoint.GetValue(),TEXT("TWÓJ ZNACZNIK"),FLinearColor(.67,.46,.68,1));
    FSlateDrawElement::MakeText(Out,Layer+4,G.ToPaintGeometry(FVector2D(140,28),FSlateLayoutTransform(FVector2D(24,20))),TEXT("N ↑"),WTGMenu::Font(20,true),ESlateDrawEffect::None,WTGMenu::Paper);
    return Layer+4;
}
FReply SMenuMap::OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{
    if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();
    LastMouse=PressMouse=G.AbsoluteToLocal(E.GetScreenSpacePosition());Dragging=true;
    return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this));
}
FReply SMenuMap::OnMouseMove(const FGeometry& G,const FPointerEvent& E)
{
    if(!Dragging)return FReply::Unhandled();const FVector2D Mouse=G.AbsoluteToLocal(E.GetScreenSpacePosition()),Delta=Mouse-LastMouse;
    Center+=FVector2D(-Delta.X,Delta.Y)*(Width/G.GetLocalSize().X/Zoom);LastMouse=Mouse;Invalidate(EInvalidateWidgetReason::Paint);return FReply::Handled();
}
FReply SMenuMap::OnMouseButtonUp(const FGeometry& G,const FPointerEvent& E)
{
    if(!Dragging || E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();Dragging=false;
    const FVector2D P=G.AbsoluteToLocal(E.GetScreenSpacePosition());
    if((P-PressMouse).Size()<4 && Flow.IsValid()){
        const FVector2D D=(P-G.GetLocalSize()*.5)*(Width/G.GetLocalSize().X/Zoom);Flow->MapWaypoint=FVector(Center.X+D.X,Center.Y-D.Y,0);
    }
    return FReply::Handled().ReleaseMouseCapture();
}
FReply SMenuMap::OnMouseWheel(const FGeometry&,const FPointerEvent& E)
{
    Zoom=FMath::Clamp(Zoom*FMath::Pow(1.2f,E.GetWheelDelta()),.65f,8.f);Invalidate(EInvalidateWidgetReason::Paint);return FReply::Handled();
}
FReply SMenuMap::OnKeyDown(const FGeometry&,const FKeyEvent& E)
{
    const FKey K=E.GetKey();const float Step=Width*.04f/Zoom;
    if(K==EKeys::Up || K==EKeys::Gamepad_DPad_Up)Center.Y+=Step;
    else if(K==EKeys::Down || K==EKeys::Gamepad_DPad_Down)Center.Y-=Step;
    else if(K==EKeys::Left || K==EKeys::Gamepad_DPad_Left)Center.X-=Step;
    else if(K==EKeys::Right || K==EKeys::Gamepad_DPad_Right)Center.X+=Step;
    else if(K==EKeys::Add || K==EKeys::Equals || K==EKeys::Gamepad_RightShoulder)Zoom=FMath::Min(8.f,Zoom*1.2f);
    else if(K==EKeys::Subtract || K==EKeys::Hyphen || K==EKeys::Gamepad_LeftShoulder)Zoom=FMath::Max(.65f,Zoom/1.2f);
    else return FReply::Unhandled();Invalidate(EInvalidateWidgetReason::Paint);return FReply::Handled();
}
