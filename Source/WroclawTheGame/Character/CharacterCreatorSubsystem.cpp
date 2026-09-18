#include "Character/CharacterCreatorSubsystem.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Animation/Skeleton.h"

namespace {
float Safe(float V,float Min,float Max,float Default) { return FMath::IsFinite(V) ? FMath::Clamp(V,Min,Max) : Default; }
FName ResolvePart(const UCharacterAppearanceCatalog& C,const TArray<FAppearancePartDefinition>& Parts,FName ID,const FCharacterAppearanceDefinition& A)
{
    const auto* P=C.Part(Parts,ID);
    if (P && C.Compatible(*P,A)) return ID;
    for (const auto& Candidate:Parts) if (C.Compatible(Candidate,A)) return Candidate.ID;
    return NAME_None;
}
}
FCharacterAppearanceDefinition UCharacterCreatorValidator::Normalize(const UCharacterAppearanceCatalog& C,FCharacterAppearanceDefinition A)
{
    A.Height=Safe(A.Height,160,195,180); A.VisualAge=Safe(A.VisualAge,20,45,27);
    A.SkinRoughness=Safe(A.SkinRoughness,.3f,.8f,.5f);
    A.Freckles=Safe(A.Freckles,0,1,0); A.SkinImperfections=Safe(A.SkinImperfections,0,1,.2f);
    if (A.Sex!=EAppearanceSex::Male && A.Sex!=EAppearanceSex::Female) A.Sex=EAppearanceSex::Male;
    const auto* B=C.Body(A.BodyPreset);
    if (!B || B->Sex!=A.Sex)
    {
        B=C.Bodies.FindByPredicate([&](const auto& Body){return Body.Sex==A.Sex;});
        if (!B && C.Bodies.Num()) B=&C.Bodies[0];
        if (B) { A.BodyPreset=B->ID; A.Sex=B->Sex; }
    }
    if (A.BodyBuild!=TEXT("Slim") && A.BodyBuild!=TEXT("Average") && A.BodyBuild!=TEXT("Athletic") && A.BodyBuild!=TEXT("Heavy")) A.BodyBuild=TEXT("Average");
    if (!C.FacePresets.Contains(A.FacePreset)) A.FacePreset=C.FacePresets.IsEmpty()?NAME_None:C.FacePresets[0];
    if (!C.EyeShapes.Contains(A.EyeShape)) A.EyeShape=C.EyeShapes.IsEmpty()?NAME_None:C.EyeShapes[0];
    if (!C.Presets.ContainsByPredicate([&](const auto& P){return P.PresetID==A.PresetID;})) A.PresetID=C.FallbackDefinition.PresetID;
    if (!C.VoiceProfiles.ContainsByPredicate([&](const auto& V){return V.ID==A.VoiceProfileID;})) A.VoiceProfileID=C.VoiceProfiles.IsEmpty()?NAME_None:C.VoiceProfiles[0].ID;
    A.Name=A.Name.Left(40).TrimStartAndEnd().Replace(TEXT("\n"),TEXT(" ")).Replace(TEXT("\r"),TEXT(" "));
    A.SkinTone=FMath::Clamp(A.SkinTone,0,FMath::Max(0,C.SkinPalette.Num()-1));
    A.EyeColor=FMath::Clamp(A.EyeColor,0,FMath::Max(0,C.EyePalette.Num()-1));
    A.HairColor=FMath::Clamp(A.HairColor,0,FMath::Max(0,C.HairPalette.Num()-1));
    A.BeardColor=FMath::Clamp(A.BeardColor,0,FMath::Max(0,C.HairPalette.Num()-1));
    A.EyebrowColor=FMath::Clamp(A.EyebrowColor,0,FMath::Max(0,C.HairPalette.Num()-1));
    A.HairStyle=ResolvePart(C,C.HairStyleDefinitions,A.HairStyle,A);
    A.BeardStyle=ResolvePart(C,C.BeardStyleDefinitions,A.BeardStyle,A);
    A.EyebrowStyle=ResolvePart(C,C.EyebrowStyleDefinitions,A.EyebrowStyle,A);
    TMap<FName,float> Morphs;
    float Energy=0;
    for (const auto& M:C.Morphs)
    {
        const float* V=A.FaceMorphs.Find(M.MorphID);
        float Value=Safe(V?*V:M.DefaultValue,M.MinValue,M.MaxValue,M.DefaultValue);
        Morphs.Add(M.MorphID,Value); Energy+=FMath::Square(Value-M.DefaultValue);
    }
    // Joint budget supplements per-morph limits. Final bounds require validation on authored faces.
    if (Energy>1.f) for (const auto& M:C.Morphs) Morphs[M.MorphID]=M.DefaultValue+(Morphs[M.MorphID]-M.DefaultValue)/FMath::Sqrt(Energy);
    A.FaceMorphs=MoveTemp(Morphs);
    TMap<EClothingSlot,FName> Clothes; TMap<EClothingSlot,int32> Variants;
    for (int32 I=0; I<7; ++I)
    {
        const auto S=static_cast<EClothingSlot>(I); const FName ID=A.Clothing.FindRef(S);
        auto* P=C.Part(C.ClothingDefinitions,ID);
        if (!P || P->Slot!=S || !C.Compatible(*P,A))
        {
            P=nullptr;
            if (I==1 || I==2 || I==3 || !ID.IsNone())
                P=C.ClothingDefinitions.FindByPredicate([&](const auto& X){return X.Slot==S && C.Compatible(X,A);});
        }
        if (P) { Clothes.Add(S,P->ID); Variants.Add(S,FMath::Clamp(A.ClothingVariants.FindRef(S),0,FMath::Max(0,P->Variants.Num()-1))); }
    }
    A.Clothing=MoveTemp(Clothes); A.ClothingVariants=MoveTemp(Variants); return A;
}
TArray<FString> UCharacterCreatorValidator::ValidateCatalog(UCharacterAppearanceCatalog* C,bool CheckAssets)
{
    TArray<FString> Errors; if (!C) { Errors.Add(TEXT("Missing catalog")); return Errors; }
    auto IDs=[&](const auto& Items,auto GetID,const TCHAR* Group,bool AllowNone=false) {
        TSet<FName> Seen; for (const auto& P:Items) { const FName ID=GetID(P); if ((!AllowNone && ID.IsNone()) || Seen.Contains(ID)) Errors.Add(FString(Group)+TEXT(": duplicate/empty ID ")+ID.ToString()); Seen.Add(ID); }
    };
    IDs(C->Bodies,[](const auto& P){return P.ID;},TEXT("Body"));
    IDs(C->Morphs,[](const auto& P){return P.MorphID;},TEXT("Morph"));
    IDs(C->Presets,[](const auto& P){return P.PresetID;},TEXT("Preset"));
    IDs(C->VoiceProfiles,[](const auto& P){return P.ID;},TEXT("Voice"));
    if (C->Bodies.IsEmpty() || C->Presets.IsEmpty() || C->SkinPalette.IsEmpty() || C->HairPalette.IsEmpty() || C->EyePalette.IsEmpty()) Errors.Add(TEXT("Required catalog array is empty"));
    for (const auto& B:C->Bodies) if (!FMath::IsFinite(B.ReferenceHeight) || B.ReferenceHeight<100 || B.ReferenceHeight>220) Errors.Add(TEXT("Invalid reference height: ")+B.ID.ToString());
    for (const auto& M:C->Morphs) if (M.MorphTargetName.IsNone() || !FMath::IsFinite(M.MinValue) || !FMath::IsFinite(M.MaxValue) || !FMath::IsFinite(M.DefaultValue) || M.MinValue>M.DefaultValue || M.MaxValue<M.DefaultValue) Errors.Add(TEXT("Invalid morph bounds/name: ")+M.MorphID.ToString());
    for (const auto* Parts:{&C->HairStyleDefinitions,&C->BeardStyleDefinitions,&C->EyebrowStyleDefinitions,&C->ClothingDefinitions})
    {
        IDs(*Parts,[](const auto& P){return P.ID;},TEXT("Part"),true);
        for (const auto& P:*Parts)
        {
            if (P.LODProfile<0) Errors.Add(TEXT("Invalid LOD: ")+P.ID.ToString());
            for (auto Tag:P.ClothingCompatibilityTags) if (!C->Bodies.ContainsByPredicate([&](const auto& B){return B.CompatibilityTag==Tag;})) Errors.Add(TEXT("Unknown body tag: ")+Tag.ToString());
            if (!CheckAssets || P.bPlaceholder || P.ID==TEXT("None")) continue;
            auto* Mesh=P.Mesh.LoadSynchronous(); auto* Static=P.StaticMesh.LoadSynchronous();
            if (!Mesh && !Static) Errors.Add(TEXT("Missing mesh: ")+P.ID.ToString());
            if (Mesh && Mesh->GetLODNum()<2) Errors.Add(TEXT("Missing runtime LOD: ")+P.ID.ToString());
            if (Static && Static->GetNumLODs()<2) Errors.Add(TEXT("Missing static LOD: ")+P.ID.ToString());
            if (!P.Material.IsNull() && !P.Material.LoadSynchronous()) Errors.Add(TEXT("Missing material: ")+P.ID.ToString());
        }
    }
    if (CheckAssets) for (const auto& B:C->Bodies)
    {
        if (B.bPlaceholder) continue;
        auto* Mesh=B.Mesh.LoadSynchronous(); auto* Face=B.FaceMesh.IsNull()?Mesh:B.FaceMesh.LoadSynchronous();
        if (!Mesh || !Face) { Errors.Add(TEXT("Missing body/face: ")+B.ID.ToString()); continue; }
        if (Mesh->GetLODNum()<2 || Face->GetLODNum()<2) Errors.Add(TEXT("Missing body LOD: ")+B.ID.ToString());
        if (!B.AnimationClass.LoadSynchronous()) Errors.Add(TEXT("Missing animation class: ")+B.ID.ToString());
        if (!B.SkinMaterial.LoadSynchronous()) Errors.Add(TEXT("Missing skin material: ")+B.ID.ToString());
        for (const auto& M:C->Morphs) if (!Face->FindMorphTarget(M.MorphTargetName)) Errors.Add(TEXT("Missing morph ")+M.MorphTargetName.ToString()+TEXT(" on ")+B.ID.ToString());
        for (const auto& P:C->ClothingDefinitions) if (P.ClothingCompatibilityTags.IsEmpty() || P.ClothingCompatibilityTags.Contains(B.CompatibilityTag))
            if (auto* Clothing=P.Mesh.LoadSynchronous()) if (Clothing->GetSkeleton()!=Mesh->GetSkeleton()) Errors.Add(TEXT("Clothing skeleton mismatch: ")+P.ID.ToString());
    }
    return Errors;
}
void UCharacterCreatorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Catalog=LoadObject<UCharacterAppearanceCatalog>(nullptr,TEXT("/Game/CharacterCreator/DA_CharacterAppearanceCatalog.DA_CharacterAppearanceCatalog"));
    if (Catalog && !UCharacterCreatorValidator::ValidateCatalog(Catalog,false).IsEmpty())
    { UE_LOG(LogTemp,Warning,TEXT("Invalid Character Creator catalog; using built-in fallback.")); Catalog=nullptr; }
    if (!Catalog) { Catalog=NewObject<UCharacterAppearanceCatalog>(this); Catalog->BuildFallbackCatalog(); }
    Catalog->ApplyModernHeroProfile();
    Committed.PlayerAppearanceData=UCharacterCreatorValidator::Normalize(*Catalog,Catalog->FallbackDefinition);
    Draft=Committed.PlayerAppearanceData;
}
void UCharacterCreatorSubsystem::BeginCreation() { bEditing=true; bSummary=false; Draft=Catalog->FallbackDefinition; History.Empty(); Cursor=0; OnChanged.Broadcast(); }
void UCharacterCreatorSubsystem::Cancel() { bEditing=false; bSummary=false; Draft=Committed.PlayerAppearanceData; History.Empty(); Cursor=0; OnChanged.Broadcast(); }
void UCharacterCreatorSubsystem::Change(const FCharacterAppearanceDefinition& A)
{
    auto Next=UCharacterCreatorValidator::Normalize(*Catalog,A);
    if (FCharacterAppearanceDefinition::StaticStruct()->CompareScriptStruct(&Draft,&Next,0)) return;
    History.SetNum(Cursor); FAppearanceChange Entry; Entry.Before=Draft; Entry.After=Next; History.Add(Entry);
    if (History.Num()>50) History.RemoveAt(0); Cursor=History.Num(); Draft=Next; OnChanged.Broadcast();
}
void UCharacterCreatorSubsystem::Undo() { if (CanUndo()) { Draft=History[--Cursor].Before; OnChanged.Broadcast(); } }
void UCharacterCreatorSubsystem::Redo() { if (CanRedo()) { Draft=History[Cursor++].After; OnChanged.Broadcast(); } }
void UCharacterCreatorSubsystem::ResetAll() { Change(Catalog->FallbackDefinition); }
void UCharacterCreatorSubsystem::SelectPreset(int32 I) { if (Catalog->Presets.IsValidIndex(I)) Change(Catalog->Presets[I]); }
void UCharacterCreatorSubsystem::ResetCategory(EAppearanceCategory C)
{
    auto A=Draft; const auto& D=Catalog->FallbackDefinition;
    switch (C)
    {
    case EAppearanceCategory::Character: A.Name=D.Name; A.Sex=D.Sex; A.BodyPreset=D.BodyPreset; break;
    case EAppearanceCategory::Body: A.Height=D.Height; A.BodyBuild=D.BodyBuild; break;
    case EAppearanceCategory::Skin: A.SkinTone=D.SkinTone; A.VisualAge=D.VisualAge; A.SkinRoughness=D.SkinRoughness; A.Freckles=D.Freckles; A.SkinImperfections=D.SkinImperfections; break;
    case EAppearanceCategory::Hair: A.HairStyle=D.HairStyle; A.HairColor=D.HairColor; A.EyebrowStyle=D.EyebrowStyle; A.EyebrowColor=D.EyebrowColor; break;
    case EAppearanceCategory::Beard: A.BeardStyle=D.BeardStyle; A.BeardColor=D.BeardColor; break;
    case EAppearanceCategory::Clothing: A.Clothing=D.Clothing; A.ClothingVariants=D.ClothingVariants; break;
    case EAppearanceCategory::Voice: A.VoiceProfileID=D.VoiceProfileID; break;
    case EAppearanceCategory::Face: A.FacePreset=D.FacePreset; break;
    case EAppearanceCategory::Eyes: A.EyeColor=D.EyeColor; A.EyeShape=D.EyeShape; break;
    default: break;
    }
    for (const auto& M:Catalog->Morphs) if (M.Category==C) A.FaceMorphs.Add(M.MorphID,M.DefaultValue);
    Change(A);
}
FCharacterAppearanceDefinition UCharacterCreatorSubsystem::GenerateRandomNPCAppearance(int32 Seed) const
{
    FRandomStream R(Seed); auto A=Catalog->FallbackDefinition; A.RandomSeed=Seed;
    if (Catalog->Presets.Num()) A=Catalog->Presets[R.RandRange(0,Catalog->Presets.Num()-1)];
    A.RandomSeed=Seed; A.Height=R.FRandRange(160,195); A.VisualAge=R.FRandRange(20,45);
    const FName Builds[]={TEXT("Slim"),TEXT("Average"),TEXT("Athletic"),TEXT("Heavy")}; A.BodyBuild=Builds[R.RandRange(0,3)];
    A.SkinTone=R.RandRange(0,FMath::Max(0,Catalog->SkinPalette.Num()-1)); A.EyeColor=R.RandRange(0,FMath::Max(0,Catalog->EyePalette.Num()-1));
    A.HairColor=R.RandRange(0,FMath::Max(0,Catalog->HairPalette.Num()-1)); A.BeardColor=A.EyebrowColor=A.HairColor;
    A.Freckles=R.FRandRange(0,.5f); A.SkinRoughness=R.FRandRange(.4f,.65f); A.SkinImperfections=R.FRandRange(0,.5f);
    auto Pick=[&](const TArray<FAppearancePartDefinition>& Parts) { TArray<FName> Valid; for (const auto& P:Parts) if (Catalog->Compatible(P,A)) Valid.Add(P.ID); return Valid.IsEmpty()?NAME_None:Valid[R.RandRange(0,Valid.Num()-1)]; };
    A.HairStyle=Pick(Catalog->HairStyleDefinitions); A.BeardStyle=Pick(Catalog->BeardStyleDefinitions); A.EyebrowStyle=Pick(Catalog->EyebrowStyleDefinitions);
    if (Catalog->FacePresets.Num()) A.FacePreset=Catalog->FacePresets[R.RandRange(0,Catalog->FacePresets.Num()-1)];
    for (const auto& M:Catalog->Morphs) A.FaceMorphs.Add(M.MorphID,R.FRandRange(M.MinValue*.65f,M.MaxValue*.65f));
    for (int32 I=0; I<7; ++I)
    {
        const auto S=static_cast<EClothingSlot>(I); TArray<const FAppearancePartDefinition*> Valid;
        for (const auto& P:Catalog->ClothingDefinitions) if (P.Slot==S && Catalog->Compatible(P,A)) Valid.Add(&P);
        if (Valid.Num()) { const auto* P=Valid[R.RandRange(0,Valid.Num()-1)]; A.Clothing.Add(S,P->ID); A.ClothingVariants.Add(S,R.RandRange(0,FMath::Max(0,P->Variants.Num()-1))); }
        if ((I==0 || I>=4) && R.FRand()<.5f) A.Clothing.Remove(S);
    }
    return UCharacterCreatorValidator::Normalize(*Catalog,A);
}
void UCharacterCreatorSubsystem::Randomize(int32 Seed) { Change(GenerateRandomNPCAppearance(Seed)); }
void UCharacterCreatorSubsystem::Restore(const FCharacterCustomizationSaveData& Data)
{
    Committed=Data;
    // Future formats are displayed using a safe fallback; never dereference serialized asset paths.
    if (Data.CharacterAppearanceVersion>1 || Data.CharacterAppearanceVersion<1) Committed.PlayerAppearanceData=Catalog->FallbackDefinition;
    Committed.CharacterAppearanceVersion=1;
    Committed.PlayerAppearanceData=UCharacterCreatorValidator::Normalize(*Catalog,Committed.PlayerAppearanceData);
    Committed.OwnedClothing.RemoveAll([&](FName ID){return !Catalog->Part(Catalog->ClothingDefinitions,ID);});
    for (const auto& P:Committed.PlayerAppearanceData.Clothing) Committed.OwnedClothing.AddUnique(P.Value);
    if (!bEditing) Draft=Committed.PlayerAppearanceData;
    OnChanged.Broadcast();
}
FCharacterCustomizationSaveData UCharacterCreatorSubsystem::MakeSaveData() const
{
    FCharacterCustomizationSaveData D; D.PlayerAppearanceData=UCharacterCreatorValidator::Normalize(*Catalog,Draft);
    for (const auto& P:D.PlayerAppearanceData.Clothing) D.OwnedClothing.AddUnique(P.Value);
    return D;
}
FString UCharacterCreatorSubsystem::DebugText() const
{
    FString S=FString::Printf(TEXT("Seed: %d | v1 | %s | %s\nBody: %s | Hair: %s | Beard: %s\n"),Draft.RandomSeed,*Draft.PresetID.ToString(),*Draft.Name,*Draft.BodyPreset.ToString(),*Draft.HairStyle.ToString(),*Draft.BeardStyle.ToString());
    for (const auto& M:Catalog->Morphs) S+=FString::Printf(TEXT("%s = %.3f\n"),*M.MorphID.ToString(),Draft.FaceMorphs.FindRef(M.MorphID));
    for (const auto& P:Draft.Clothing) S+=P.Value.ToString()+TEXT("\n"); return S;
}
