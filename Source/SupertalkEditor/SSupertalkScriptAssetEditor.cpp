// Copyright (c) MissiveArts LLC


#include "SSupertalkScriptAssetEditor.h"
#include "Supertalk/SupertalkPlayer.h"

SSupertalkScriptAssetEditor::~SSupertalkScriptAssetEditor()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
}

void SSupertalkScriptAssetEditor::Construct(const FArguments& InArgs, class USupertalkScript* InScriptAsset)
{
	check(InScriptAsset);
	ScriptAsset = InScriptAsset;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(TextBox, SMultiLineEditableTextBox)
					.IsReadOnly(true)
					.Text(FText::FromString(ScriptAsset->SourceData))
			]
	];

	FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &SSupertalkScriptAssetEditor::HandleScriptAssetPropertyChanged);
}

void SSupertalkScriptAssetEditor::HandleScriptAssetPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	if (Object == ScriptAsset)
	{
		TextBox->SetText(FText::FromString(ScriptAsset->SourceData));
	}
}
