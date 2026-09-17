using UnrealBuildTool;
public class WroclawTheGame : ModuleRules {
 public WroclawTheGame(ReadOnlyTargetRules Target) : base(Target) {
  PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
  PrivateIncludePaths.Add(ModuleDirectory);
  CppStandard = CppStandardVersion.Cpp20;
  PublicDependencyModuleNames.AddRange(new string[] {"Core","CoreUObject","Engine","InputCore","EnhancedInput","AIModule","NavigationSystem","GameplayTasks","GameplayTags","Json","JsonUtilities","GeoReferencing"});
  if (Target.bBuildEditor) PrivateDependencyModuleNames.AddRange(new string[] {"MeshDescription","StaticMeshDescription","AssetRegistry"});
 }
}
