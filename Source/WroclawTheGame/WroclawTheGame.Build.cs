using UnrealBuildTool;
public class WroclawTheGame : ModuleRules {
 public WroclawTheGame(ReadOnlyTargetRules Target) : base(Target) {
  PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
  PrivateIncludePaths.Add(ModuleDirectory);
  PublicDependencyModuleNames.AddRange(new string[] {"Core","CoreUObject","Engine","InputCore","EnhancedInput","AIModule","NavigationSystem","GameplayTasks"});
 }
}
