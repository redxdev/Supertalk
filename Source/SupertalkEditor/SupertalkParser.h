// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
#include "Supertalk/SupertalkExpression.h"
#include "Supertalk/SupertalkLine.h"
#include "SupertalkParser.generated.h"

class USupertalkScript;
class USupertalkValue;
struct FSupertalkAction;

UENUM()
enum class ESupertalkTokenType : uint8
{
	Unknown,
	Eof,
    Name,
    Separator,
	Member,
    Assign,
    Text,
    Asset,
    Choice,
    Section,
    Comment,
    Jump,
    Command,
    ParallelStart,
    ParallelEnd,
    QueueStart,
    QueueEnd,
	StatementEnd,
	LocalizationKey,
	Directive,
	GroupStart,
	GroupEnd,
	If,
	Then,
	Else,
	Equal,
	NotEqual,
	Not,

	AttrStart = ParallelStart,
	AttrEnd = ParallelEnd
};

/**
 * A parser for Supertalk Script (*.sts) files
 */
class FSupertalkParser : public TSharedFromThis<FSupertalkParser>
{
public:
	static TSharedRef<FSupertalkParser> Create(class FMessageLog* MessageLog);

private:
	enum class ETextParseMode : uint8
	{
		SingleLine,
		MultiLine,
		Quoted,
	};

	struct FTokenContext
	{
		FString File;
		int32 Line = -1;
		int32 Col = -1;
	};

	struct FToken
	{
		FTokenContext Context;
		ESupertalkTokenType Type = ESupertalkTokenType::Unknown;
		FString Content;
		FString Namespace;
		int32 Indentation;

		FText GetDisplayName() const;

		bool IsIgnorable() const;

		FString GetGeneratedLocalizationKey() const;
	};

	struct FLxStream
	{
		FLxStream()
		{
			CurrentLine = 0;
			CurrentChar = 0;
		}
		
		TArray<FString> Lines;
		int32 CurrentLine;
		int32 CurrentChar;

		FORCEINLINE bool IsEOF() const
		{
			return !Lines.IsValidIndex(CurrentLine);
		}

		TCHAR ReadChar();
		FString ReadToEndOfLine();

		void GoBack(int32 Count);

		bool IsOnEmptyLine();
	};

	struct FLxContext
	{
		FString File;
		FLxStream Stream;
		int32 CurrentIndentation;

		FORCEINLINE FTokenContext CreateContext() const
		{
			FTokenContext Ctx;
			Ctx.File = File;
			Ctx.Line = Stream.CurrentLine + 1;
			Ctx.Col = Stream.CurrentChar + 1;
			return Ctx;
        }
	};

	struct FPaStream
	{
		FPaStream()
		{
			CurrentToken = 0;
		}
		
		TArray<FToken> Tokens;
		int32 CurrentToken;

		FORCEINLINE bool IsEOF() const
		{
			return !Tokens.IsValidIndex(CurrentToken);
		}

		FToken ReadToken();
		FToken PeekToken() const;
		void GoBack(int32 Count);
	};

	struct FPaContext
	{
		FPaStream Stream;

		USupertalkScript* Script;

		FString DefaultNamespace;

		// Used for checking that all jumps are valid later.
		TArray<TTuple<FToken, FName>> Jumps;
	};

public:
	~FSupertalkParser();

	bool Parse(const FString& File, const FString& Input, USupertalkScript* Script);

	//void RunTest();

private:
	FSupertalkParser();
	void Initialize();

	bool RunLexer(const FString& File, const FString& Input, TArray<FToken>& OutTokens);
	bool RunParser(USupertalkScript* Script, const TArray<FToken>& InTokens);

	void ConsumeEmptyLines(FLxContext& InCtx);
	int32 ConsumeWhitespaceUpdateIndentation(FLxContext& InCtx);
	int32 ConsumeWhitespace(FLxContext& InCtx);

	bool LxToken(FLxContext& InCtx, FToken& OutToken);

	bool LxTokenName(FLxContext& InCtx, FToken& OutName, bool AllowWhitespace);
	bool LxTokenText(FLxContext& InCtx, FToken& OutText, ETextParseMode Mode);
	bool LxTokenAsset(FLxContext& InCtx, FToken& OutAsset);
	bool LxTokenComment(FLxContext& InCtx, FToken& OutComment);
	bool LxTokenDirective(FLxContext& InCtx, FToken& OutDirective);
	bool LxTokenLocalizationKey(FLxContext& InCtx, FToken& OutLocalizationKey);

	void ParseTokenError(const FPaContext& InCtx, const FToken& Token, ESupertalkTokenType ExpectedTokenType) const;
	void ParseTokenError(const FPaContext& InCtx, const FToken& Token, const FString& Expected) const;
	void ParseError(const FPaContext& InCtx, const FToken& Token, const FText& Message) const;
	void ParseError(const FPaContext& InCtx, const FText& Message) const;
	void ParseWarning(const FPaContext& InCtx, const FToken& Token, const FText& Message) const;
	void ParseWarning(const FPaContext& InCtx, const FText& Message) const;

	bool PaTrySectionName(FPaContext& InCtx, FName& OutName);
	bool PaSection(FPaContext& InCtx, FName Name);
	
	bool PaAction(FPaContext& InCtx, FSupertalkAction& OutAction);
	
	bool PaDirective(FPaContext& InCtx);
	
	bool PaAssign(FPaContext& InCtx, FSupertalkAction& OutAction);
	bool PaLine(FPaContext& InCtx, FSupertalkAction& OutAction);
	bool PaChoice(FPaContext& InCtx, FSupertalkAction& OutAction, struct FSupertalkLine& Line);
	bool PaCommand(FPaContext& InCtx, FSupertalkAction& OutAction);
	bool PaJump(FPaContext& InCtx, FSupertalkAction& OutAction);
	bool PaParallel(FPaContext& InCtx, FSupertalkAction& OutAction);
	bool PaQueue(FPaContext& InCtx, FSupertalkAction& OutAction);
	bool PaConditional(FPaContext& InCtx, FSupertalkAction& OutAction);

	bool PaExpression(FPaContext& InCtx, TObjectPtr<USupertalkExpression>& OutExpression);
	bool PaEqualityExpression(FPaContext& InCtx, TObjectPtr<USupertalkExpression>& OutExpression);
	bool PaUnaryExpression(FPaContext& InCtx, TObjectPtr<USupertalkExpression>& OutExpression);
	bool PaGroupExpression(FPaContext& InCtx, TObjectPtr<USupertalkExpression>& OutExpression);
	bool PaValueExpression(FPaContext& InCtx, TObjectPtr<USupertalkExpression>& OutExpression);

	bool PaValue(FPaContext& InCtx, TObjectPtr<USupertalkValue>& OutValue);
	bool PaVariableValue(FPaContext& InCtx, TObjectPtr<USupertalkValue>& OutValue);
	bool PaTextValue(FPaContext& InCtx, TObjectPtr<USupertalkValue>& OutValue);
	bool PaAssetValue(FPaContext& InCtx, TObjectPtr<USupertalkValue>& OutValue);

	bool PaAttributeList(FPaContext& InCtx, TArray<FSupertalkAttribute>& OutAttributes);

	FMessageLog* MessageLog;
};