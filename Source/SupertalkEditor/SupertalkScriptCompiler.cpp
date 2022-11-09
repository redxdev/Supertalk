// Copyright (c) MissiveArts LLC

#include "SupertalkScriptCompiler.h"

#include "SupertalkParser.h"
#include "Supertalk/Supertalk.h"
#include "Supertalk/SupertalkPlayer.h"

FOnSupertalkScriptCompiled FSupertalkScriptCompiler::OnScriptCompiled;

void FSupertalkScriptCompiler::Initialize()
{
	USupertalkScript::OnScriptPreSave.BindStatic(&FSupertalkScriptCompiler::OnScriptPreSave);
}

void FSupertalkScriptCompiler::Shutdown()
{
	USupertalkScript::OnScriptPreSave.Unbind();
}

bool FSupertalkScriptCompiler::CompileScript(USupertalkScript* Script)
{
	if (!IsValid(Script))
	{
		UE_LOG(LogSupertalk, Error, TEXT("Cannot compile invalid script"));
		return false;
	}

	if (!Script->bCanCompileFromSource)
	{
		UE_LOG(LogSupertalk, Warning, TEXT("Script '%s' cannot be compiled - please reimport or recreate to enable compilation. This will not affect the ability to run the script."), *Script->GetName());
		return false;
	}

	FBufferedOutputDevice Buffer;

	FOutputDeviceRedirector Output;
	Output.AddOutputDevice(GLog);
	Output.AddOutputDevice(&Buffer);

	bool bResult = FSupertalkParser::ParseIntoScript(Script->GetName(), Script->SourceData, Script, &Output);

	TArray<FBufferedLine> CompilerOutput;
	Buffer.GetContents(CompilerOutput);

	if (bResult)
	{
		UE_LOG(LogSupertalk, Log, TEXT("Successfully compiled script '%s' (stats: %d sections)"), *Script->GetName(), Script->Sections.Num());
		OnScriptCompiled.Broadcast(Script, true, CompilerOutput);
	}
	else
	{
		UE_LOG(LogSupertalk, Error, TEXT("Failed to compile script '%s', check log for details"), *Script->GetName());
		OnScriptCompiled.Broadcast(Script, false, CompilerOutput);
	}

	return bResult;
}

void FSupertalkScriptCompiler::OnScriptPreSave(USupertalkScript* Script)
{
	check(Script);
	CompileScript(Script);
}
