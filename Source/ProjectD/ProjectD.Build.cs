using UnrealBuildTool;

public class ProjectD : ModuleRules
{
	public ProjectD(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "CoreOnline", "Engine", "InputCore", "EnhancedInput",
			"GameplayAbilities", "GameplayTags", "GameplayTasks",
			"AIModule", "NavigationSystem",
			"StateTreeModule", "GameplayStateTreeModule",
			"UMG", "Slate", "SlateCore", "Niagara", "CommonUI", "CommonInput",
			"PhysicsCore",
			"PreLoadScreen", "DeveloperSettings",
			"GeometryCollectionEngine", "ChaosSolverEngine",
			"OnlineSubsystem", "OnlineSubsystemUtils", "OnlineSubsystemSteam", "SteamSockets"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
