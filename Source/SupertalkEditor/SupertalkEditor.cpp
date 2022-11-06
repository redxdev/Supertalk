// Copyright (c) MissiveArts LLC

#include "SupertalkEditor.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "SupertalkEditorStyle.h"
#include "SupertalkScriptAssetFactory.h"
#include "SupertalkScriptCompiler.h"

IMPLEMENT_MODULE(FSupertalkEditorModule, SupertalkEditor);

void FSupertalkEditorModule::StartupModule()
{
	FSupertalkEditorStyle::Initialize();

	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_SupertalkScript()));

	FSupertalkScriptCompiler::Initialize();
}

void FSupertalkEditorModule::ShutdownModule()
{
	FSupertalkScriptCompiler::Shutdown();

	FSupertalkEditorStyle::Shutdown();
}

