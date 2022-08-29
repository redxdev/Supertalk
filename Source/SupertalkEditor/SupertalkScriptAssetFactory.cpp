﻿// Copyright (c) MissiveArts LLC

#include "SupertalkScriptAssetFactory.h"
#include "IMessageLogListing.h"
#include "MessageLogModule.h"
#include "SupertalkParser.h"
#include "SupertalkScriptEditorToolkit.h"
#include "EditorFramework/AssetImportData.h"
#include "Supertalk/SupertalkPlayer.h"
#include "Supertalk/Supertalk.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "SupertalkScriptAssetFactory"

USupertalkScriptAssetFactory::USupertalkScriptAssetFactory()
{
	SupportedClass = USupertalkScript::StaticClass();
	bCreateNew = false;
	bEditAfterNew = false;
	bEditorImport = true;
	bText = true;

	Formats.Add(TEXT("sts;Supertalk Script"));
}

FText USupertalkScriptAssetFactory::GetDisplayName() const
{
	return LOCTEXT("FactoryDescription", "Supertalk Script");
}

bool USupertalkScriptAssetFactory::FactoryCanImport(const FString& Filename)
{
	const FString Extension = FPaths::GetExtension(Filename);

	return Extension == TEXT("sts");
}

UObject* USupertalkScriptAssetFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer,
	const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, Type);

	const FString FileContent(BufferEnd - Buffer, Buffer);

	USupertalkScript* Script = NewObject<USupertalkScript>(InParent, InName, Flags | RF_Transactional);

	FModuleManager::GetModuleChecked<FMessageLogModule>("MessageLog").GetLogListing(SupertalkMessageLogName)->ClearMessages();
	FMessageLog MessageLog(SupertalkMessageLogName);
	
	TSharedRef<FSupertalkParser> Parser = FSupertalkParser::Create(&MessageLog);

	if (!Parser->Parse(GetCurrentFilename(), FileContent, Script))
	{
		return nullptr;
	}

	Script->SourceData = FileContent;
	Script->AssetImportData->Update(GetCurrentFilename());

	// For some reason the min severity needs to be one level below what we actually want.
	MessageLog.Notify(LOCTEXT("SupertalkCompilerErrorsReported", "Errors were reported by the Supertalk compiler"));

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, Script);

	return Script;
}

bool USupertalkScriptAssetFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	if (USupertalkScript* Script = Cast<USupertalkScript>(Obj))
	{
		if (Script && Script->AssetImportData)
		{
			Script->AssetImportData->ExtractFilenames(OutFilenames);
			return true;
		}
	}

	return false;
}

void USupertalkScriptAssetFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	if (USupertalkScript* Script = Cast<USupertalkScript>(Obj))
	{
		if (ensure(NewReimportPaths.Num() == 1))
		{
			Script->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
		}
	}
}

EReimportResult::Type USupertalkScriptAssetFactory::Reimport(UObject* Obj)
{
	USupertalkScript* Script = Cast<USupertalkScript>(Obj);
	if (!Script)
	{
		return EReimportResult::Failed;
	}

	const FString Filename = Script->AssetImportData->GetFirstFilename();
	if (Filename.IsEmpty() || !FPaths::FileExists(*Filename))
	{
		return EReimportResult::Failed;
	}

	bool OutCancelled = false;
	if (ImportObject(Script->GetClass(), Script->GetOuter(), Script->GetFName(), Script->GetFlags(), Filename, nullptr, OutCancelled) != nullptr)
	{
		UE_LOG(LogSupertalk, Log, TEXT("Imported successfully"));

		Script->AssetImportData->Update(Filename);
		
		// Try to find the outer package so we can dirty it up
		if (Script->GetOuter())
		{
			Script->GetOuter()->MarkPackageDirty();
		}
		
		Script->MarkPackageDirty();
		
		return EReimportResult::Succeeded;
	}
	else
	{
		if (OutCancelled)
		{
			UE_LOG(LogSupertalk, Warning, TEXT("-- import canceled"));
			return EReimportResult::Cancelled;
		}
		else
		{
			UE_LOG(LogSupertalk, Warning, TEXT("-- import failed"));
			return EReimportResult::Failed;
		}
	}
}

FText FAssetTypeActions_SupertalkScript::GetName() const
{
	return LOCTEXT("SupertalkScriptAssetName", "Supertalk Script");
}

UClass* FAssetTypeActions_SupertalkScript::GetSupportedClass() const
{
	return USupertalkScript::StaticClass();
}

FColor FAssetTypeActions_SupertalkScript::GetTypeColor() const
{
	return FColor::Cyan;
}

uint32 FAssetTypeActions_SupertalkScript::GetCategories()
{
	return EAssetTypeCategories::Misc | EAssetTypeCategories::Blueprint;
}

bool FAssetTypeActions_SupertalkScript::IsImportedAsset() const
{
	return true;
}

void FAssetTypeActions_SupertalkScript::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
{
	for (const auto& Asset : TypeAssets)
	{
		const auto Script = CastChecked<USupertalkScript>(Asset);
		if (Script->AssetImportData)
		{
			Script->AssetImportData->ExtractFilenames(OutSourceFilePaths);
		}
	}
}

void FAssetTypeActions_SupertalkScript::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid()
		? EToolkitMode::WorldCentric
		: EToolkitMode::Standalone;

	for (UObject* Obj : InObjects)
	{
		USupertalkScript* ScriptAsset = Cast<USupertalkScript>(Obj);
		if (ScriptAsset)
		{
			TSharedRef<FSupertalkScriptEditorToolkit> EditorToolkit = MakeShareable(new FSupertalkScriptEditorToolkit());
			EditorToolkit->Initialize(ScriptAsset, Mode, EditWithinLevelEditor);
		}
	}
}

#undef LOCTEXT_NAMESPACE
