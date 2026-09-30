// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SlateLivePreview : ModuleRules
{
	public SlateLivePreview(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Slate",
				"SlateCore",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"ToolMenus",
				"WorkspaceMenuStructure",
				"DesktopPlatform",
				"ImageWrapper",
				"RenderCore",
				"ApplicationCore",
			}
		);

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PrivateDependencyModuleNames.Add("LiveCoding");
		}
	}
}
