// Copyright (c) MissiveArts LLC


#include "SupertalkScriptEditorCommands.h"

#define LOCTEXT_NAMESPACE "SupertalkScriptEditorCommands"

void FSupertalkScriptEditorCommands::RegisterCommands()
{
	UI_COMMAND(OpenInExternalEditor, "Open in External Editor", "Opens the source file in an external editor", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE