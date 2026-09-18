#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Save/SliceSave.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

namespace {
UCharacterCreatorSubsystem* MakeCreator()
{
    auto* GI=NewObject<UGameInstance>();
    auto* S=NewObject<UCharacterCreatorSubsystem>(GI); S->Catalog=NewObject<UCharacterAppearanceCatalog>(S); S->Catalog->BuildFallbackCatalog(); S->BeginCreation(); return S;
}
bool Same(const FCharacterAppearanceDefinition& A,const FCharacterAppearanceDefinition& B)
{ return FCharacterAppearanceDefinition::StaticStruct()->CompareScriptStruct(&A,&B,0); }
}
#define CREATOR_TEST(Name) IMPLEMENT_SIMPLE_AUTOMATION_TEST(F##Name,"WTG.CharacterCreator." #Name,EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) bool F##Name::RunTest(const FString& Parameters)
CREATOR_TEST(RandomizationDeterminism)
{
    auto* S=MakeCreator(); auto A=S->GenerateRandomNPCAppearance(12345); auto* Other=MakeCreator();
    TestTrue(TEXT("Fresh subsystem, seed 12345 produces identical character"),Same(A,Other->GenerateRandomNPCAppearance(12345)));
    TestFalse(TEXT("Different seed changes identity"),Same(A,S->GenerateRandomNPCAppearance(12346)));
    for (int I=0; I<1000; ++I) { auto R=S->GenerateRandomNPCAppearance(I); if (!Same(R,UCharacterCreatorValidator::Normalize(*S->Catalog,R))) { AddError(TEXT("Random result not normalized")); break; } }
    return true;
}
CREATOR_TEST(SaveLoadAppearance)
{
    auto* S=MakeCreator(); S->Randomize(12345); auto Saved=S->Draft;
    auto* Save=NewObject<USliceSave>(); Save->CharacterCustomization=S->MakeSaveData(); TArray<uint8> Bytes;
    TestTrue(TEXT("Serialize full campaign SaveGame"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
    S->Randomize(54321); auto* Restored=Cast<USliceSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Reload"),Restored)) return false;
    S->bEditing=false; S->Restore(Restored->CharacterCustomization);
    TestTrue(TEXT("Restore saved appearance after edits"),Same(Saved,S->Committed.PlayerAppearanceData));
    auto* Restarted=MakeCreator(); TestTrue(TEXT("Saved seed reproduces after restart"),Same(Saved,Restarted->GenerateRandomNPCAppearance(Restored->CharacterCustomization.PlayerAppearanceData.RandomSeed)));
    TestTrue(TEXT("Inventory persisted"),Restored->CharacterCustomization.OwnedClothing.Num()>=3); return true;
}
CREATOR_TEST(MissingAssetFallback)
{
    auto* S=MakeCreator(); auto A=S->Draft; A.HairStyle=TEXT("DeletedHair"); A.BodyPreset=TEXT("MissingBody"); A.Clothing.Add(EClothingSlot::Top,TEXT("DeletedShirt"));
    A=UCharacterCreatorValidator::Normalize(*S->Catalog,A);
    TestNotNull(TEXT("Fallback body"),S->Catalog->Body(A.BodyPreset)); TestNotNull(TEXT("Fallback hair"),S->Catalog->Part(S->Catalog->HairStyleDefinitions,A.HairStyle));
    TestNotNull(TEXT("Fallback clothing"),S->Catalog->Part(S->Catalog->ClothingDefinitions,A.Clothing.FindRef(EClothingSlot::Top)));
    FCharacterCustomizationSaveData Future; Future.CharacterAppearanceVersion=999; S->Restore(Future);
    TestEqual(TEXT("Future version uses default identity"),S->Committed.PlayerAppearanceData.PresetID,S->Catalog->FallbackDefinition.PresetID); return true;
}
CREATOR_TEST(InvalidMorphClamp)
{
    auto* S=MakeCreator(); auto A=S->Draft; A.Height=999; A.VisualAge=-2;
    for (const auto& M:S->Catalog->Morphs) A.FaceMorphs.Add(M.MorphID,999);
    A.FaceMorphs.Add(TEXT("Unknown"),99); A=UCharacterCreatorValidator::Normalize(*S->Catalog,A);
    TestEqual(TEXT("Height bound"),A.Height,195.f); TestEqual(TEXT("Age bound"),A.VisualAge,20.f);
    TestFalse(TEXT("Unknown morph dropped"),A.FaceMorphs.Contains(TEXT("Unknown")));
    float Energy=0; for (const auto& M:S->Catalog->Morphs) { float V=A.FaceMorphs.FindRef(M.MorphID); TestTrue(TEXT("Morph bound"),V>=M.MinValue && V<=M.MaxValue); Energy+=V*V; }
    TestTrue(TEXT("Combination budget"),Energy<=1.0001f); return true;
}
CREATOR_TEST(ClothingCompatibility)
{
    auto* S=MakeCreator(); auto& P=S->Catalog->ClothingDefinitions[0]; P.ClothingCompatibilityTags={TEXT("Body.Male.Standard")};
    auto A=S->Catalog->Presets[2]; A.Clothing.Add(P.Slot,P.ID);
    TestFalse(TEXT("Male-only jacket incompatible"),S->Catalog->Compatible(P,A));
    A=UCharacterCreatorValidator::Normalize(*S->Catalog,A); TestNotEqual(TEXT("Incompatible jacket replaced"),A.Clothing.FindRef(P.Slot),P.ID); return true;
}
CREATOR_TEST(PresetLoading)
{
    auto* S=MakeCreator(); TestEqual(TEXT("Four presets"),S->Catalog->Presets.Num(),4);
    for (int32 I=0; I<4; ++I) { S->SelectPreset(I); TestEqual(TEXT("Preset identity"),S->Draft.PresetID,S->Catalog->Presets[I].PresetID); }
    TestTrue(TEXT("Catalog valid with explicit placeholders"),UCharacterCreatorValidator::ValidateCatalog(S->Catalog,false).IsEmpty()); return true;
}
CREATOR_TEST(UndoRedoAndCancel)
{
    auto* S=MakeCreator(); const auto Original=S->Draft;
    S->Randomize(12345); auto Next=S->Draft; S->Undo(); TestTrue(TEXT("Undo"),Same(Original,S->Draft));
    S->Redo(); TestTrue(TEXT("Redo"),Same(Next,S->Draft)); S->Undo(); S->Randomize(42); TestFalse(TEXT("Branch discards redo"),S->CanRedo());
    for (int I=0; I<70; ++I) S->Randomize(I);
    int Count=0; while (S->CanUndo()) { S->Undo(); ++Count; } TestEqual(TEXT("Bounded 50 entries"),Count,50);
    const auto Before=S->Committed; S->Cancel(); TestTrue(TEXT("Cancel preserves save"),Same(Before.PlayerAppearanceData,S->Committed.PlayerAppearanceData)); return true;
}
#undef CREATOR_TEST
#endif
