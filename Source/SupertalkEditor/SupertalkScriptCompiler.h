// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"

// OnScriptCompiled(Script, bSuccess, CompilerOutput)
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSupertalkScriptCompiled, class USupertalkScript*, bool, const TArray<FBufferedLine>&);

class FSupertalkScriptCompiler
{
public:
	static void Initialize();
	static void Shutdown();

	static bool CompileScript(class USupertalkScript* Script);

	static FOnSupertalkScriptCompiled OnScriptCompiled;

private:
	FSupertalkScriptCompiler() {}

	static void OnScriptPreSave(class USupertalkScript* Script);
};