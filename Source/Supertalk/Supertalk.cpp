// Copyright (c) MissiveArts LLC

#include "Supertalk.h"
#include "MessageLogModule.h"

DEFINE_LOG_CATEGORY(LogSupertalk)

const FName SupertalkMessageLogName = FName(TEXT("SupertalkMessageLog"));

IMPLEMENT_MODULE(FSupertalkModule, Supertalk);

void FSupertalkModule::StartupModule()
{
	UE_LOG(LogSupertalk, Log, TEXT("Supertalk startup"));

	if (FModuleManager::Get().IsModuleLoaded("MessageLog"))
	{
		FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");

		FMessageLogInitializationOptions Options;
		Options.bAllowClear = true;
		Options.bShowInLogWindow = true;
		MessageLogModule.RegisterLogListing(SupertalkMessageLogName, NSLOCTEXT("Supertalk", "SupertalkMessageLog", "Supertalk"), Options);
	}
}

void FSupertalkModule::ShutdownModule()
{
	UE_LOG(LogSupertalk, Log, TEXT("Supertalk shutdown"));

	if (FModuleManager::Get().IsModuleLoaded("MessageLog"))
	{
		FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
		MessageLogModule.UnregisterLogListing(SupertalkMessageLogName);
	}
}

