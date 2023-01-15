// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
#include "SupertalkEditorSettings.generated.h"

UCLASS(Config=Editor, DefaultConfig, meta = (DisplayName = "Supertalk Editor"))
class SUPERTALKEDITOR_API USupertalkEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USupertalkEditorSettings();

	// If enabled, supertalk scripts can be created and modified within the editor.
	// Changing this option may require a restart.
	// This is incredibly experimental - the text editor has not been well tested and is liable to delete text at random!
	UPROPERTY(EditAnywhere, Config, Category = Experimental)
	uint8 bEnableScriptEditor : 1;

	// If enabled, saving a script in the script editor will also save the script asset's source file.
	// This will attempt to checkout the source file in source control if necessary.
	UPROPERTY(EditAnywhere, Config, Category = Experimental, meta = (EditCondition = "bEnableScriptEditor"))
	uint8 bSaveSourceFilesInScriptEditor : 1;
};
