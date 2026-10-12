#include "Systems/DistrictGameplaySubsystem.h"
#include "Geography/DistrictRegistry.h"
#include "Vehicles/VehiclePursuitSubsystem.h"
#include "Vehicles/DriveableVehicle.h"
#include "Character/SliceCharacter.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/GameplayComponents.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Mission/SliceMission.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"

bool UDistrictGameplaySubsystem::IsSearching() const
{
    return GetWorld()->GetSubsystem<UVehiclePursuitSubsystem>()->GetState()==EVehiclePursuitState::Search;
}
void UDistrictGameplaySubsystem::ReportCrime(float Severity,const FString& Reason)
{
    if (!Registry) return;
    Wanted.Report(Severity);
    GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>()->Notify(Reason);
}
void UDistrictGameplaySubsystem::RecoverPlayer()
{
    if (!Registry) return;
    ++RecoverySerial;
    auto* Pawn=UGameplayStatics::GetPlayerPawn(this,0);
    if (auto* Car=Cast<ADriveableVehicle>(Pawn))
    {
        Car->Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Car->Chassis->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
        Car->SetActorLocationAndRotation(Registry->VehicleSpawn,FRotator(0,Registry->VehicleYaw,0),false,nullptr,ETeleportType::TeleportPhysics);
        Car->Speed=0;
        Car->Health=100;
        Car->Exit();
        Pawn=UGameplayStatics::GetPlayerPawn(this,0);
    }
    if (auto* Player=Cast<ASliceCharacter>(Pawn))
    {
        FHitResult Floor;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(DistrictRecovery),false,Player);
        if (!GetWorld()->LineTraceSingleByChannel(Floor,Registry->PlayerStart+FVector(0,0,150),Registry->PlayerStart-FVector(0,0,500),ECC_Visibility,Params)) return;
        const FVector Safe=Floor.ImpactPoint+FVector(0,0,Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+4);
        if (!Player->TeleportTo(Safe,FRotator::ZeroRotator,false,false)) return;
        Player->HealthState->Value=100;
        Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        auto* M=GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>();
        M->bDead=false;
        UGameplayStatics::SetGamePaused(this,false);
        M->Notify(TEXT("Powrót na bezpieczny chodnik."));
    }
}
void UDistrictGameplaySubsystem::Tick(float Dt)
{
    if (!Registry)
        for (TActorIterator<AWTGDistrictRegistry> It(GetWorld());It;++It) { Registry=*It;break; }
    if (!Registry || UGameplayStatics::IsGamePaused(this)) return;
    if (!bLoggedReady && GetWorld()->GetTimeSeconds()>NextReadyDiagnostic)
    {
        NextReadyDiagnostic=GetWorld()->GetTimeSeconds()+10;
        if (auto* Player=Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this,0)))
        {
            UE_LOG(LogTemp,Display,TEXT("WTG_DISTRICT_WAITING position=%s movement=%d appearanceLoading=%d placeholder=%d animation=%s"),*Player->GetActorLocation().ToString(),int32(Player->GetCharacterMovement()->MovementMode),Player->Appearance->bLoading,Player->Appearance->bUsingPlaceholder,Player->Appearance->BodyMesh && Player->Appearance->BodyMesh->GetSingleNodeInstance()?*GetNameSafe(Player->Appearance->BodyMesh->GetSingleNodeInstance()->GetCurrentAsset()):TEXT("none"));
            FHitResult Floor;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(DistrictStartupDiagnosis),false,Player);
            const FVector Position=Player->GetActorLocation();
            const bool Found=GetWorld()->LineTraceSingleByChannel(Floor,Position+FVector(0,0,100),Position-FVector(0,0,450),ECC_Visibility,Params);
            bool Blocked=false;
            if (Found) Blocked=GetWorld()->OverlapBlockingTestByChannel(Floor.ImpactPoint+FVector(0,0,Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+4),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Player->GetCapsuleComponent()->GetScaledCapsuleRadius(),Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),Params);
            UE_LOG(LogTemp,Display,TEXT("WTG_DISTRICT_FLOOR_DIAG floor=%d actor=%s point=%s capsuleBlocked=%d gameModeTick=%d streamingComplete=%d"),Found,*GetNameSafe(Floor.GetActor()),*Floor.ImpactPoint.ToString(),Blocked,GetWorld()->GetAuthGameMode()->IsActorTickEnabled(),Player->StreamingSource->IsStreamingCompleted());
        }
    }
    if (!bLoggedReady)
        if (auto* Player=Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this,0)))
            if (Player->GetCharacterMovement()->MovementMode==MOVE_Walking && Player->Appearance->BodyMesh && Player->Appearance->BodyMesh->GetSingleNodeInstance() && Cast<UBlendSpace1D>(Player->Appearance->BodyMesh->GetSingleNodeInstance()->GetCurrentAsset()))
            {
                bLoggedReady=true;
                UE_LOG(LogTemp,Display,TEXT("WTG_DISTRICT_GAMEPLAY_READY: floor=walking locomotion=BlendSpace cameraCollision=%d registry=%s"),Player->Boom->bDoCollisionTest,*Registry->DistrictId);
            }
    auto* Pursuit=GetWorld()->GetSubsystem<UVehiclePursuitSubsystem>();
    Wanted.Tick(Dt,Pursuit->GetState()==EVehiclePursuitState::Chase);
    const double Now=GetWorld()->GetTimeSeconds();
    if (Wanted.Level()>0 && Pursuit->ActivePursuerCount()==0 && Now>=NextPoliceAttempt)
    {
        Pursuit->StartPursuit(Wanted.Level());
        NextPoliceAttempt=Now+8;
    }
    if (!Wanted.Level() && Pursuit->ActivePursuerCount()) Pursuit->StopPursuit();
    Accumulator+=Dt;
    if (Accumulator<.5f) return;
    Accumulator=0;
    if (auto* Pawn=UGameplayStatics::GetPlayerPawn(this,0))
    {
        const FVector P=Pawn->GetActorLocation();
        if (P.Z < -1000 || P.X<Registry->BoundsMin.X || P.X>Registry->BoundsMax.X || P.Y<Registry->BoundsMin.Y || P.Y>Registry->BoundsMax.Y)
            RecoverPlayer();
    }
}
