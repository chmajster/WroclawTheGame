using UnrealBuildTool;
public class WroclawTheGameEditorTarget : TargetRules {
 public WroclawTheGameEditorTarget(TargetInfo Target) : base(Target) {
  Type = TargetType.Editor; DefaultBuildSettings = BuildSettingsVersion.V5;
  IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
  ExtraModuleNames.Add("WroclawTheGame");
 }
}
