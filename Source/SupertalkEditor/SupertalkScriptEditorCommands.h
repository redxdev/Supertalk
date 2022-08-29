// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"

class FSupertalkScriptEditorCommands : public TCommands<FSupertalkScriptEditorCommands>
{
public:
	FSupertalkScriptEditorCommands()
		: TCommands<FSupertalkScriptEditorCommands>(
			TEXT("SupertalkScriptEditor"),
			NSLOCTEXT("Contexts", "SupertalkScriptEditor", "Supertalk Script Editor"),
			NAME_None,
			FEditorStyle::GetStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> OpenInExternalEditor;
};
