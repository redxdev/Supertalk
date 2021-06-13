// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"

SUPERTALK_API DECLARE_LOG_CATEGORY_EXTERN(LogSupertalk, Log, All);

extern SUPERTALK_API const FName SupertalkMessageLogName;

SUPERTALK_API class FSupertalkModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};