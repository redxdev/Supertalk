// Copyright (c) MissiveArts LLC

#include "SupertalkScriptAssetFactory.h"
#include "IMessageLogListing.h"
#include "MessageLogModule.h"
#include "SupertalkEditorSettings.h"
#include "SupertalkParser.h"
#include "SupertalkScriptEditorToolkit.h"
#include "EditorFramework/AssetImportData.h"
#include "Supertalk/SupertalkPlayer.h"
#include "Supertalk/Supertalk.h"

#define LOCTEXT_NAMESPACE "SupertalkScriptAssetFactory"

USupertalkScriptAssetFactory::USupertalkScriptAssetFactory()
{
	SupportedClass = USupertalkScript::StaticClass();
	bEditAfterNew = true;
	bEditorImport = true;
	bText = true;

	bCreateNew = GetDefault<USupertalkEditorSettings>()->bEnableScriptEditor;

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

UObject* USupertalkScriptAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	return NewObject<USupertalkScript>(InParent, SupportedClass, InName, Flags | RF_Transactional);
}

UObject* USupertalkScriptAssetFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer,
                                                         const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, Type);

	const FString FileContent(BufferEnd - Buffer, Buffer);

	USupertalkScript* Script = NewObject<USupertalkScript>(InParent, InName, Flags | RF_Transactional);

	FBufferedOutputDevice Output;
	bool bResult = FSupertalkParser::ParseIntoScript(GetCurrentFilename(), FileContent, Script, IsRunningCommandlet() ? static_cast<FOutputDevice*>(GLog) : &Output);

	if (!IsRunningCommandlet())
	{
		FModuleManager::GetModuleChecked<FMessageLogModule>("MessageLog")
			.GetLogListing(SupertalkMessageLogName)->NewPage(
				FText::Format(
					LOCTEXT("MessageLogPageName", "Compile of {0} at {1}"),
					FText::FromString(GetCurrentFilename()),
					FText::AsDateTime(FDateTime::Now())));

		FMessageLog MessageLog(SupertalkMessageLogName);
		TArray<FBufferedLine> Lines;
		Output.GetContents(Lines);
		for (const FBufferedLine& Line : Lines)
		{
			FText Message = FText::FromString(Line.Data);
			switch (Line.Verbosity)
			{
			default:
				MessageLog.Info(Message);
				break;

			case ELogVerbosity::Fatal:
			case ELogVerbosity::Error:
				MessageLog.Error(Message);
				break;

			case ELogVerbosity::Warning:
				MessageLog.Warning(Message);
				break;
			}
		}

		MessageLog.Notify(LOCTEXT("SupertalkCompilerErrorsReported", "Errors were reported by the Supertalk compiler"));
	}

	if (!bResult)
	{
		return nullptr;
	}

	Script->SourceData = FileContent;
	Script->bCanCompileFromSource = true;
	Script->AssetImportData->Update(GetCurrentFilename());

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
	if (FactoryCreateFile(Script->GetClass(), Script->GetOuter(), Script->GetFName(), Script->GetFlags(), Filename, nullptr, GWarn, OutCancelled) != nullptr)
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
	return EAssetTypeCategories::Misc;
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

USupertalkScriptExporter::USupertalkScriptExporter()
{
	SupportedClass = USupertalkScript::StaticClass();
	bText = true;
	FormatExtension.Add(TEXT("sts"));
	FormatDescription.Add(TEXT("Supertalk Script"));
	PreferredFormatIndex = 0;
}

bool USupertalkScriptExporter::ExportText(const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, uint32 PortFlags)
{
	USupertalkScript* Script = CastChecked<USupertalkScript>(Object);
	Ar.Log(Script->SourceData);
	return true;
}

#undef LOCTEXT_NAMESPACE
