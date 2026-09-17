using UnrealBuildTool;
public class WroclawTheGameEditorTarget : TargetRules {
 public WroclawTheGameEditorTarget(TargetInfo Target) : base(Target) {
  Type = TargetType.Editor; DefaultBuildSettings = BuildSettingsVersion.V7;
  IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
  ExtraModuleNames.Add("WroclawTheGame");
 }
}
