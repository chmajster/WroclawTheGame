#include "Geography/GeoReferenceLibrary.h"
#include "GeoReferencingSystem.h"
#include "GeographicCoordinates.h"
FVector UGeoReferenceLibrary::ProjectedToWorld(AGeoReferencingSystem *System, FVector Projected)
{
    FVector Result = FVector::ZeroVector;
    if (System)
        System->ProjectedToEngine(Projected, Result);
    return Result;
}
FVector UGeoReferenceLibrary::GeoToWorld(AGeoReferencingSystem *System, FVector Geo)
{
    if (!System)
        return FVector::ZeroVector;
    FGeographicCoordinates Coordinates;
    Coordinates.Longitude = Geo.X;
    Coordinates.Latitude = Geo.Y;
    Coordinates.Altitude = 0;
    FVector Projected;
    System->GeographicToProjected(Coordinates, Projected);
    Projected.Z = Geo.Z;
    return ProjectedToWorld(System, Projected);
}
FVector UGeoReferenceLibrary::LatLonToWorld(AGeoReferencingSystem *System, double Latitude, double Longitude,
                                            double Height)
{
    return GeoToWorld(System, FVector(Longitude, Latitude, Height));
}
FVector UGeoReferenceLibrary::WorldToGeo(AGeoReferencingSystem *System, FVector World)
{
    if (!System)
        return FVector::ZeroVector;
    FVector Projected;
    System->EngineToProjected(World, Projected);
    const double Height = Projected.Z;
    Projected.Z = 0;
    FGeographicCoordinates Coordinates;
    System->ProjectedToGeographic(Projected, Coordinates);
    return FVector(Coordinates.Longitude, Coordinates.Latitude, Height);
}
