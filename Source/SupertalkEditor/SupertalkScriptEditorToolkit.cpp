// Copyright (c) MissiveArts LLC


#include "SupertalkScriptEditorToolkit.h"
#include "Supertalk/SupertalkPlayer.h"
#include "EditorReimportHandler.h"
#include "IMessageLogListing.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "MessageLogModule.h"
#include "SourceControlOperations.h"
#include "SSupertalkScriptAssetEditor.h"
#include "SupertalkEditorSettings.h"
#include "SupertalkScriptCompiler.h"
#include "SupertalkScriptEditorCommands.h"
#include "UnrealEdGlobals.h"
#include "AutoReimport/AutoReimportManager.h"
#include "Editor/UnrealEdEngine.h"
#include "EditorFramework/AssetImportData.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Misc/FileHelper.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "FSupertalkScriptEditorToolkit"

static const FName SupertalkAssetEditorAppIdentifier("SupertalkAssetEditorApp");
static const FName SupertalkScriptEditorTabId("SupertalkScriptEditor");
static const FName SupertalkCompilerOutputTabId("SupertalkCompilerOutput");

FSupertalkScriptEditorToolkit::~FSupertalkScriptEditorToolkit()
{
	FSupertalkScriptCompiler::OnScriptCompiled.RemoveAll(this);

	FReimportManager::Instance()->OnPreReimport().RemoveAll(this);
	FReimportManager::Instance()->OnPostReimport().RemoveAll(this);
}

void FSupertalkScriptEditorToolkit::Initialize(USupertalkScript* InScriptAsset, const EToolkitMode::Type InMode, const TSharedPtr<IToolkitHost>& InToolkitHost)
{
	ScriptAsset = InScriptAsset;

	{
		FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
		FMessageLogInitializationOptions CompilerLogOptions;
		CompilerLogOptions.bAllowClear = true;
		CompilerLogOptions.bDiscardDuplicates = false;
		CompilerLogOptions.bShowFilters = false;
		CompilerLogOptions.bShowPages = false;
		CompilerLogOptions.bScrollToBottom = false;
		CompilerLogOptions.bShowInLogWindow = false;
		CompilerLogListing = MessageLogModule.CreateLogListing("SupertalkCompiler", CompilerLogOptions);
	}

	FSupertalkScriptEditorCommands::Register();
	BindCommands();

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("Standalone_SupertalkScriptAssetEditor_v3")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
				->SetOrientation(Orient_Vertical)
				->Split
				(
				FTabManager::NewStack()
					->SetSizeCoefficient(0.8)
					->AddTab(SupertalkScriptEditorTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->AddTab(SupertalkCompilerOutputTabId, ETabState::OpenedTab)
				)
		);

	FAssetEditorToolkit::InitAssetEditor(
		InMode,
		InToolkitHost,
		SupertalkAssetEditorAppIdentifier,
		Layout,
		true,
		true,
		InScriptAsset);

	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder)
		{
			ToolbarBuilder.BeginSection("Command");
			{
				ToolbarBuilder.AddToolBarButton(FSupertalkScriptEditorCommands::Get().CompileScript);
				ToolbarBuilder.AddSeparator();
				ToolbarBuilder.AddToolBarButton(FSupertalkScriptEditorCommands::Get().OpenSourceFile);
			}
			ToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar));

	AddToolbarExtender(ToolbarExtender);

	RegenerateMenusAndToolbars();

	FSupertalkScriptCompiler::OnScriptCompiled.AddRaw(this, &FSupertalkScriptEditorToolkit::OnScriptCompiled);
}

FString FSupertalkScriptEditorToolkit::GetDocumentationLink() const
{
	return TEXT("https://github.com/redxdev/Supertalk");
}

void FSupertalkScriptEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_SupertalkScriptAssetEditor", "Supertalk Script Asset Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(SupertalkScriptEditorTabId, FOnSpawnTab::CreateSP(this, &FSupertalkScriptEditorToolkit::HandleTabManagerSpawnTab, SupertalkScriptEditorTabId))
		.SetDisplayName(LOCTEXT("SupertalkScriptEditorTabName", "Supertalk Script Editor"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));

	InTabManager->RegisterTabSpawner(SupertalkCompilerOutputTabId, FOnSpawnTab::CreateSP(this, &FSupertalkScriptEditorToolkit::HandleTabManagerSpawnTab, SupertalkCompilerOutputTabId))
		.SetDisplayName(LOCTEXT("SupertalkCompilerOutputTabName", "Compiler Output"))
		.SetGroup(WorkspaceMenuCategoryRef);
}

void FSupertalkScriptEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(SupertalkScriptEditorTabId);
	InTabManager->UnregisterTabSpawner(SupertalkCompilerOutputTabId);
}

FText FSupertalkScriptEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Supertalk Script Editor");
}

FName FSupertalkScriptEditorToolkit::GetToolkitFName() const
{
	return FName("SupertalkScriptAssetEditor");
}

FLinearColor FSupertalkScriptEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.0f, 0.0f, 0.5f, 0.5f);
}

FString FSupertalkScriptEditorToolkit::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "SupertalkScriptAsset ").ToString();
}

void FSupertalkScriptEditorToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(ScriptAsset);
}

FString FSupertalkScriptEditorToolkit::GetReferencerName() const
{
	return TEXT("FSupertalkScriptEditorToolkit");
}

void FSupertalkScriptEditorToolkit::SaveAsset_Execute()
{
	// TODO: only do this if we receive confirmation
	const USupertalkEditorSettings* Settings = GetDefault<USupertalkEditorSettings>();
	if (Settings->bEnableScriptEditor && Settings->bSaveSourceFilesInScriptEditor)
	{
		if (IsValid(ScriptAsset) && ScriptAsset->bCanCompileFromSource)
		{
			bool bIsNewFile = false;
			FString RelativeFilename;
			if (!IsValid(ScriptAsset->AssetImportData) || ScriptAsset->AssetImportData->SourceData.SourceFiles.IsEmpty())
			{
				RelativeFilename = ScriptAsset->GetName();
				RelativeFilename.RemoveFromStart("ST_");
				RelativeFilename.Append(".sts");
				bIsNewFile = true;
			}
			else
			{
				RelativeFilename = ScriptAsset->AssetImportData->SourceData.SourceFiles[0].RelativeFilename;
			}

			const FString PackagePath = FPackageName::GetLongPackagePath(ScriptAsset->GetPathName()) / TEXT("");
			const FString AbsoluteSrcPath = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(PackagePath));
			const FString SrcFile = AbsoluteSrcPath / RelativeFilename;
			if (SaveToSourceFile(SrcFile))
			{
				if (bIsNewFile)
				{
					if (!IsValid(ScriptAsset->AssetImportData))
					{
						ScriptAsset->AssetImportData = NewObject<UAssetImportData>(ScriptAsset, "AssetImportData");
					}

					ScriptAsset->AssetImportData->SourceData.Insert({ RelativeFilename });
				}

				FAssetImportInfo::FSourceFile& SourceFile = ScriptAsset->AssetImportData->SourceData.SourceFiles[0];
				SourceFile.FileHash = FMD5Hash::HashFile(*SrcFile);
				SourceFile.Timestamp = IFileManager::Get().GetTimeStamp(*SrcFile);

				if (bIsNewFile)
				{
					GUnrealEd->AutoReimportManager->IgnoreNewFile(SrcFile);
				}
				else
				{
					GUnrealEd->AutoReimportManager->IgnoreFileModification(SrcFile);
				}
			}
		}
	}

	FAssetEditorToolkit::SaveAsset_Execute();
}

bool FSupertalkScriptEditorToolkit::SaveToSourceFile(const FString& Path)
{
	ISourceControlModule& SourceControlModule = ISourceControlModule::Get();
	ISourceControlProvider* SourceControlProvider = SourceControlModule.IsEnabled() ? &SourceControlModule.GetProvider() : nullptr;

	TArray<FString> FilesToCheckout { Path };
	bool bNeedsMarkForAdd = false;

	if (SourceControlProvider)
	{
		if (FPaths::FileExists(Path))
		{
			SourceControlProvider->Execute(ISourceControlOperation::Create<FCheckOut>(), FilesToCheckout);
		}
		else
		{
			bNeedsMarkForAdd = true;
		}
	}

	bool bResult = FFileHelper::SaveStringToFile(ScriptAsset->SourceData, *Path);

	if (SourceControlProvider && bNeedsMarkForAdd)
	{
		SourceControlProvider->Execute(ISourceControlOperation::Create<FMarkForAdd>(), FilesToCheckout);
	}

	return bResult;
}

void FSupertalkScriptEditorToolkit::BindCommands()
{
	const FSupertalkScriptEditorCommands& Commands = FSupertalkScriptEditorCommands::Get();
	const TSharedRef<FUICommandList>& UICommandList = GetToolkitCommands();

	UICommandList->MapAction(
		Commands.OpenSourceFile,
		FExecuteAction::CreateUObject(ScriptAsset, &USupertalkScript::OpenSourceFileInExternalProgram),
		FCanExecuteAction::CreateWeakLambda(ScriptAsset, [Script=ScriptAsset]()
		{
			const FString Filename = Script->AssetImportData->GetFirstFilename();
			return !Filename.IsEmpty() && FPaths::FileExists(*Filename);
		}));

	UICommandList->MapAction(
		Commands.CompileScript,
		FExecuteAction::CreateRaw(this, &FSupertalkScriptEditorToolkit::CompileScript),
		FCanExecuteAction::CreateWeakLambda(ScriptAsset, [Script=ScriptAsset]()
		{
			return GetDefault<USupertalkEditorSettings>()->bEnableScriptEditor && Script->bCanCompileFromSource;
		}));
}

TSharedRef<SDockTab> FSupertalkScriptEditorToolkit::HandleTabManagerSpawnTab(const FSpawnTabArgs& Args, FName TabIdentifier)
{
	if (TabIdentifier == SupertalkScriptEditorTabId)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::PanelTab)
			[
				SNew(SSupertalkScriptAssetEditor, ScriptAsset)
				.IsReadOnly(!GetDefault<USupertalkEditorSettings>()->bEnableScriptEditor || !ScriptAsset->bCanCompileFromSource)
			];
	}
	else if (TabIdentifier == SupertalkCompilerOutputTabId)
	{
		FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				MessageLogModule.CreateLogListingWidget(CompilerLogListing.ToSharedRef())
			];
	}

	return SNew(SDockTab);
}

void FSupertalkScriptEditorToolkit::CompileScript()
{
	if (GetDefault<USupertalkEditorSettings>()->bEnableScriptEditor && IsValid(ScriptAsset))
	{
		FSupertalkScriptCompiler::CompileScript(ScriptAsset);
		ScriptAsset->MarkPackageDirty();
	}
}

void FSupertalkScriptEditorToolkit::OnScriptCompiled(USupertalkScript* InScript, bool bResult, const TArray<FBufferedLine>& CompilerOutput)
{
	if (ScriptAsset != InScript || !IsValid(InScript))
	{
		return;
	}

	TArray<TSharedRef<FTokenizedMessage>> Messages;
	Messages.Reserve(CompilerOutput.Num());
	for (const FBufferedLine& Line : CompilerOutput)
	{
		EMessageSeverity::Type MessageSeverity;
		switch (Line.Verbosity)
		{
		default:
			MessageSeverity = EMessageSeverity::Info;
			break;

		case ELogVerbosity::Fatal:
		case ELogVerbosity::Error:
			MessageSeverity = EMessageSeverity::Error;
			break;

		case ELogVerbosity::Warning:
			MessageSeverity = EMessageSeverity::Warning;
			break;
		}

		Messages.Add(FTokenizedMessage::Create(MessageSeverity, FText::FromString(Line.Data)));
	}

	CompilerLogListing->ClearMessages();
	CompilerLogListing->AddMessages(Messages, false);
}


#undef LOCTEXT_NAMESPACE
