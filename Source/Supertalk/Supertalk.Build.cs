// Copyright (c) MissiveArts LLC

using UnrealBuildTool;

public class Supertalk : ModuleRules
{
	public Supertalk(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"MessageLog",
			});
	}
}