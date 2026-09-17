#include "Systems/PhoneSystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "Mission/SliceMission.h"
#include "Systems/WroclawMapSubsystem.h"
#include "Systems/CityGameplaySubsystem.h"
#include "Engine/World.h"
USliceMission *UPhoneSystem::Mission() const
{
    return GetLocalPlayer()->GetGameInstance()->GetSubsystem<USliceMission>();
}
void UPhoneSystem::Register(FName App, TFunction<FString()> Presenter)
{
    Presenters.Add(App, MoveTemp(Presenter));
}
void UPhoneSystem::Initialize(FSubsystemCollectionBase &C)
{
    Super::Initialize(C);
    for (const auto &App : Wroclaw::PhoneApps())
    {
        const FString Body = UTF8_TO_TCHAR(App.text.c_str());
        Register(FName(UTF8_TO_TCHAR(App.id.c_str())), [Body]() { return Body; });
    }
    Register(TEXT("messages"), [this]() {
        FString Text;
        for (const auto &M : Wroclaw::Messages())
            if (Mission()->WorldState.delivered.count(M.id))
                Text += FString(UTF8_TO_TCHAR(M.sender.c_str())) + TEXT(": ") +
                        UTF8_TO_TCHAR(M.text.c_str()) + TEXT("\n\n");
        return Text;
    });
    Register(TEXT("gallery"), [this]() {
        FString Text;
        for (const auto &Id : Mission()->WorldState.photos)
        {
            const auto *A = Wroclaw::Progress::Find(Id);
            if (A)
                Text += FString(UTF8_TO_TCHAR(A->label.c_str())) + TEXT("\n");
        }
        return Text;
    });
    Register(TEXT("map"), [this]() { return Mission()->WorldMapText() + GetWorld()->GetSubsystem<UWroclawMapSubsystem>()->PhoneMapText(); });
    Register(TEXT("notes"),
             [this]() { return Mission()->ObjectiveText() + TEXT("\n") + Mission()->HintText(); });
    Register(TEXT("investigation"), [this]() { return Mission()->InvestigationText(2); });
    Register(TEXT("objectives"), [this]() { auto *City = GetWorld()->GetSubsystem<UCityGameplaySubsystem>(); return City->IsActive() ? City->Journal() : Mission()->QuestLogText(); });
}
FString UPhoneSystem::Render(int32 Page) const
{
    FString Text;
    int I = 1;
    for (const auto &App : Wroclaw::PhoneApps())
        Text += FString::Printf(TEXT("%d %s | "), I++, UTF8_TO_TCHAR(App.title.c_str()));
    Text += TEXT("\n\n");
    if (Page < 0 || Page >= static_cast<int32>(Wroclaw::PhoneApps().size()))
        return Text;
    const auto *Presenter = Presenters.Find(FName(UTF8_TO_TCHAR(Wroclaw::PhoneApps()[Page].id.c_str())));
    return Presenter ? Text + (*Presenter)() : Text;
}
void UPhoneSystem::Open(int32 Page)
{
    if (Page < 0 || Page >= static_cast<int32>(Wroclaw::PhoneApps().size()))
        return;
    for (const auto &Id : Wroclaw::PhoneApps()[Page].on_open)
        Mission()->Act(UTF8_TO_TCHAR(Id.c_str()));
    Mission()->WorldState.NewMessages(Mission()->State);
}
