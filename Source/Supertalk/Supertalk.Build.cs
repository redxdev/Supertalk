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

		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"MessageLog",
				});
		}
	}
}