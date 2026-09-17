#include "World/SurveillanceCamera.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/GameplayComponents.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Character/SliceCharacter.h"
#include "Systems/OpenWorldSubsystem.h"
#include "Systems/GameplayEventBus.h"
#include "UI/SliceController.h"
#include "Content/WorldCatalog.h"
ASurveillanceCamera::ASurveillanceCamera()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .25;
    Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
    RootComponent = Capture;
    Capture->bCaptureEveryFrame = false;
    Capture->bCaptureOnMovement = false;
    Capture->FOVAngle = 75;
    Power = CreateDefaultSubobject<UPowerConsumerComponent>(TEXT("Power"));
}
void ASurveillanceCamera::BeginPlay()
{
    Super::BeginPlay();
    for (const auto &C : Wroclaw::Cameras())
        if (DefinitionId == FName(UTF8_TO_TCHAR(C.id.c_str())))
        {
            Power->DisabledBy =
                FGameplayTag::RequestGameplayTag(FName(UTF8_TO_TCHAR(C.disabled_tag.c_str())), false);
            Power->RequiredPower =
                FGameplayTag::RequestGameplayTag(FName(UTF8_TO_TCHAR(C.power_tag.c_str())), false);
        }
}
UTextureRenderTarget2D *ASurveillanceCamera::Feed()
{
    if (!Power->Powered())
        return nullptr;
    if (!RenderTarget)
    {
        RenderTarget = NewObject<UTextureRenderTarget2D>(this);
        RenderTarget->InitAutoFormat(640, 360);
        Capture->TextureTarget = RenderTarget;
    }
    Capture->CaptureScene();
    return RenderTarget;
}
void ASurveillanceCamera::Tick(float Dt)
{
    Super::Tick(Dt);
    if (!Power->Powered())
        return;
    auto *P = Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!P)
        return;
    auto *PC = Cast<ASliceController>(P->GetController());
    if (PC && PC->bCCTV && PC->CCTVFeed == RenderTarget && RenderTarget)
        Capture->CaptureScene();
    for (const auto &C : Wroclaw::Cameras())
        if (DefinitionId == FName(UTF8_TO_TCHAR(C.id.c_str())))
        {
            const FVector Delta = P->GetActorLocation() - GetActorLocation();
            if (Delta.Size() > C.radius ||
                FVector::DotProduct(Delta.GetSafeNormal(), GetActorForwardVector()) < .8 ||
                GetWorld()->GetSubsystem<UOpenWorldSubsystem>()->VisibilityTo(this, P) < .12)
                return;
            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(CCTV), false, this);
            GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), P->GetActorLocation(),
                                                 ECC_Visibility, Params);
            if (Hit.GetActor() != P)
                return;
            const double Now = GetWorld()->GetTimeSeconds();
            if (Now - LastAlarm > 12)
            {
                LastAlarm = Now;
                GetWorld()->GetSubsystem<UGameplayEventBus>()->Emit(
                    TEXT("Event.Alarm"), DefinitionId, C.alarm_heat, P->GetActorLocation(), this);
            }
            return;
        }
}
