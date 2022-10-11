// Copyright (c) MissiveArts LLC


#include "SupertalkScriptEditorToolkit.h"
#include "Supertalk/SupertalkPlayer.h"
#include "EditorReimportHandler.h"
#include "SSupertalkScriptAssetEditor.h"
#include "SupertalkScriptEditorCommands.h"

#define LOCTEXT_NAMESPACE "FSupertalkScriptEditorToolkit"

static const FName SupertalkAssetEditorAppIdentifier("SupertalkAssetEditorApp");
static const FName SupertalkScriptEditorTabId("SupertalkScriptEditor");

FSupertalkScriptEditorToolkit::~FSupertalkScriptEditorToolkit()
{
	FReimportManager::Instance()->OnPreReimport().RemoveAll(this);
	FReimportManager::Instance()->OnPostReimport().RemoveAll(this);
}

void FSupertalkScriptEditorToolkit::Initialize(USupertalkScript* InScriptAsset, const EToolkitMode::Type InMode, const TSharedPtr<IToolkitHost>& InToolkitHost)
{
	ScriptAsset = InScriptAsset;

	FSupertalkScriptEditorCommands::Register();
	BindCommands();

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("Standalone_SupertalkScriptAssetEditor_v2")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
				->SetOrientation(Orient_Vertical)
				->Split
				(
				FTabManager::NewStack()
					->AddTab(SupertalkScriptEditorTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
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
				ToolbarBuilder.AddToolBarButton(FSupertalkScriptEditorCommands::Get().OpenInExternalEditor);
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
}

void FSupertalkScriptEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(SupertalkScriptEditorTabId);
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

void FSupertalkScriptEditorToolkit::BindCommands()
{
	const FSupertalkScriptEditorCommands& Commands = FSupertalkScriptEditorCommands::Get();
	const TSharedRef<FUICommandList>& UICommandList = GetToolkitCommands();

	UICommandList->MapAction(
		Commands.OpenInExternalEditor,
		FExecuteAction::CreateUObject(ScriptAsset, &USupertalkScript::OpenSourceFileInExternalProgram),
		FCanExecuteAction());
}

TSharedRef<SDockTab> FSupertalkScriptEditorToolkit::HandleTabManagerSpawnTab(const FSpawnTabArgs& Args, FName TabIdentifier)
{
	TSharedPtr<SWidget> TabWidget = SNullWidget::NullWidget;
	if (TabIdentifier == SupertalkScriptEditorTabId)
	{
		TabWidget = SNew(SSupertalkScriptAssetEditor, ScriptAsset);
	}

	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		[
			TabWidget.ToSharedRef()
		];
}

#undef LOCTEXT_NAMESPACE
