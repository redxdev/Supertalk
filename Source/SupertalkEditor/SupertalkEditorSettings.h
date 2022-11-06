// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
#include "SupertalkEditorSettings.generated.h"

UCLASS(Config=Editor, DefaultConfig, meta = (DisplayName = "Supertalk Editor"))
class SUPERTALKEDITOR_API USupertalkEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	// If enabled, supertalk scripts can be created and modified within the editor.
	// Changing this option may require a restart.
	// This is incredibly experimental - the text editor has not been well tested and is liable to delete text at random!
	UPROPERTY(EditAnywhere, Config, Category = Experimental)
	bool bEnableScriptEditor = false;
};
