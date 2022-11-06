// Copyright (c) MissiveArts LLC

#include "SupertalkRichTextSyntaxHighlighterTextLayoutMarshaller.h"
#include "SupertalkEditorStyle.h"
#include "SupertalkParser.h"
#include "Framework/Text/ISlateRun.h"
#include "Framework/Text/SlateTextRun.h"

#define GET_TEXT_STYLE(Name) FSupertalkEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>(Name)
FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller::FSyntaxTextStyle::FSyntaxTextStyle()
	: NormalTextStyle(GET_TEXT_STYLE("SyntaxHighlight.STS.Normal"))
	, CommentTextStyle(GET_TEXT_STYLE("SyntaxHighlight.STS.Comment"))
	, KeywordTextStyle(GET_TEXT_STYLE("SyntaxHighlight.STS.Keyword"))
	, OperatorTextStyle(GET_TEXT_STYLE("SyntaxHighlight.STS.Operator"))
	, ValueTextStyle(GET_TEXT_STYLE("SyntaxHighlight.STS.Value"))
	, StringTextStyle(GET_TEXT_STYLE("SyntaxHighlight.STS.String"))
	, CommandTextStyle(GET_TEXT_STYLE("SyntaxHighlight.STS.Command"))
	, SectionTextStyle(GET_TEXT_STYLE("SyntaxHighlight.STS.Section"))
	, JumpTextStyle(GET_TEXT_STYLE("SyntaxHighlight.STS.Jump"))
	, ErrorTextStyle(GET_TEXT_STYLE("SyntaxHighlight.STS.Error"))
{
}
#undef GET_TEXT_STYLE

TSharedRef<FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller> FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller::Create(TSharedRef<class FSupertalkParser> InParser, const FSyntaxTextStyle& InSyntaxTextStyle)
{
	return MakeShareable(new FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller(InParser, InSyntaxTextStyle));
}

FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller::~FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller()
{
}

void FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
{
	ParseTokens(SourceString, TargetTextLayout);
}

bool FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller::RequiresLiveUpdate() const
{
	return true;
}

FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller::FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller(TSharedRef<class FSupertalkParser> InParser, const FSyntaxTextStyle& InSyntaxTextStyle)
	: SyntaxTextStyle(InSyntaxTextStyle), Parser(InParser)
{
}

void FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller::ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout)
{
	// This entire function is *beyond* hacky, as the supertalk parser tokenizes things into a different format than what FTextLayout works with.
	// It might be worth refactoring FSupertalkParser to use ranges or something at some point, but for now this is "good enough".

	TArray<FSupertalkParser::FToken> Tokens;
	Parser->TokenizeSyntax(FString("Input"), SourceString, Tokens);

	TArray<FTextLayout::FNewLineData> LinesToAdd;
	TSharedRef<FString> ModelString = MakeShared<FString>();
	TArray<TSharedRef<IRun>> Runs;
	for (const FSupertalkParser::FToken& Token : Tokens)
	{
		TArray<FString> Lines;
		Token.Source.ParseIntoArrayLines(Lines, false);

		for (int32 LineIdx = 0; LineIdx < Lines.Num(); ++LineIdx)
		{
			if (LineIdx != 0)
			{
				LinesToAdd.Emplace(ModelString, Runs);
				ModelString = MakeShared<FString>();
				Runs = TArray<TSharedRef<IRun>>();
			}

			FRunInfo RunInfo;
			RunInfo.Name = TEXT("SyntaxHighlight.STS.Normal");

			FTextBlockStyle TextBlockStyle = SyntaxTextStyle.NormalTextStyle;

			switch (Token.Type)
			{
			case ESupertalkTokenType::Comment:
				RunInfo.Name = TEXT("SyntaxHighlight.STS.Comment");
				TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
				break;

			case ESupertalkTokenType::If:
			case ESupertalkTokenType::Then:
			case ESupertalkTokenType::Else:
				RunInfo.Name = TEXT("SyntaxHighlight.STS.Keyword");
				TextBlockStyle = SyntaxTextStyle.KeywordTextStyle;
				break;


			case ESupertalkTokenType::Directive:
				if (Token.Content.StartsWith("error", ESearchCase::IgnoreCase))
				{
					RunInfo.Name = TEXT("SyntaxHighlight.STS.Error");
					TextBlockStyle = SyntaxTextStyle.ErrorTextStyle;
				}
				else
				{
					RunInfo.Name = TEXT("SyntaxHighlight.STS.Keyword");
					TextBlockStyle = SyntaxTextStyle.KeywordTextStyle;
				}

				break;

			case ESupertalkTokenType::Name:
				if (FSupertalkParser::IsReservedName(FName(Token.Content)))
				{
					RunInfo.Name = TEXT("SyntaxHighlight.STS.Keyword");
					TextBlockStyle = SyntaxTextStyle.KeywordTextStyle;
				}

				break;

			case ESupertalkTokenType::Separator:
			case ESupertalkTokenType::Assign:
			case ESupertalkTokenType::ParallelStart:
			case ESupertalkTokenType::ParallelEnd:
			case ESupertalkTokenType::QueueStart:
			case ESupertalkTokenType::QueueEnd:
			case ESupertalkTokenType::StatementEnd:
			case ESupertalkTokenType::GroupStart:
			case ESupertalkTokenType::GroupEnd:
			case ESupertalkTokenType::Equal:
			case ESupertalkTokenType::NotEqual:
			case ESupertalkTokenType::Not:
				RunInfo.Name = TEXT("SyntaxHighlight.STS.Operator");
				TextBlockStyle = SyntaxTextStyle.OperatorTextStyle;
				break;

			case ESupertalkTokenType::Asset:
			case ESupertalkTokenType::LocalizationKey:
				RunInfo.Name = TEXT("SyntaxHighlight.STS.Value");
				TextBlockStyle = SyntaxTextStyle.ValueTextStyle;
				break;

			case ESupertalkTokenType::Text:
				RunInfo.Name = TEXT("SyntaxHighlight.STS.String");
				TextBlockStyle = SyntaxTextStyle.StringTextStyle;
				break;

			case ESupertalkTokenType::Command:
				RunInfo.Name = TEXT("SyntaxHighlight.STS.Command");
				TextBlockStyle = SyntaxTextStyle.CommandTextStyle;
				break;

			case ESupertalkTokenType::Section:
				RunInfo.Name = TEXT("SyntaxHighlight.STS.Section");
				TextBlockStyle = SyntaxTextStyle.SectionTextStyle;
				break;

			case ESupertalkTokenType::Jump:
				RunInfo.Name = TEXT("SyntaxHighlight.STS.Jump");
				TextBlockStyle = SyntaxTextStyle.JumpTextStyle;
				break;
			}

			const FString& Line = Lines[LineIdx];
			const FTextRange ModelRange(ModelString->Len(), ModelString->Len() + Line.Len());
			ModelString->Append(*Line);

			TSharedRef<ISlateRun> Run = FSlateTextRun::Create(RunInfo, ModelString, TextBlockStyle, ModelRange);
			Runs.Add(Run);
		}
	}

	if (!ModelString->IsEmpty())
	{
		LinesToAdd.Emplace(MoveTemp(ModelString), MoveTemp(Runs));
	}

	TargetTextLayout.AddLines(LinesToAdd);
}
