// Copyright (c) MissiveArts LLC

using UnrealBuildTool;

public class SupertalkEditor : ModuleRules
{
	public SupertalkEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"SlateCore",
				"Slate",
				"Supertalk",
				"DeveloperSettings"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"AssetTools",
				"Engine",
				"MessageLog",
				"EditorStyle"
			});
	}
}