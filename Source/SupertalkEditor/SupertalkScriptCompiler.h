// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"

class FSupertalkScriptCompiler
{
public:
	static void Initialize();
	static void Shutdown();

private:
	FSupertalkScriptCompiler() {}

	static void OnScriptPreSave(class USupertalkScript* Script);
};