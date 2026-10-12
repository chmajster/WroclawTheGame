#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FrontEndMenuWidget.generated.h"
UCLASS()
class WROCLAWTHEGAME_API UFrontEndMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Refresh();
    virtual void ReleaseSlateResources(bool ReleaseChildren) override;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    TSharedPtr<class SMenuShell> Shell;
};
