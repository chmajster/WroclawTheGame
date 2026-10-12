#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
class UMenuFlowSubsystem;
// Projection/view is independent of future GPS/pathfinding. World-space waypoint
// belongs to MenuFlow; a route provider can later consume the same coordinates.
class SMenuMap : public SCompoundWidget
{
public:
    float ViewZoom() const { return Zoom; }
    FVector2D ViewCenter() const { return Center; }
    SLATE_BEGIN_ARGS(SMenuMap) {} SLATE_ARGUMENT(UMenuFlowSubsystem*,Flow) SLATE_END_ARGS()
    void Construct(const FArguments& A);
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1440,600); }
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool) const override;
    virtual FReply OnMouseButtonDown(const FGeometry&,const FPointerEvent&) override;
    virtual FReply OnMouseButtonUp(const FGeometry&,const FPointerEvent&) override;
    virtual FReply OnMouseMove(const FGeometry&,const FPointerEvent&) override;
    virtual FReply OnMouseWheel(const FGeometry&,const FPointerEvent&) override;
    virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent&) override;
    FVector2D Project(const FVector& Position,const FVector2D& Size) const;
private:
    TWeakObjectPtr<UMenuFlowSubsystem> Flow;
    FVector2D Center=FVector2D::ZeroVector,LastMouse,PressMouse;
    float Width=60000,Zoom=1;
    bool Dragging=false;
};
