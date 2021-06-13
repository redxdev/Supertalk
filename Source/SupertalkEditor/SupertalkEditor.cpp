// Copyright (c) MissiveArts LLC

#include "SupertalkEditor.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Supertalk/Supertalk.h"
#include "SupertalkScriptAssetFactory.h"


IMPLEMENT_MODULE(FSupertalkEditorModule, SupertalkEditor);

void FSupertalkEditorModule::StartupModule()
{
	UE_LOG(LogSupertalk, Log, TEXT("Supertalk editor startup"));
	
	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_SupertalkScript()));
}

void FSupertalkEditorModule::ShutdownModule()
{
	UE_LOG(LogSupertalk, Log, TEXT("Supertalk editor shutdown"));
}

