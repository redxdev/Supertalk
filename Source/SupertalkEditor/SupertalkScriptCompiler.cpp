// Copyright (c) MissiveArts LLC

#include "SupertalkScriptCompiler.h"

#include "SupertalkParser.h"
#include "Supertalk/Supertalk.h"
#include "Supertalk/SupertalkPlayer.h"

void FSupertalkScriptCompiler::Initialize()
{
	USupertalkScript::OnScriptPreSave.BindStatic(&FSupertalkScriptCompiler::OnScriptPreSave);
}

void FSupertalkScriptCompiler::Shutdown()
{
	USupertalkScript::OnScriptPreSave.Unbind();
}

void FSupertalkScriptCompiler::OnScriptPreSave(USupertalkScript* Script)
{
	check(Script);
	if (!Script->bCanCompileFromSource)
	{
		UE_LOG(LogSupertalk, Warning, TEXT("Script '%s' cannot be compiled - please reimport or recreate to enable compilation. This will not affect the ability to run the script."), *Script->GetName());
		return;
	}

	if (FSupertalkParser::ParseIntoScript(Script->GetName(), Script->SourceData, Script, true))
	{
		UE_LOG(LogSupertalk, Log, TEXT("Successfully compiled script '%s' (stats: %d sections)"), *Script->GetName(), Script->Sections.Num());
	}
	else
	{
		UE_LOG(LogSupertalk, Error, TEXT("Failed to compile script '%s', check message log for details"), *Script->GetName());
	}
}
