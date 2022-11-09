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
	Ignore,
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
DECLARE_ENUM_TO_STRING(ESupertalkTokenType);

/**
 * A parser for Supertalk Script (*.sts) files
 */
class FSupertalkParser : public TSharedFromThis<FSupertalkParser>
{
public:
	static TSharedRef<FSupertalkParser> Create(FOutputDevice* Ar);
	static bool ParseIntoScript(FString File, FString Input, class USupertalkScript* Script, FOutputDevice* Ar);

	static bool IsReservedName(FName Input);

public:
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

		friend bool operator==(const FTokenContext& Lhs, const FTokenContext& Rhs)
		{
			return Lhs.File == Rhs.File && Lhs.Line == Rhs.Line && Lhs.Col == Rhs.Col;
		}

		FString ToString() const;
	};

	struct FToken
	{
		FTokenContext Context;
		ESupertalkTokenType Type = ESupertalkTokenType::Unknown;
		FString Content;
		FString Source;
		FString Namespace;
		int32 Indentation;

		FString GetDisplayName() const;

		bool IsIgnorable() const;

		FString GetGeneratedLocalizationKey() const;

		static bool IsPotentialLoop(const FToken& Lhs, const FToken& Rhs)
		{
			// Used to test if the same token is being emitted over and over again.
			return Lhs.Context == Rhs.Context && Lhs.Type == Rhs.Type && Lhs.Content == Rhs.Content;
		}
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
		FString ReadToEndOfFile();
		FString ReadBetweenContexts(const FTokenContext& Left, const FTokenContext& Right);

		void GoBack(int32 Count);

		bool IsOnEmptyLine();
	};

	struct FLxContext
	{
		FString File;
		FLxStream Stream;
		int32 CurrentIndentation;

		// HACK: emitting a choice token results in some special behavior around consuming the rest of the line.
		uint32 bIsChoiceLine : 1;

		FLxContext()
		{
			bIsChoiceLine = false;
		}

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
	bool TokenizeSyntax(const FString& File, const FString& Input, TArray<FToken>& OutTokens);

	//void RunTest();

private:
	FSupertalkParser();
	void Initialize();

	// If bIgnoreErrors is true, attempts to ignore any problems with lexing.
	// If an error can't be ignored, the rest of the input will be appended as a final "unknown" token.
	bool RunLexer(const FString& File, const FString& Input, TArray<FToken>& OutTokens, bool bIgnoreErrors = false);
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
	void ParseError(const FPaContext& InCtx, const FToken& Token, const FString& Message) const;
	void ParseError(const FPaContext& InCtx, const FString& Message) const;
	void ParseWarning(const FPaContext& InCtx, const FToken& Token, const FString& Message) const;
	void ParseWarning(const FPaContext& InCtx, const FString& Message) const;

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

	FOutputDevice* Ar = nullptr;
};