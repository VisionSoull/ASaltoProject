// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class ASaltoProject : ModuleRules
{
	public ASaltoProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore",
			"EnhancedInput",
			"NavigationSystem",
   			"AIModule",
            "GameplayTags",
            "UMG",
            "Slate",
			"SlateCore",
			//Camera
			"CinematicCamera",
			// Interaction
			"DeveloperSettings",
			// Animation
			"MotionWarping",
			"AnimGraphRuntime"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });
		
		// Editor-only dependencies
		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(new string[] {
				"UnrealEd",
				"EditorStyle",
				"EditorWidgets",
				"ToolMenus"
			});

			PrivateDependencyModuleNames.AddRange(new string[] {
				"EditorSubsystem",
				"UnrealEd"
			});
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
