#include "Data/CampaignMigrationDefinition.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool ReadVector(const TSharedPtr<FJsonObject> &Object, const TCHAR *Field, FVector &Out)
{
    const TArray<TSharedPtr<FJsonValue>> *Values = nullptr;
    if (!Object.IsValid() || !Object->TryGetArrayField(Field, Values) || !Values || Values->Num() != 3)
        return false;
    double X = 0, Y = 0, Z = 0;
    if (!(*Values)[0]->TryGetNumber(X) || !(*Values)[1]->TryGetNumber(Y) || !(*Values)[2]->TryGetNumber(Z) ||
        !FMath::IsFinite(X) || !FMath::IsFinite(Y) || !FMath::IsFinite(Z))
        return false;
    Out = FVector(X, Y, Z);
    return true;
}
}

bool UCampaignMigrationDefinition::ImportJson(const FString &JsonText)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
        return false;

    double Version = 0;
    const TArray<TSharedPtr<FJsonValue>> *ZoneValues = nullptr;
    if (!Root->TryGetNumberField(TEXT("schema_version"), Version) || Version != 1 ||
        !Root->TryGetArrayField(TEXT("zones"), ZoneValues) || !ZoneValues || ZoneValues->IsEmpty())
        return false;

    TArray<FCampaignMigrationZone> Parsed;
    for (const TSharedPtr<FJsonValue> &Value : *ZoneValues)
    {
        const TSharedPtr<FJsonObject> ZoneObject = Value.IsValid() ? Value->AsObject() : nullptr;
        if (!ZoneObject.IsValid())
            return false;

        FString Id;
        double MinX = 0, MaxX = 0, Yaw = 0;
        FVector SourceAnchor, TargetAnchor;
        if (!ZoneObject->TryGetStringField(TEXT("id"), Id) ||
            !ZoneObject->TryGetNumberField(TEXT("source_min_x"), MinX) ||
            !ZoneObject->TryGetNumberField(TEXT("source_max_x"), MaxX) ||
            !ZoneObject->TryGetNumberField(TEXT("target_yaw"), Yaw) ||
            !ReadVector(ZoneObject, TEXT("source_anchor"), SourceAnchor) ||
            !ReadVector(ZoneObject, TEXT("target_anchor"), TargetAnchor) ||
            !FMath::IsFinite(MinX) || !FMath::IsFinite(MaxX) || !FMath::IsFinite(Yaw) || MinX >= MaxX)
            return false;

        FCampaignMigrationZone Zone;
        Zone.Id = FName(*Id);
        Zone.SourceMinX = MinX;
        Zone.SourceMaxX = MaxX;
        Zone.SourceAnchor = SourceAnchor;
        Zone.TargetAnchor = TargetAnchor;
        Zone.TargetYaw = Yaw;
        ZoneObject->TryGetStringField(TEXT("building_id"), Zone.BuildingId);
        Parsed.Add(MoveTemp(Zone));
    }

    Parsed.Sort([](const FCampaignMigrationZone &A, const FCampaignMigrationZone &B) {
        return A.SourceMinX < B.SourceMinX;
    });
    for (int32 I = 1; I < Parsed.Num(); ++I)
        if (!FMath::IsNearlyEqual(Parsed[I - 1].SourceMaxX, Parsed[I].SourceMinX, 0.1f))
            return false;

    Zones = MoveTemp(Parsed);
    MarkPackageDirty();
    return true;
}

bool UCampaignMigrationDefinition::TransformLegacyPosition(const FVector &Source, FVector &Target) const
{
    if (Source.ContainsNaN())
        return false;
    for (const FCampaignMigrationZone &Zone : Zones)
    {
        if (Source.X < Zone.SourceMinX || Source.X >= Zone.SourceMaxX)
            continue;
        const FVector Local = Source - Zone.SourceAnchor;
        const FVector Rotated = FRotator(0, Zone.TargetYaw, 0).RotateVector(Local);
        Target = Zone.TargetAnchor + Rotated;
        return !Target.ContainsNaN();
    }
    return false;
}

bool UCampaignMigrationDefinition::TransformLegacyTransform(const FTransform &Source, FTransform &Target) const
{
    FVector Location;
    if (Source.ContainsNaN() || !TransformLegacyPosition(Source.GetLocation(), Location))
        return false;

    const FCampaignMigrationZone *Matched = Zones.FindByPredicate([&](const FCampaignMigrationZone &Zone) {
        return Source.GetLocation().X >= Zone.SourceMinX && Source.GetLocation().X < Zone.SourceMaxX;
    });
    if (!Matched)
        return false;

    Target = Source;
    Target.SetLocation(Location);
    const FQuat YawRotation(FVector::UpVector, FMath::DegreesToRadians(Matched->TargetYaw));
    Target.SetRotation(YawRotation * Source.GetRotation());
    return !Target.ContainsNaN();
}
