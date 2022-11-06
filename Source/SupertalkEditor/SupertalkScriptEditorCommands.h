// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
#include "EditorStyleSet.h"

class FSupertalkScriptEditorCommands : public TCommands<FSupertalkScriptEditorCommands>
{
public:
	FSupertalkScriptEditorCommands()
		: TCommands<FSupertalkScriptEditorCommands>(
			TEXT("SupertalkScriptEditor"),
			NSLOCTEXT("Contexts", "SupertalkScriptEditor", "Supertalk Script Editor"),
			NAME_None,
			FAppStyle::GetAppStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> OpenInExternalEditor;
	TSharedPtr<FUICommandInfo> CompileScript;
};
