#include "Character/CharacterAppearanceComponent.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Character/SliceCharacter.h"
#include "Mission/SliceMission.h"
#include "Systems/CityGameplaySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimInstance.h"

UCharacterAppearanceComponent::UCharacterAppearanceComponent()
{ PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.bTickEvenWhenPaused=true; }
void UCharacterAppearanceComponent::BeginPlay()
{
    Super::BeginPlay();
    auto* S=GetWorld()->GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>(); Catalog=S->Catalog;
    VisualRoot=NewObject<USceneComponent>(GetOwner()); VisualRoot->SetupAttachment(GetOwner()->GetRootComponent()); VisualRoot->RegisterComponent();
    BodyMesh=NewObject<USkeletalMeshComponent>(GetOwner()); BodyMesh->SetupAttachment(VisualRoot); BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); BodyMesh->RegisterComponent();
    FaceMesh=NewObject<USkeletalMeshComponent>(GetOwner()); FaceMesh->SetupAttachment(BodyMesh); FaceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); FaceMesh->RegisterComponent();
    ApplyAppearance(S->Committed.PlayerAppearanceData);
}
void UCharacterAppearanceComponent::EndPlay(const EEndPlayReason::Type Reason)
{ ++RequestSerial; if (LoadHandle) LoadHandle->CancelHandle(); Super::EndPlay(Reason); }
void UCharacterAppearanceComponent::ReloadAppearance() { LoadedPaths.Empty(); ApplyAppearance(Appearance); }
void UCharacterAppearanceComponent::ApplyAppearance(const FCharacterAppearanceDefinition& Data)
{
    if (!Catalog || !VisualRoot) return;
    Appearance=UCharacterCreatorValidator::Normalize(*Catalog,Data);
    TArray<FSoftObjectPath> Paths;
    auto Add=[&](const auto& Ref){ if (!Ref.IsNull()) Paths.AddUnique(Ref.ToSoftObjectPath()); };
    if (const auto* B=Catalog->Body(Appearance.BodyPreset))
    { Add(B->Mesh); Add(B->FaceMesh); Add(B->AnimationClass); Add(B->FaceAnimationClass); Add(B->SkinMaterial); Add(B->CreatorIdle); Add(B->Walk); Add(B->Jog); Add(B->Crouch); }
    auto AddPart=[&](const FAppearancePartDefinition* P){if (P) { Add(P->Mesh); Add(P->StaticMesh); Add(P->Material); }};
    AddPart(Catalog->Part(Catalog->HairStyleDefinitions,Appearance.HairStyle));
    AddPart(Catalog->Part(Catalog->BeardStyleDefinitions,Appearance.BeardStyle));
    AddPart(Catalog->Part(Catalog->EyebrowStyleDefinitions,Appearance.EyebrowStyle));
    for (const auto& P:Appearance.Clothing) AddPart(Catalog->Part(Catalog->ClothingDefinitions,P.Value));
    if (Paths==LoadedPaths && !bLoading) { Render(); return; }
    ++RequestSerial; if (LoadHandle) LoadHandle->CancelHandle();
    LoadedPaths=Paths;
    if (Paths.IsEmpty()) { bLoading=false; Render(); return; }
    bLoading=true; const int32 Serial=RequestSerial; TWeakObjectPtr<UCharacterAppearanceComponent> Weak(this);
    LoadHandle=UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths,FStreamableDelegate::CreateLambda([Weak,Serial](){
        if (auto* Self=Weak.Get()) if (Serial==Self->RequestSerial) { Self->bLoading=false; Self->Render(); }
    }));
    if (!LoadHandle) { bLoading=false; Render(); }
}
void UCharacterAppearanceComponent::Primitive(FName ID,FVector Location,FVector Scale,FLinearColor Color,bool Visible)
{
    auto& P=Primitives.FindOrAdd(ID);
    if (!P)
    {
        P=NewObject<UStaticMeshComponent>(GetOwner()); P->SetupAttachment(VisualRoot);
        P->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        P->SetCollisionEnabled(ECollisionEnabled::NoCollision); P->SetCanEverAffectNavigation(false); P->RegisterComponent();
        const FString PartName = ID.ToString();
        const bool Cloth = ID==TEXT("Torso") || ID==TEXT("Hips") || ID==TEXT("Cap") || ID==TEXT("Backpack") || PartName.StartsWith(TEXT("Leg"));
        const bool Skin = ID==TEXT("Head") || ID==TEXT("Neck") || ID==TEXT("Jaw") || ID==TEXT("Chin") || ID==TEXT("Nose") || PartName.StartsWith(TEXT("Hand")) || PartName.StartsWith(TEXT("Arm")) || PartName.StartsWith(TEXT("Ear")) || PartName.StartsWith(TEXT("Cheek"));
        const TCHAR* Surface = Cloth ? TEXT("/Game/SurfaceQuality/Instances/MI_Fabric.MI_Fabric") : (Skin ? TEXT("/Game/SurfaceQuality/Instances/MI_Skin.MI_Skin") : TEXT("/Game/CharacterCreator/M_CharacterPlaceholder.M_CharacterPlaceholder"));
        auto* Base=LoadObject<UMaterialInterface>(nullptr,Surface);
        if (!Base) Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/CharacterCreator/M_CharacterPlaceholder.M_CharacterPlaceholder"));
        if (!Base) Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        auto* MID=UMaterialInstanceDynamic::Create(Base,this); P->SetMaterial(0,MID); Materials.Add(ID,MID);
    }
    P->SetRelativeLocation(Location); P->SetRelativeScale3D(Scale/100.0); P->SetVisibility(Visible);
    if (auto MID=Materials.FindRef(ID))
    {
        MID->SetVectorParameterValue(TEXT("BaseColor"),Color);
        MID->SetVectorParameterValue(TEXT("Tint"),Color);
        MID->SetScalarParameterValue(TEXT("Roughness"),Appearance.SkinRoughness);
        // Clothing keeps its own roughness; the skin control must not turn cloth glossy.
        const FString PartName=ID.ToString();
        if (ID==TEXT("Head") || ID==TEXT("Neck") || ID==TEXT("Jaw") || PartName.StartsWith(TEXT("Hand")))
            MID->SetScalarParameterValue(TEXT("RoughnessScale"),Appearance.SkinRoughness/.48f);
    }
}
void UCharacterAppearanceComponent::BuildPlaceholder()
{
    const auto& A=Appearance;
    auto Color=[](const TArray<FLinearColor>& P,int32 I){return P.IsValidIndex(I)?P[I]:FLinearColor(.4f,.3f,.2f);};
    const auto Skin=Color(Catalog->SkinPalette,A.SkinTone), Hair=Color(Catalog->HairPalette,A.HairColor), Beard=Color(Catalog->HairPalette,A.BeardColor);
    auto Morph=[&](const TCHAR* ID){return A.FaceMorphs.FindRef(FName(ID));};
    float Build=A.BodyBuild==TEXT("Slim")?.88f:(A.BodyBuild==TEXT("Heavy")?1.14f:(A.BodyBuild==TEXT("Athletic")?1.07f:1.f));
    if (A.Sex==EAppearanceSex::Female) Build*=.94f;
    const float FW=1+Morph(TEXT("Face.Width"))*.16f, FL=1+Morph(TEXT("Face.Length"))*.12f;
    auto ClothingColor=[&](EClothingSlot S){auto* P=Catalog->Part(Catalog->ClothingDefinitions,A.Clothing.FindRef(S)); int32 V=A.ClothingVariants.FindRef(S); return P && P->Variants.IsValidIndex(V)?P->Variants[V].Color:FLinearColor(.12f,.14f,.18f);};
    const bool Jacket=A.Clothing.Contains(EClothingSlot::Outerwear);
    const auto Top=ClothingColor(Jacket?EClothingSlot::Outerwear:EClothingSlot::Top), Bottom=ClothingColor(EClothingSlot::Bottom);
    Primitive(TEXT("Torso"),{0,0,117},{25*Build,43*Build,56},Top);
    Primitive(TEXT("Hips"),{0,0,88},{24*Build,33*Build,25},Bottom);
    Primitive(TEXT("Neck"),{0,0,148},{12,13,16},Skin);
    Primitive(TEXT("Head"),{0,0,164},{21,17*FW,27*FL},Skin);
    Primitive(TEXT("Jaw"),{4,0,155},{17,15*(1+Morph(TEXT("Jaw.Width"))*.18f),12*(1+Morph(TEXT("Jaw.Height"))*.12f)},Skin);
    Primitive(TEXT("Chin"),{9+Morph(TEXT("Chin.Projection"))*2,0,152},{6,8*(1+Morph(TEXT("Chin.Width"))*.15f),5},Skin);
    Primitive(TEXT("Nose"),{11+Morph(TEXT("Nose.Bridge")),0,163},{5+Morph(TEXT("Nose.Length"))*2,3.2+Morph(TEXT("Nose.Width")),6+Morph(TEXT("Nose.Tip"))},Skin*.93f);
    Primitive(TEXT("UpperLip"),{11,0,157},{1.5,6*(1+Morph(TEXT("Mouth.Width"))*.15f),1.1+Morph(TEXT("Mouth.UpperLip"))*.4},Skin*FLinearColor(.85f,.6f,.65f));
    Primitive(TEXT("LowerLip"),{11,0,155.8},{1.5,6*(1+Morph(TEXT("Mouth.Width"))*.15f),1.3+Morph(TEXT("Mouth.LowerLip"))*.4},Skin*FLinearColor(.85f,.6f,.65f));
    for (int I=0; I<2; ++I)
    {
        const float Side=I?1.f:-1.f, EyeY=Side*(4.3f+Morph(TEXT("Eyes.Spacing")));
        const FString S=FString::FromInt(I);
        Primitive(FName(*(TEXT("Cheek")+S)),{7,Side*6,160},{10,7+Morph(TEXT("Face.Cheeks")),8},Skin);
        const float EyeHeight=A.EyeShape==TEXT("Almond")?1.8f:(A.EyeShape==TEXT("Round")?2.8f:2.4f);
        Primitive(FName(*(TEXT("Eye")+S)),{9,EyeY,168},{2.5,3.3+Morph(TEXT("Eyes.Size")),EyeHeight},FLinearColor(.8f,.8f,.74f));
        Primitive(FName(*(TEXT("Iris")+S)),{10.4,EyeY,168},{.6,1.6,1.6},Color(Catalog->EyePalette,A.EyeColor));
        Primitive(FName(*(TEXT("Brow")+S)),{9,EyeY,171+Morph(TEXT("Eyes.Brows"))*2},{2,4,A.EyebrowStyle==TEXT("Thick")?1.4: .8},Color(Catalog->HairPalette,A.EyebrowColor));
        Primitive(FName(*(TEXT("Ear")+S)),{-1,Side*(9+Morph(TEXT("Ears.Angle"))*2),163},{4,3,7+Morph(TEXT("Ears.Size"))*2},Skin);
        Primitive(FName(*(TEXT("Arm")+S)),{0,Side*26*Build,116},{12,12,49},Jacket?Top:Skin);
        Primitive(FName(*(TEXT("Hand")+S)),{0,Side*26*Build,88},{8,8,13},Skin);
        Primitive(FName(*(TEXT("Leg")+S)),{0,Side*9,47},{16*Build,16*Build,80},Bottom);
        Primitive(FName(*(TEXT("Shoe")+S)),{5,Side*9,7},{27,13,12},ClothingColor(EClothingSlot::Shoes));
    }
    Primitive(TEXT("Hair"),{-2,0,175},{22,19*FW,A.HairStyle==TEXT("Crop")?9.f:14.f},Hair,A.HairStyle!=TEXT("None"));
    Primitive(TEXT("HairBack"),{-8,0,158},{12,22,30},Hair,A.HairStyle==TEXT("Medium"));
    const float BeardLength=A.BeardStyle==TEXT("Full")?16:(A.BeardStyle==TEXT("Medium")?11:6);
    Primitive(TEXT("Beard"),{8,0,152},{10,16,BeardLength},Beard,A.BeardStyle!=TEXT("None"));
    Primitive(TEXT("Cap"),{-1,0,179},{24,22,12},ClothingColor(EClothingSlot::Head),A.Clothing.Contains(EClothingSlot::Head));
    Primitive(TEXT("Glasses"),{11,0,168},{2,16,3},ClothingColor(EClothingSlot::Accessory),A.Clothing.Contains(EClothingSlot::Accessory));
    Primitive(TEXT("Backpack"),{-18,0,119},{18,31,43},ClothingColor(EClothingSlot::Back),A.Clothing.Contains(EClothingSlot::Back));
    for (int32 I=0; I<12; ++I)
        Primitive(FName(*FString::Printf(TEXT("Freckle%d"),I)),{11.6,(I%6-2.5)*1.9,160.5+(I/6)*1.2},{.35,.35,.35},Skin*.55f,I< FMath::RoundToInt(A.Freckles*12));
    Primitive(TEXT("AgeDetail"),{9.5,0,173.2},{.5,10,.25},Skin*.8f,A.VisualAge>34);
    Primitive(TEXT("SkinDetail"),{10,6,159},{.4,.8,.8},Skin*.7f,A.SkinImperfections>.4f);
}
void UCharacterAppearanceComponent::Render()
{
    const auto* B=Catalog->Body(Appearance.BodyPreset);
    HeightRatio=Appearance.Height/(B ? B->ReferenceHeight:180.f);
    VisualRoot->SetRelativeScale3D(FVector(HeightRatio));
    if (auto* C=Cast<ASliceCharacter>(GetOwner()))
    {
        // Keep the movement capsule stable across crouch/uncrouch and checkpoint restores.
        // The authored maximum-height body fits the standing capsule; only the visual rig scales.
        VisualRoot->SetRelativeLocation(FVector(0,0,-C->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
        C->Boom->SocketOffset=FVector(0,35,55+(Appearance.Height-180)*.65f);
        C->GetCharacterMovement()->GetNavAgentPropertiesRef().AgentHeight=C->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()*2;
    }
    bUsingPlaceholder=!B || !B->Mesh.Get();
    for (auto P:ModularParts) if (P) P->SetVisibility(false);
    for (auto& P:Primitives) P.Value->SetVisibility(false);
    BodyMesh->SetVisibility(!bUsingPlaceholder); FaceMesh->SetVisibility(!bUsingPlaceholder && B && B->FaceMesh.Get());
    if (bUsingPlaceholder) BuildPlaceholder();
    else
    {
        if (BodyMesh->GetSkeletalMeshAsset()!=B->Mesh.Get()) BodyMesh->SetSkeletalMesh(B->Mesh.Get());
        BodyMesh->SetRelativeRotation(B->MeshRotation);
        if (BodyMesh->GetAnimClass()!=B->AnimationClass.Get()) BodyMesh->SetAnimInstanceClass(B->AnimationClass.Get());
        BodyMesh->SetForcedLOD(bPreview?1:0);
        if (FaceMesh->GetSkeletalMeshAsset()!=B->FaceMesh.Get()) FaceMesh->SetSkeletalMesh(B->FaceMesh.Get());
        if (B->FaceAnimationClass.Get()) { if (FaceMesh->GetAnimClass()!=B->FaceAnimationClass.Get()) FaceMesh->SetAnimInstanceClass(B->FaceAnimationClass.Get()); }
        else FaceMesh->SetLeaderPoseComponent(BodyMesh);
        auto* Face=B->FaceMesh.Get()?FaceMesh.Get():BodyMesh.Get(); Face->ClearMorphTargets();
        for (const auto& M:Catalog->Morphs) Face->SetMorphTarget(M.MorphTargetName,Appearance.FaceMorphs.FindRef(M.MorphID));
        for (FName Build:{FName(TEXT("Slim")),FName(TEXT("Average")),FName(TEXT("Athletic")),FName(TEXT("Heavy"))}) BodyMesh->SetMorphTarget(FName(*(TEXT("Body_")+Build.ToString())),Build==Appearance.BodyBuild?1.f:0.f);
        for (auto* Mesh:{BodyMesh.Get(),Face})
        {
            for (int32 I=0; I<Mesh->GetNumMaterials(); ++I)
            {
                auto* MID=Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(I));
                if (!MID) MID=Mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(I,B->SkinMaterial.Get()?B->SkinMaterial.Get():Mesh->GetMaterial(I));
                if (!MID) continue;
                MID->SetVectorParameterValue(TEXT("SkinTone"),Catalog->SkinPalette[Appearance.SkinTone]);
                MID->SetVectorParameterValue(TEXT("EyeColor"),Catalog->EyePalette[Appearance.EyeColor]);
                MID->SetScalarParameterValue(TEXT("Roughness"),Appearance.SkinRoughness);
                MID->SetScalarParameterValue(TEXT("Freckles"),Appearance.Freckles);
                MID->SetScalarParameterValue(TEXT("Imperfections"),Appearance.SkinImperfections);
                MID->SetScalarParameterValue(TEXT("AgeDetail"),(Appearance.VisualAge-20)/25);
            }
        }
        auto Attach=[&](FName Role,const FAppearancePartDefinition* P,FLinearColor Color) {
            if (!P || P->ID==TEXT("None") || (P->bPlaceholder && !P->Mesh.Get() && !P->StaticMesh.Get())) return;
            const FName Key(*(Role.ToString()+TEXT(":")+P->ID.ToString()));
            UMeshComponent* Part=CachedParts.FindRef(Key);
            if (!Part && P->Mesh.Get())
            {
                auto* M=NewObject<USkeletalMeshComponent>(GetOwner()); M->SetupAttachment(BodyMesh); M->SetSkeletalMesh(P->Mesh.Get()); M->SetLeaderPoseComponent(BodyMesh); M->SetForcedLOD(bPreview?1:P->LODProfile); Part=M;
            }
            else if (!Part && P->StaticMesh.Get())
            { auto* M=NewObject<UStaticMeshComponent>(GetOwner()); M->SetupAttachment(BodyMesh,P->Socket); M->SetStaticMesh(P->StaticMesh.Get()); Part=M; }
            if (!Part) return;
            Part->SetRelativeTransform(P->Offset); Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            if (!Part->IsRegistered()) { Part->RegisterComponent(); ModularParts.Add(Part); CachedParts.Add(Key,Part); }
            Part->SetVisibility(true);
            for (int32 I=0; I<Part->GetNumMaterials(); ++I)
            {
                auto* MID=Cast<UMaterialInstanceDynamic>(Part->GetMaterial(I));
                if (!MID) MID=Part->CreateAndSetMaterialInstanceDynamicFromMaterial(I,P->Material.Get()?P->Material.Get():Part->GetMaterial(I));
                if (MID && P->ColorSupport) MID->SetVectorParameterValue(TEXT("BaseColor"),Color);
            }
        };
        Attach(TEXT("Hair"),Catalog->Part(Catalog->HairStyleDefinitions,Appearance.HairStyle),Catalog->HairPalette[Appearance.HairColor]);
        Attach(TEXT("Beard"),Catalog->Part(Catalog->BeardStyleDefinitions,Appearance.BeardStyle),Catalog->HairPalette[Appearance.BeardColor]);
        Attach(TEXT("Brows"),Catalog->Part(Catalog->EyebrowStyleDefinitions,Appearance.EyebrowStyle),Catalog->HairPalette[Appearance.EyebrowColor]);
        TSet<FName> Hidden;
        for (const auto& Pair:Appearance.Clothing)
        {
            const auto* P=Catalog->Part(Catalog->ClothingDefinitions,Pair.Value); if (!P) continue;
            int32 V=Appearance.ClothingVariants.FindRef(Pair.Key); Attach(TEXT("Clothing"),P,P->Variants.IsValidIndex(V)?P->Variants[V].Color:FLinearColor::White);
            if (P->Mesh.Get() || P->StaticMesh.Get()) for (FName Region:P->HiddenBodyRegions) Hidden.Add(Region);
        }
        for (int32 I=0; I<BodyMesh->GetNumMaterials(); ++I) for (int32 L=0; L<B->Mesh.Get()->GetLODNum(); ++L)
            BodyMesh->ShowMaterialSection(I,I,!Hidden.Contains(B->Mesh.Get()->GetMaterials()[I].MaterialSlotName),L);
    }
    OnAppearanceApplied(Appearance);
}
void UCharacterAppearanceComponent::TickComponent(float Dt,ELevelTick T,FActorComponentTickFunction* F)
{
    Super::TickComponent(Dt,T,F); if (!VisualRoot) return; Time+=Dt;
    const auto* Character=Cast<ASliceCharacter>(GetOwner());
    const bool Crouched=bPreview?PreviewMovement==TEXT("Crouch"):(Character && Character->bIsCrouched);
    if (Character) VisualRoot->SetRelativeLocation(FVector(0,0,-Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
    float Speed=GetOwner()->GetVelocity().Size2D();
    if (bPreview) Speed=PreviewMovement==TEXT("Walk")?150:(PreviewMovement==TEXT("Jog")?330:0);
    if (bUsingPlaceholder)
    {
        VisualRoot->SetRelativeScale3D(FVector(HeightRatio,HeightRatio,HeightRatio*(Crouched?.65f:1.f)));
        if (auto P=Primitives.FindRef(TEXT("Torso"))) P->SetRelativeLocation(FVector(0,0,117+FMath::Sin(Time*1.8f)*.35f));
        if (auto P=Primitives.FindRef(TEXT("Head"))) P->SetRelativeRotation(FRotator(0,FMath::Sin(Time*.6f)*2,0));
        for (int32 I=0; I<2; ++I)
        {
            const float Swing=FMath::Sin(Time*(Speed>250?10:6))*FMath::Min(Speed*.06f,20.f)*(I?1:-1);
            for (const TCHAR* Part:{TEXT("Arm"),TEXT("Leg")}) if (auto P=Primitives.FindRef(FName(*FString::Printf(TEXT("%s%d"),Part,I)))) P->SetRelativeRotation(FRotator(Swing,0,0));
            if (auto P=Primitives.FindRef(FName(*FString::Printf(TEXT("Eye%d"),I)))) P->SetVisibility(FMath::Fmod(Time,4.f)>.12f);
        }
        if (bPreview) VisualRoot->SetRelativeLocation(FVector::ZeroVector);
    }
    else
    {
        const auto* B=Catalog->Body(Appearance.BodyPreset); if (!B) return;
        UAnimationAsset* Anim=nullptr;
        if (bPreview)
            Anim=PreviewMovement==TEXT("Walk")?B->Walk.Get():(PreviewMovement==TEXT("Jog")?B->Jog.Get():(PreviewMovement==TEXT("Crouch")?B->Crouch.Get():B->CreatorIdle.Get()));
        else if (!B->AnimationClass.Get())
            Anim=Crouched?B->Crouch.Get():(Speed>380?B->Jog.Get():(Speed>20?B->Walk.Get():B->CreatorIdle.Get()));
        if (Anim && BodyMesh->AnimationData.AnimToPlay!=Anim) BodyMesh->PlayAnimation(Anim,true);
    }
}
TArray<FName> UWardrobeComponent::OwnedClothing() const
{ return GetWorld()->GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>()->Committed.OwnedClothing; }
void UWardrobeComponent::GrantClothing(FName ID)
{
    auto* S=GetWorld()->GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>();
    if (S->Catalog->Part(S->Catalog->ClothingDefinitions,ID)) S->Committed.OwnedClothing.AddUnique(ID);
}
bool UWardrobeComponent::Equip(EClothingSlot Slot,FName ID,int32 Variant)
{
    auto* S=GetWorld()->GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>(); auto* P=S->Catalog->Part(S->Catalog->ClothingDefinitions,ID);
    if (S->bEditing || !P || P->Slot!=Slot || !S->Committed.OwnedClothing.Contains(ID) || !S->Catalog->Compatible(*P,S->Committed.PlayerAppearanceData)) return false;
    auto Before=S->Committed; auto Next=Before;
    Next.PlayerAppearanceData.Clothing.Add(Slot,ID); Next.PlayerAppearanceData.ClothingVariants.Add(Slot,Variant); S->Restore(Next);
    const bool Saved=GetWorld()->GetSubsystem<UCityGameplaySubsystem>()->IsActive() ? GetWorld()->GetSubsystem<UCityGameplaySubsystem>()->SaveAppearance() : GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>()->SaveCheckpoint();
    if (!Saved) { S->Restore(Before); return false; }
    if (auto* A=GetOwner()->FindComponentByClass<UCharacterAppearanceComponent>()) A->ApplyAppearance(S->Committed.PlayerAppearanceData);
    return true;
}
