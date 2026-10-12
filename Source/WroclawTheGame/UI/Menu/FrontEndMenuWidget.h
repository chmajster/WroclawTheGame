#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FrontEndMenuWidget.generated.h"
UCLASS()
class WROCLAWTHEGAME_API UFrontEndMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFrontEndMenuWidget(const FObjectInitializer& Initializer);
    void Refresh();
    TSharedPtr<class SMenuShell> SlateMenu() const { return Shell; }
    virtual void ReleaseSlateResources(bool ReleaseChildren) override;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    TSharedPtr<class SMenuShell> Shell;
};
