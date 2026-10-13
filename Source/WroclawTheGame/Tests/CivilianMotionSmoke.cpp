#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/MorphTarget.h"
#include "Engine/SkeletalMesh.h"
#include "Character/SliceCharacter.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Character/CivilianAnimInstance.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HighResScreenshot.h"
#include "Serialization/JsonSerializer.h"

struct FCivilianMotionProbe
{
    float Elapsed=0,Started=0,SampleAt=0;
    int Stage=INDEX_NONE;
    FVector Origin=FVector::ZeroVector,Forward=FVector::ForwardVector;
    TArray<TSharedPtr<FJsonValue>> Samples;
    TSet<FName> States;
    bool OK=true,Captured=false;
    FCharacterAppearanceDefinition OriginalAppearance;
};

static FAutoConsoleCommandWithWorld GCivilianMotionSmoke(TEXT("WTG.CivilianMotionSmoke"),
 TEXT("Real start-map input, transitions, crouch/jump, foot and camera evidence; no saved changes."),
 FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World){
    if(!World || !World->IsGameWorld())return;
    auto S=MakeShared<FCivilianMotionProbe>();
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Realism/motion");
    IFileManager::Get().MakeDirectory(*Folder,true);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak=TWeakObjectPtr<UWorld>(World),S,Folder](float Dt){
        auto* W=Weak.Get();if(!W)return false;S->Elapsed+=Dt;
        auto* PC=UGameplayStatics::GetPlayerController(W,0);
        auto* P=Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(W,0));
        if(S->Elapsed<50 || !PC || !P || !P->Appearance || P->Appearance->bLoading || W->IsPaused())return true;
        auto* Mesh=P->Appearance->BodyMesh.Get();
        auto* Anim=Cast<UCivilianAnimInstance>(Mesh->GetAnimInstance());
        if(!Anim){UE_LOG(LogTemp,Error,TEXT("WTG_MOTION_FAIL missing_native_animation"));FPlatformMisc::RequestExit(false);return false;}
        const TCHAR* Names[]={TEXT("idle"),TEXT("walk"),TEXT("run"),TEXT("sprint"),TEXT("turn"),TEXT("stop"),TEXT("crouch"),TEXT("crouch_walk"),TEXT("jump"),TEXT("land"),TEXT("heavy"),TEXT("slim"),TEXT("restored")};
        const float Duration[]={4,4,4,4,4,3,3,3,2,4,3,3,3};
        auto Advance=[&](){
            ++S->Stage;S->Started=S->Elapsed;S->Captured=false;
            if(S->Stage==0){
                PC->SetIgnoreMoveInput(true);PC->SetIgnoreLookInput(true);PC->SetViewTarget(P);
                S->Origin=P->GetActorLocation();
                S->OriginalAppearance=P->Appearance->Appearance;
                // Select an unobstructed street direction, preserving actual movement/collision.
                float Best=0;
                for(int I=0;I<24;++I){const float Angle=I*PI/12;const FVector Dir(FMath::Cos(Angle),FMath::Sin(Angle),0);
                    FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(MotionRoute),false,P);
                    W->SweepSingleByChannel(Hit,S->Origin,S->Origin+Dir*5000,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(36),Params);
                    const float Reach=Hit.bBlockingHit?Hit.Distance:5000;
                    if(Reach>Best){Best=Reach;S->Forward=Dir;}
                }
                PC->SetControlRotation(FRotator(-12,S->Forward.Rotation().Yaw,0));
                UE_LOG(LogTemp,Display,TEXT("WTG_MOTION_ROUTE clearance_cm=%.1f heading=%s"),Best,*S->Forward.ToString());
            }
            if(S->Stage==3)P->SetSprint(true);
            else P->SetSprint(false);
            if(S->Stage==6)P->Crouch();
            if(S->Stage==8)P->UnCrouch();
            if(S->Stage>=10){auto Appearance=S->OriginalAppearance;
                Appearance.BodyBuild=S->Stage==10?TEXT("Heavy"):(S->Stage==11?TEXT("Slim"):S->OriginalAppearance.BodyBuild);
                P->Appearance->ApplyAppearance(Appearance);
            }
        };
        if(S->Stage==INDEX_NONE)Advance();
        if(S->Elapsed-S->Started>Duration[S->Stage]){
            if(S->Stage==12){
                for(FName State:{FName(TEXT("Idle")),FName(TEXT("Walk")),FName(TEXT("Run")),FName(TEXT("Sprint")),FName(TEXT("Crouch")),FName(TEXT("Jump"))})S->OK&=S->States.Contains(State);
                S->OK&=FVector::Dist2D(P->GetActorLocation(),S->Origin)>100;
                auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("status"),S->OK?TEXT("PASS_REQUIRES_VISUAL_REVIEW"):TEXT("FAIL"));
                Report->SetStringField(TEXT("map"),W->GetOutermost()->GetName());Report->SetStringField(TEXT("anim_class"),Anim->GetClass()->GetPathName());
                Report->SetArrayField(TEXT("samples"),S->Samples);Report->SetNumberField(TEXT("displacement_cm"),FVector::Dist2D(P->GetActorLocation(),S->Origin));
                FString Json;auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(Report,Writer);
                FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("motion.json")));
                UE_LOG(LogTemp,Display,TEXT("WTG_CIVILIAN_MOTION_DONE result=%s samples=%d"),S->OK?TEXT("PASS"):TEXT("FAIL"),S->Samples.Num());
                FPlatformMisc::RequestExit(false);return false;
            }
            Advance();
        }
        const float Age=S->Elapsed-S->Started;
        if(S->Stage==8 && Age>.15f && Age<.3f)P->Jump();
        if(S->Stage>=1 && S->Stage<=3)P->AddMovementInput(S->Forward,S->Stage==1?.6f:1.f,true);
        if(S->Stage==4 || S->Stage==7)P->AddMovementInput(-S->Forward,1,true);
        if(S->Stage==4)PC->SetControlRotation(FRotator(-12,(-S->Forward).Rotation().Yaw,0));
        if(!S->Captured && Age>1.5f){
            FScreenshotRequest::RequestScreenshot(Folder/(FString(Names[S->Stage])+TEXT(".png")),true,false);S->Captured=true;
        }
        if(S->Elapsed>=S->SampleAt){
            S->SampleAt=S->Elapsed+.1f;S->States.Add(Anim->MovementState);
            auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("stage"),Names[S->Stage]);Item->SetStringField(TEXT("state"),Anim->MovementState.ToString());
            Item->SetNumberField(TEXT("time"),S->Elapsed);Item->SetNumberField(TEXT("speed_cm_s"),Anim->GroundSpeed);
            Item->SetStringField(TEXT("position"),P->GetActorLocation().ToString());Item->SetBoolField(TEXT("grounded"),Anim->bGrounded);
            Item->SetNumberField(TEXT("left_offset_cm"),Anim->FootOffsets.X);Item->SetNumberField(TEXT("right_offset_cm"),Anim->FootOffsets.Y);
            Item->SetBoolField(TEXT("left_contact"),Anim->bLeftContact);Item->SetBoolField(TEXT("right_contact"),Anim->bRightContact);
            Item->SetStringField(TEXT("left_foot"),Mesh->GetSocketLocation(TEXT("foot_l")).ToString());
            Item->SetStringField(TEXT("right_foot"),Mesh->GetSocketLocation(TEXT("foot_r")).ToString());
            Item->SetStringField(TEXT("left_ground"),Anim->LeftGround.ToString());Item->SetStringField(TEXT("right_ground"),Anim->RightGround.ToString());
            const float CameraHead=FVector::Dist(P->Camera->GetComponentLocation(),Mesh->GetSocketLocation(TEXT("head")));
            Item->SetNumberField(TEXT("camera_head_cm"),CameraHead);Item->SetNumberField(TEXT("lod"),Mesh->GetPredictedLODLevel());
            S->OK&=!P->Appearance->bUsingPlaceholder && FMath::IsFinite(Anim->FootOffsets.X) && FMath::IsFinite(Anim->FootOffsets.Y)
                && FMath::Abs(Anim->FootOffsets.X)<=22.1 && FMath::Abs(Anim->FootOffsets.Y)<=22.1 && CameraHead>45 && P->Boom->bDoCollisionTest;
            const float Floor=P->GetActorLocation().Z-P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
            if(Anim->bGrounded)S->OK&=Mesh->GetSocketLocation(TEXT("foot_l")).Z>Floor-10 && Mesh->GetSocketLocation(TEXT("foot_r")).Z>Floor-10;
            S->OK&=Mesh->Bounds.BoxExtent.Z>30 && Mesh->Bounds.BoxExtent.Z<150;
            if(S->Stage==10 || S->Stage==11){
                TArray<USkeletalMeshComponent*> Parts;P->GetComponents(Parts);int32 Matched=0;
                const float Expected=S->Stage==10?.25f:-.2f;
                for(auto* Part:Parts)if(Part->IsVisible() && Part->GetSkeletalMeshAsset())
                    for(const auto& Target:Part->GetSkeletalMeshAsset()->GetMorphTargets())if(Target && Target->GetName().EndsWith(TEXT("Body_Weight"))){
                        const float Value=Part->GetMorphTarget(Target->GetFName());
                        Item->SetNumberField(Part->GetSkeletalMeshAsset()->GetName()+TEXT("_weight"),Value);
                        if(FMath::IsNearlyEqual(Value,Expected,.001f))++Matched;
                    }
                if(Age>1)S->OK&=Matched==7;
            }
            S->Samples.Add(MakeShared<FJsonValueObject>(Item));
        }
        return true;
    }));
 }));
#endif
