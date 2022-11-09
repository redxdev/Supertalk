// Copyright (c) MissiveArts LLC


#include "SSupertalkScriptAssetEditor.h"
#include "SupertalkEditorStyle.h"
#include "SupertalkParser.h"
#include "SupertalkRichTextSyntaxHighlighterTextLayoutMarshaller.h"
#include "Supertalk/SupertalkPlayer.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBarTrack.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/SlateEditableTextLayout.h"

class SMultiLineEditableTextWithScrollExposed : public SMultiLineEditableText
{
public:
	float GetVerticalScroll()
	{
		return EditableTextLayout->GetScrollOffset().Y;
	}
};

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

	TSharedRef<FSupertalkParser> Parser = FSupertalkParser::Create(nullptr);
	TSharedRef<FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller> RichTextMarshaller = FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller::Create(
		Parser,
		FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller::FSyntaxTextStyle());

	TSharedRef<SScrollBar> HorizontalScrollBar =
		SNew(SScrollBar)
		.Orientation(Orient_Horizontal)
		.Thickness(FVector2D(7.0, 7.0));

	TSharedRef<SScrollBar> VerticalScrollBar =
		SNew(SScrollBar)
		.Orientation(Orient_Vertical)
		.Thickness(FVector2D(7.0, 7.0));

	ChildSlot
	[
		SNew(SGridPanel)
		.FillColumn(1, 1.0f)
		.FillRow(0, 1.0f)
		+ SGridPanel::Slot(0, 0)
		[
			SAssignNew(LineNumbersScroll, SScrollBox)
			.Orientation(Orient_Vertical)
			.ScrollBarVisibility(EVisibility::Collapsed)
			+ SScrollBox::Slot()
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(FColor(0xffffffff)))
				.Padding(0.5f)
				[
					SAssignNew(LineNumbers, STextBlock)
						.TextStyle(FSupertalkEditorStyle::Get(), "TextEditor.LineNumberText")
						.Justification(ETextJustify::Right)
						.Margin(FMargin(5.f, 0.f))
				]
			]
		]
		+ SGridPanel::Slot(1, 0)
		[
			SAssignNew(EditableText, SMultiLineEditableTextWithScrollExposed)
			.IsReadOnly(InArgs._IsReadOnly)
			.Text(FText::FromString(ScriptAsset->SourceData))
			.Font(FSupertalkEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("TextEditor.NormalText").Font)
			.TextStyle(FSupertalkEditorStyle::Get(), "TextEditor.EditableTextBox")
			.Marshaller(RichTextMarshaller)
			.HScrollBar(HorizontalScrollBar)
			.VScrollBar(VerticalScrollBar)
			.AutoWrapText(false)
			.Margin(FMargin(2.f, 0.f))
			.OnTextChanged(this, &SSupertalkScriptAssetEditor::OnTextChanged)
			.OnIsTypedCharValid_Lambda([](TCHAR Character) { return true; })
		]
		+ SGridPanel::Slot(1, 1)
		[
			HorizontalScrollBar
		]
		+ SGridPanel::Slot(2, 0)
		[
			VerticalScrollBar
		]
	];

	RefreshLineNumbers();

	FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &SSupertalkScriptAssetEditor::HandleScriptAssetPropertyChanged);
}

void SSupertalkScriptAssetEditor::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	LineNumbersScroll->SetScrollOffset(EditableText->GetVerticalScroll());

	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

void SSupertalkScriptAssetEditor::HandleScriptAssetPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	if (Object == ScriptAsset)
	{
		EditableText->SetText(FText::FromString(ScriptAsset->SourceData));
		RefreshLineNumbers();
	}
}

void SSupertalkScriptAssetEditor::OnTextChanged(const FText& NewText)
{
	if (IsValid(ScriptAsset))
	{
		ScriptAsset->SourceData = NewText.ToString();
		ScriptAsset->MarkPackageDirty();
		RefreshLineNumbers();
	}
}

static int32 CountLineBreaks(const FString& Str)
{
	int32 Result = 0;
	for (int32 Idx = 0; Idx < Str.Len(); ++Idx)
	{
		TCHAR Char = Str[Idx];
		if (Char == TEXT('\n'))
		{
			++Result;
		}
	}

	return Result;
}

void SSupertalkScriptAssetEditor::RefreshLineNumbers()
{
	// TODO: text box doesn't expose the number of lines, but it's really where we should be getting this information from.
	// Could use a custom subclass to access that info.
	int32 WantedNumbers = IsValid(ScriptAsset) ? (CountLineBreaks(ScriptAsset->SourceData) + 1) : 1;
	if (WantedNumbers <= PreviousNumbers)
	{
		return;
	}

	while (WantedNumbers > CacheLen)
	{
		++CacheLen;
		NumberCache += FString::FromInt(CacheLen) + '\n';
	}

	LineNumbers->SetText(FText::FromString(NumberCache));
}
