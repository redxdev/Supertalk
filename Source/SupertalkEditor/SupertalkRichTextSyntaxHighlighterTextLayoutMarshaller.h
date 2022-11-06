// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/PlainTextLayoutMarshaller.h"

class FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller : public FPlainTextLayoutMarshaller
{
public:
    struct FSyntaxTextStyle
    {
        FSyntaxTextStyle();
        FSyntaxTextStyle(
            const FTextBlockStyle& InNormalTextStyle,
            const FTextBlockStyle& InCommentTextStyle,
            const FTextBlockStyle& InKeywordTextStyle,
            const FTextBlockStyle& InOperatorTextStyle,
            const FTextBlockStyle& InValueTextStyle,
            const FTextBlockStyle& InStringTextStyle,
            const FTextBlockStyle& InCommandTextStyle,
            const FTextBlockStyle& InSectionTextStyle,
            const FTextBlockStyle& InJumpTextStyle,
            const FTextBlockStyle& InErrorTextStyle)
            : NormalTextStyle(InNormalTextStyle)
            , CommentTextStyle(InCommentTextStyle)
            , KeywordTextStyle(InKeywordTextStyle)
            , OperatorTextStyle(InOperatorTextStyle)
            , ValueTextStyle(InValueTextStyle)
            , StringTextStyle(InStringTextStyle)
            , CommandTextStyle(InCommandTextStyle)
            , SectionTextStyle(InSectionTextStyle)
            , JumpTextStyle(InJumpTextStyle)
            , ErrorTextStyle(InErrorTextStyle)
        {
        }

        FTextBlockStyle NormalTextStyle;
        FTextBlockStyle CommentTextStyle;
        FTextBlockStyle KeywordTextStyle;
        FTextBlockStyle OperatorTextStyle;
        FTextBlockStyle ValueTextStyle;
        FTextBlockStyle StringTextStyle;
        FTextBlockStyle CommandTextStyle;
        FTextBlockStyle SectionTextStyle;
        FTextBlockStyle JumpTextStyle;
        FTextBlockStyle ErrorTextStyle;
    };

    static TSharedRef<FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller> Create(TSharedRef<class FSupertalkParser> InParser, const FSyntaxTextStyle& InSyntaxTextStyle);

    virtual ~FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller();

    virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout) override;

    virtual bool RequiresLiveUpdate() const override;

protected:
    FSupertalkRichTextSyntaxHighlighterTextLayoutMarshaller(TSharedRef<class FSupertalkParser> InParser, const FSyntaxTextStyle& InSyntaxTextStyle);

    virtual void ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout);

    FSyntaxTextStyle SyntaxTextStyle;
    TSharedRef<class FSupertalkParser> Parser;
};
