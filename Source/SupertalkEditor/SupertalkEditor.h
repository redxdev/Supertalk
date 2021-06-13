// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"

SUPERTALK_API class FSupertalkEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};