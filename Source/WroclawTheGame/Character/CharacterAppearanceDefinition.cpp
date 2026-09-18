#include "Character/CharacterAppearanceDefinition.h"

const FBodyPresetDefinition* UCharacterAppearanceCatalog::Body(FName ID) const
{ return Bodies.FindByPredicate([&](const auto& B){ return B.ID == ID; }); }
const FAppearancePartDefinition* UCharacterAppearanceCatalog::Part(const TArray<FAppearancePartDefinition>& Parts, FName ID) const
{ return Parts.FindByPredicate([&](const auto& P){ return P.ID == ID; }); }
bool UCharacterAppearanceCatalog::Compatible(const FAppearancePartDefinition& P, const FCharacterAppearanceDefinition& A) const
{
    const auto* B = Body(A.BodyPreset);
    return B && (P.ClothingCompatibilityTags.IsEmpty() || P.ClothingCompatibilityTags.Contains(B->CompatibilityTag));
}
void UCharacterAppearanceCatalog::BuildFallbackCatalog()
{
    Bodies.Empty(); Morphs.Empty(); HairStyleDefinitions.Empty(); BeardStyleDefinitions.Empty();
    EyebrowStyleDefinitions.Empty(); ClothingDefinitions.Empty(); Presets.Empty(); VoiceProfiles.Empty();
    for (int32 I=0; I<2; ++I)
    {
        FBodyPresetDefinition B;
        B.ID = I ? TEXT("Female.Standard") : TEXT("Male.Standard");
        B.Sex = I ? EAppearanceSex::Female : EAppearanceSex::Male;
        B.CompatibilityTag = I ? TEXT("Body.Female.Standard") : TEXT("Body.Male.Standard");
        Bodies.Add(B);
    }
    auto Morph = [&](const TCHAR* ID, const TCHAR* Label, EAppearanceCategory C) {
        FFaceMorphDefinition M; M.MorphID=ID; M.DisplayName=FText::FromString(Label);
        M.MorphTargetName=FName(*FString(ID).Replace(TEXT("."),TEXT("_"))); M.Category=C; Morphs.Add(M);
    };
    Morph(TEXT("Face.Width"),TEXT("Szerokość twarzy"),EAppearanceCategory::Face);
    Morph(TEXT("Face.Length"),TEXT("Długość twarzy"),EAppearanceCategory::Face);
    Morph(TEXT("Face.Cheeks"),TEXT("Policzki"),EAppearanceCategory::Face);
    Morph(TEXT("Eyes.Size"),TEXT("Wielkość oczu"),EAppearanceCategory::Eyes);
    Morph(TEXT("Eyes.Spacing"),TEXT("Rozstaw oczu"),EAppearanceCategory::Eyes);
    Morph(TEXT("Eyes.Brows"),TEXT("Wysokość brwi"),EAppearanceCategory::Eyes);
    Morph(TEXT("Nose.Width"),TEXT("Szerokość nosa"),EAppearanceCategory::Nose);
    Morph(TEXT("Nose.Length"),TEXT("Długość nosa"),EAppearanceCategory::Nose);
    Morph(TEXT("Nose.Bridge"),TEXT("Grzbiet nosa"),EAppearanceCategory::Nose);
    Morph(TEXT("Nose.Tip"),TEXT("Czubek nosa"),EAppearanceCategory::Nose);
    Morph(TEXT("Jaw.Width"),TEXT("Szerokość szczęki"),EAppearanceCategory::Jaw);
    Morph(TEXT("Jaw.Height"),TEXT("Wysokość szczęki"),EAppearanceCategory::Jaw);
    Morph(TEXT("Chin.Width"),TEXT("Szerokość brody"),EAppearanceCategory::Jaw);
    Morph(TEXT("Chin.Projection"),TEXT("Wysunięcie brody"),EAppearanceCategory::Jaw);
    Morph(TEXT("Mouth.Width"),TEXT("Szerokość ust"),EAppearanceCategory::Mouth);
    Morph(TEXT("Mouth.UpperLip"),TEXT("Górna warga"),EAppearanceCategory::Mouth);
    Morph(TEXT("Mouth.LowerLip"),TEXT("Dolna warga"),EAppearanceCategory::Mouth);
    Morph(TEXT("Ears.Size"),TEXT("Wielkość uszu"),EAppearanceCategory::Ears);
    Morph(TEXT("Ears.Angle"),TEXT("Odstawanie uszu"),EAppearanceCategory::Ears);
    auto AddParts = [](TArray<FAppearancePartDefinition>& Parts, std::initializer_list<const TCHAR*> IDs) {
        for (const TCHAR* ID:IDs) { FAppearancePartDefinition P; P.ID=ID; P.DisplayName=FText::FromString(ID); Parts.Add(P); }
    };
    AddParts(HairStyleDefinitions,{TEXT("None"),TEXT("Short"),TEXT("Medium"),TEXT("Crop")});
    AddParts(BeardStyleDefinitions,{TEXT("None"),TEXT("Stubble"),TEXT("Short"),TEXT("Medium"),TEXT("Full")});
    for (auto& P:BeardStyleDefinitions) if (P.ID != TEXT("None")) P.ClothingCompatibilityTags={TEXT("Body.Male.Standard")};
    AddParts(EyebrowStyleDefinitions,{TEXT("Natural"),TEXT("Fine"),TEXT("Thick")});
    const TCHAR* IDs[]={TEXT("Jacket"),TEXT("TShirt"),TEXT("Jeans"),TEXT("Sneakers"),TEXT("Cap"),TEXT("Glasses"),TEXT("Backpack")};
    for (int32 I=0; I<7; ++I)
    {
        for (int32 Variant=0; Variant<(I<4 ? 2 : 1); ++Variant)
        {
            FAppearancePartDefinition P; P.ID=FName(*(FString(IDs[I])+(Variant ? TEXT("02") : TEXT("01"))));
            P.DisplayName=FText::FromName(P.ID); P.Slot=static_cast<EClothingSlot>(I);
            for (auto Color:{FLinearColor(.025f,.035f,.05f),FLinearColor(.22f,.27f,.34f),FLinearColor(.38f,.27f,.16f),FLinearColor(.65f,.62f,.56f)})
            { FClothingVariantDefinition V; V.Color=Color; V.DisplayName=FText::AsNumber(P.Variants.Num()+1); P.Variants.Add(V); }
            if (I==0) P.HiddenBodyRegions={TEXT("Torso"),TEXT("Arms")};
            if (I==1) P.HiddenBodyRegions={TEXT("Torso")};
            if (I==2) P.HiddenBodyRegions={TEXT("Legs")};
            ClothingDefinitions.Add(P);
        }
    }
    SkinPalette={FLinearColor(.72f,.49f,.35f),FLinearColor(.58f,.36f,.24f),FLinearColor(.43f,.25f,.15f),FLinearColor(.28f,.14f,.075f),FLinearColor(.13f,.06f,.03f),FLinearColor(.065f,.028f,.014f)};
    HairPalette={FLinearColor(.009f,.006f,.004f),FLinearColor(.025f,.012f,.006f),FLinearColor(.08f,.036f,.014f),FLinearColor(.18f,.095f,.038f),FLinearColor(.27f,.18f,.075f),FLinearColor(.55f,.39f,.16f),FLinearColor(.22f,.055f,.018f),FLinearColor(.36f,.34f,.31f)};
    EyePalette={FLinearColor(.09f,.045f,.017f),FLinearColor(.055f,.14f,.22f),FLinearColor(.08f,.16f,.08f),FLinearColor(.23f,.15f,.04f),FLinearColor(.18f,.2f,.22f)};
    FacePresets={TEXT("Face01"),TEXT("Face02"),TEXT("Face03"),TEXT("Face04")};
    EyeShapes={TEXT("Natural"),TEXT("Almond"),TEXT("Round")};
    for (int32 I=0; I<4; ++I)
    {
        FVoiceProfileDefinition V; V.ID=FName(*FString::Printf(TEXT("Voice%02d"),I+1)); V.DisplayName=FText::FromName(V.ID); VoiceProfiles.Add(V);
        FCharacterAppearanceDefinition A;
        A.PresetID=FName(*FString::Printf(TEXT("Preset%02d"),I+1)); A.Sex=I<2 ? EAppearanceSex::Male : EAppearanceSex::Female;
        A.BodyPreset=Bodies[I/2].ID; A.VisualAge=I==1 ? 32 : 27; A.Height=I<2 ? 180 : 170;
        A.FacePreset=FacePresets[I]; A.HairStyle=I==2 ? TEXT("Medium") : (I==3 ? TEXT("Crop") : TEXT("Short"));
        A.BeardStyle=I==0 ? TEXT("Stubble") : TEXT("None"); A.VoiceProfileID=V.ID;
        for (const auto& M:Morphs) A.FaceMorphs.Add(M.MorphID, M.DefaultValue);
        A.FaceMorphs[TEXT("Face.Width")]=(I-1.5f)*.08f;
        for (int32 S=0; S<4; ++S) { A.Clothing.Add(static_cast<EClothingSlot>(S),FName(*(FString(IDs[S])+TEXT("01")))); A.ClothingVariants.Add(static_cast<EClothingSlot>(S),I%4); }
        Presets.Add(A);
    }
    ApplyModernHeroProfile();
}

void UCharacterAppearanceCatalog::ApplyModernHeroProfile()
{
    if (Presets.IsEmpty())
        return;

    FCharacterAppearanceDefinition Hero = Presets[0];
    Hero.PresetID = TEXT("Preset01");
    Hero.Name = TEXT("Alex");
    Hero.Sex = EAppearanceSex::Male;
    Hero.BodyPreset = TEXT("Male.Standard");
    Hero.BodyBuild = TEXT("Athletic");
    Hero.Height = 182.0f;
    Hero.VisualAge = 27.0f;
    Hero.HairStyle = TEXT("Short");
    Hero.BeardStyle = TEXT("Stubble");
    Hero.EyebrowStyle = TEXT("Natural");
    Hero.SkinRoughness = 0.44f;
    Hero.Freckles = 0.04f;
    Hero.SkinImperfections = 0.12f;
    Hero.EyeShape = TEXT("Almond");

    Hero.FaceMorphs.Add(TEXT("Face.Width"), 0.04f);
    Hero.FaceMorphs.Add(TEXT("Face.Length"), 0.02f);
    Hero.FaceMorphs.Add(TEXT("Jaw.Width"), 0.10f);
    Hero.FaceMorphs.Add(TEXT("Chin.Projection"), 0.04f);
    Hero.FaceMorphs.Add(TEXT("Eyes.Brows"), 0.03f);

    Hero.Clothing.Empty();
    Hero.ClothingVariants.Empty();
    const auto Equip = [&](EClothingSlot Slot, FName ID, int32 Variant)
    {
        if (const auto* Item = Part(ClothingDefinitions, ID))
        {
            if (Item->Slot == Slot)
            {
                Hero.Clothing.Add(Slot, ID);
                Hero.ClothingVariants.Add(Slot, FMath::Clamp(Variant, 0, FMath::Max(0, Item->Variants.Num() - 1)));
            }
        }
    };

    Equip(EClothingSlot::Outerwear, TEXT("Jacket02"), 0);
    Equip(EClothingSlot::Top, TEXT("TShirt02"), 1);
    Equip(EClothingSlot::Bottom, TEXT("Jeans02"), 1);
    Equip(EClothingSlot::Shoes, TEXT("Sneakers02"), 3);
    Equip(EClothingSlot::Accessory, TEXT("Glasses01"), 0);
    Equip(EClothingSlot::Back, TEXT("Backpack01"), 0);

    Presets[0] = Hero;
    FallbackDefinition = Hero;
}
