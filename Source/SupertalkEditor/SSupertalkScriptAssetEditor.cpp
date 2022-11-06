// Copyright (c) MissiveArts LLC


#include "SSupertalkScriptAssetEditor.h"
#include "SupertalkEditorStyle.h"
#include "SupertalkParser.h"
#include "SupertalkRichTextSyntaxHighlighterTextLayoutMarshaller.h"
#include "Supertalk/SupertalkPlayer.h"

SSupertalkScriptAssetEditor::SSupertalkScriptAssetEditor()
	: MessageLog(TEXT("Supertalk Editor"))
{
}

SSupertalkScriptAssetEditor::~SSupertalkScriptAssetEditor()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
}

void SSupertalkScriptAssetEditor::Construct(const FArguments& InArgs, class USupertalkScript* InScriptAsset)
{
	check(InScriptAsset);
	ScriptAsset = InScriptAsset;

	TSharedRef<FSupertalkParser> Parser = FSupertalkParser::Create(&MessageLog);
	TSharedRef<FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller> RichTextMarshaller = FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller::Create(
		Parser,
		FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller::FSyntaxTextStyle());

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(TextBox, SMultiLineEditableTextBox)
					.IsReadOnly(InArgs._IsReadOnly)
					.Text(FText::FromString(ScriptAsset->SourceData))
					.Font(FSupertalkEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("TextEditor.NormalText").Font)
					.Style(FSupertalkEditorStyle::Get(), "TextEditor.EditableTextBox")
					.Marshaller(RichTextMarshaller)
					.AutoWrapText(false)
					.OnTextChanged(this, &SSupertalkScriptAssetEditor::OnTextChanged)
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

void SSupertalkScriptAssetEditor::OnTextChanged(const FText& NewText)
{
	if (IsValid(ScriptAsset))
	{
		ScriptAsset->SourceData = NewText.ToString();
		ScriptAsset->MarkPackageDirty();
	}
}
