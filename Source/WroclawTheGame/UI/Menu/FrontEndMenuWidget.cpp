#include "UI/Menu/FrontEndMenuWidget.h"
#include "UI/Menu/MenuShell.h"
#include "UI/Menu/MenuFlowSubsystem.h"
#include "Engine/GameInstance.h"
UFrontEndMenuWidget::UFrontEndMenuWidget(const FObjectInitializer& I):Super(I){SetIsFocusable(true);}
TSharedRef<SWidget> UFrontEndMenuWidget::RebuildWidget()
{
    return SAssignNew(Shell,SMenuShell).Flow(GetGameInstance()->GetSubsystem<UMenuFlowSubsystem>());
}
void UFrontEndMenuWidget::Refresh(){if(Shell)Shell->Refresh();}
void UFrontEndMenuWidget::ReleaseSlateResources(bool Children)
{
    Super::ReleaseSlateResources(Children);Shell.Reset();
}
