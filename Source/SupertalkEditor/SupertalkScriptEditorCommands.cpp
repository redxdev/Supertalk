// Copyright (c) MissiveArts LLC


#include "SupertalkScriptEditorCommands.h"

#define LOCTEXT_NAMESPACE "SupertalkScriptEditorCommands"

void FSupertalkScriptEditorCommands::RegisterCommands()
{
	UI_COMMAND(OpenInExternalEditor, "Open in External Editor", "Opens the source file in an external editor", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(CompileScript, "Compile Script", "Compiles the supertalk script", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE