using UnrealBuildTool;

public class ProjectD : ModuleRules
{
	public ProjectD(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"GameplayAbilities", "GameplayTags", "GameplayTasks",
			"AIModule", "NavigationSystem",
			"StateTreeModule", "GameplayStateTreeModule",
			"UMG", "Slate", "SlateCore", "Niagara", "CommonUI", "CommonInput",
			"PhysicsCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
