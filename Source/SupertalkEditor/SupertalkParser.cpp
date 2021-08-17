// Copyright (c) MissiveArts LLC

#include "SupertalkParser.h"

#include "Misc/FeedbackContext.h"
#include "Supertalk/SupertalkPlayer.h"
#include "Supertalk/SupertalkValue.h"
#include "Supertalk/SupertalkLine.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "SupertalkParser"

namespace Symbols
{
	static const TCHAR Separator = TEXT(',');
	static const TCHAR Member = TEXT('.');
	static const TCHAR Assign = TEXT('=');
	
	static const TCHAR AttrStart = TEXT('[');
	static const TCHAR AttrEnd = TEXT(']');
	
	static const TCHAR TextStart = TEXT(':');

	static const TCHAR AssetStart1 = TEXT('/');
	static const TCHAR AssetStart2 = TEXT('\\');
	
	static const TCHAR ChoiceStart = TEXT('*');
	
	static const TCHAR SectionStart = TEXT('#');
	
	static const TCHAR Comment = TEXT('-');

	static const TCHAR Jump = TEXT('>');

	static const TCHAR CommandStart = TEXT('>');
	
	static const TCHAR ParallelStart = TEXT('{');
	static const TCHAR ParallelEnd = TEXT('}');

	static const TCHAR QueueStart = TEXT('(');
	static const TCHAR QueueEnd = TEXT(')');

	static const TCHAR StatementEnd = TEXT(';');

	static const TCHAR SingleQuote = TEXT('\'');
	static const TCHAR DoubleQuote = TEXT('"');

	static const TCHAR LocalizationKeyStart = TEXT('@');

	static const TCHAR DirectiveStart = TEXT('!');

	// TODO: implement escape sequences
	// This isn't used yet, sadly.
	static const TCHAR Escape = TEXT('\\');
}

bool IsTokenSymbol(TCHAR Char)
{
	switch (Char)
	{
	default:
		return false;

    case Symbols::Separator:
    case Symbols::Member:
    case Symbols::Assign:
    case Symbols::AttrStart:
	case Symbols::AttrEnd:
    case Symbols::TextStart:
    case Symbols::AssetStart1:
    case Symbols::AssetStart2:
    case Symbols::ChoiceStart:
    case Symbols::SectionStart:
    case Symbols::Comment:
    case Symbols::CommandStart:
    case Symbols::ParallelStart:
	case Symbols::StatementEnd:
	case Symbols::SingleQuote:
	case Symbols::DoubleQuote:
	case Symbols::LocalizationKeyStart:
	case Symbols::DirectiveStart:
		return true;
    }
}

TSharedRef<FSupertalkParser> FSupertalkParser::Create(FMessageLog* MessageLog)
{
	check(MessageLog);
	
	TSharedRef<FSupertalkParser> Parser = MakeShareable(new FSupertalkParser());
	Parser->MessageLog = MessageLog;
	Parser->Initialize();
	return Parser;
}

FText FSupertalkParser::FToken::GetDisplayName() const
{
	const UEnum* Enum = FindObjectChecked<UEnum>(ANY_PACKAGE, TEXT("ESupertalkTokenType"), true);
	return Enum->GetDisplayNameTextByValue(static_cast<int32>(Type));
}

bool FSupertalkParser::FToken::IsIgnorable() const
{
	switch (Type)
	{
	default:
		return false;

	case ESupertalkTokenType::Comment:
		return true;
	}
}

FString FSupertalkParser::FToken::GetGeneratedLocalizationKey() const
{
	return FString::Printf(TEXT("%s_L%dC%d"), *Context.File, Context.Line, Context.Col);
}

TCHAR FSupertalkParser::FLxStream::ReadChar()
{
	if (IsEOF())
	{
		++CurrentChar; // so that GoBack works correctly on Eof
		return TEXT('\0');
	}

	if (!Lines[CurrentLine].IsValidIndex(CurrentChar))
	{
		CurrentChar = 0;
		++CurrentLine;
		return TEXT('\n');
	}

	return Lines[CurrentLine][CurrentChar++];
}

FString FSupertalkParser::FLxStream::ReadToEndOfLine()
{
	if (IsEOF())
	{
		return FString();
	}

	if (!Lines[CurrentLine].IsValidIndex(CurrentChar))
	{
		ReadChar();
		return FString();
	}

	FString Result = Lines[CurrentLine].Mid(CurrentChar);
	
	CurrentChar = 0;
	++CurrentLine;

	return Result;
}

void FSupertalkParser::FLxStream::GoBack(int32 Count)
{
	check(Count >= 0);

	for (int32 Idx = 0; Idx < Count; ++Idx)
	{
		--CurrentChar;
		if (CurrentChar < 0)
		{
			--CurrentLine;
			if (CurrentLine < 0)
			{
				CurrentLine = 0;
			}

			if (Lines.IsValidIndex(CurrentLine) && !Lines[CurrentLine].IsEmpty())
			{
				CurrentChar = Lines[CurrentLine].Len() - 1;
			}
			else
			{
				CurrentChar = 0;
			}
		}
	}
}

bool FSupertalkParser::FLxStream::IsOnEmptyLine()
{
	if (IsEOF() || Lines[CurrentLine].TrimStartAndEnd().IsEmpty())
	{
		return true;
	}

	return false;
}

FSupertalkParser::FToken FSupertalkParser::FPaStream::ReadToken()
{
	if (IsEOF())
	{
		FToken Token;
		Token.Type = ESupertalkTokenType::Eof;
		++CurrentToken; // so that GoBack works correctly at Eof
		return Token;
	}

	return Tokens[CurrentToken++];
}

FSupertalkParser::FToken FSupertalkParser::FPaStream::PeekToken() const
{
	if (IsEOF())
	{
		FToken Token;
		Token.Type = ESupertalkTokenType::Eof;
		return Token;
	}

	return Tokens[CurrentToken];
}

void FSupertalkParser::FPaStream::GoBack(int32 Count)
{
	check(Count >= 0);

	CurrentToken -= Count;
	if (CurrentToken < 0)
	{
		CurrentToken = 0;
	}
}

FSupertalkParser::~FSupertalkParser()
{
}

bool FSupertalkParser::Parse(const FString& File, const FString& Input, USupertalkScript* Script)
{
	if (!ensure(Script))
	{
		return false;
	}

	TArray<FToken> Tokens;
	if (!RunLexer(File, Input, Tokens))
	{
		return false;
	}

	if (!RunParser(Script, Tokens))
	{
		return false;
	}

	return true;
}

/*
void FSupertalkParser::RunTest()
{
	FString File = TEXT("RunTest.sts");
	FString Content = TEXT("Foo: bar\n  baz blah\n\n  \n  bing\n  * abcd -- test\nBar = /abc/def/ghi -- test\n> Wait abcd");
	TArray<FToken> Tokens;

	if (RunLexer(File, Content, Tokens))
	{
		for (const FToken& Token : Tokens)
		{
			UE_LOG(LogSupertalk, Log, TEXT("Token at %d:%d %s with content %s"), Token.Context.Line, Token.Context.Col, *Token.GetDisplayName().ToString(), *Token.Content);
		}
	}
	else
	{
		UE_LOG(LogSupertalk, Warning, TEXT("Failed to run lexer"));
		return;
	}

	USupertalkScript* Script = NewObject<USupertalkScript>();
	if (RunParser(Script, Tokens))
	{
		UE_LOG(LogSupertalk, Log, TEXT("Parser success"));
	}
	else
	{
		UE_LOG(LogSupertalk, Warning, TEXT("Failed to run parser"));
	}
}
*/

FSupertalkParser::FSupertalkParser()
{
}

void FSupertalkParser::Initialize()
{
}

bool FSupertalkParser::RunLexer(const FString& File, const FString& Input, TArray<FToken>& OutTokens)
{
	FLxContext Ctx;
	Ctx.File = File;
	Ctx.Stream.CurrentChar = 0;
	Ctx.Stream.CurrentLine = 0;

	Input.ParseIntoArrayLines(Ctx.Stream.Lines, false);
	for (int32 Idx = 0; Idx < Ctx.Stream.Lines.Num(); ++Idx)
	{
		Ctx.Stream.Lines[Idx].TrimEndInline();
	}
	
	while (!Ctx.Stream.IsEOF())
	{
		FToken Token;
		if (!LxToken(Ctx, Token))
		{
			FString TokenPosition = FString::Printf(TEXT("%s:%d:%d"), *Token.Context.File, Token.Context.Line, Token.Context.Col);
			MessageLog->Error(FText::Format(LOCTEXT("LexerUnexpectedContent", "{0} - lexer: unexpected content '{1}'"), FText::FromString(TokenPosition), FText::FromString(Token.Content)));
			return false;
		}

		if (Token.Type == ESupertalkTokenType::Eof)
		{
			continue;
		}

		OutTokens.Add(Token);
	}

	return true;
}

bool FSupertalkParser::RunParser(USupertalkScript* Script, const TArray<FToken>& InTokens)
{
	check(Script);
	
	FPaContext Ctx;
	Ctx.Script = Script;
	Ctx.DefaultNamespace = TEXT("Script.Default");
	Ctx.Stream.Tokens = InTokens;

	// Remove any ignorable tokens (for example, comments) from the stream.
	Ctx.Stream.Tokens.RemoveAll([](const FToken& Token) { return Token.IsIgnorable(); });

	FName DefaultSection = NAME_None;
	TMap<FName, FSupertalkSection> Sections;
	
	while (!Ctx.Stream.IsEOF())
	{
		// All statements must be inside a section, though the first section of the file is implicit.
		// The first section, if it has any statements, will be used as the default section.
		bool bIsUnnamedSection = false;
		FName SectionName;
		if (!PaTrySectionName(Ctx, SectionName))
		{
			if (DefaultSection == NAME_None)
			{
				bIsUnnamedSection = true;
				SectionName = NAME_Default;
			}
			else
			{
				ParseTokenError(Ctx, Ctx.Stream.PeekToken(), ESupertalkTokenType::Section);
				return false;
			}
		}

		if (SectionName == NAME_None)
		{
			ParseError(Ctx, LOCTEXT("SectionNameNotNone", "Section name cannot be 'None'"));
			return false;
		}

		if (!PaSection(Ctx, SectionName))
		{
			return false;
		}

		// Certain actions (specifically, directives) don't actually compile to anything and as such
		// shouldn't cause an unnamed default section to be created.
		if (bIsUnnamedSection && Ctx.Script->Sections[SectionName].Actions.Num() == 0)
		{
			Ctx.Script->Sections.Remove(SectionName);
			continue;
		}

		if (Ctx.Script->DefaultSection == NAME_None)
		{
			Ctx.Script->DefaultSection = SectionName;
		}
	}

	// Make sure jumps all happened with valid section names
	bool bAllValidJumps = true;
	for (const auto& Jump : Ctx.Jumps)
	{
		// Always allow None as a jump destination, as it is used to signal the script to end.
		if (Jump.Value != NAME_None && !Ctx.Script->Sections.Contains(Jump.Value))
		{
			bAllValidJumps = false;
			ParseError(Ctx, Jump.Key, FText::Format(LOCTEXT("InvalidJumpSection", "jump to unknown section '{0}'"), FText::FromName(Jump.Value)));
		}
	}

	if (!bAllValidJumps)
	{
		return false;
	}

	return true;
}

void FSupertalkParser::ConsumeEmptyLines(FLxContext& InCtx)
{
	int32 InitialLine = InCtx.Stream.CurrentLine;
	int32 InitialChar = InCtx.Stream.CurrentChar;
	while (!InCtx.Stream.IsEOF())
	{
		switch (InCtx.Stream.ReadChar())
		{
		default:
			if (InitialLine != InCtx.Stream.CurrentLine)
			{
				InCtx.Stream.CurrentChar = 0;
			}
			else
			{
				InCtx.Stream.CurrentChar = InitialChar;
			}
			return;

		case TEXT('\0'):
		case TEXT('\n'):
		case TEXT('\r'):
		case TEXT(' '):
		case TEXT('\t'):
			break;
		}
	}
}

int32 FSupertalkParser::ConsumeWhitespaceUpdateIndentation(FLxContext& InCtx)
{
	if (InCtx.Stream.CurrentChar == 0)
	{
		InCtx.CurrentIndentation = ConsumeWhitespace(InCtx);
		return InCtx.CurrentIndentation;
	}

	return ConsumeWhitespace(InCtx);
}

int32 FSupertalkParser::ConsumeWhitespace(FLxContext& InCtx)
{
	if (InCtx.Stream.IsEOF())
	{
		return 0;
	}

	int32 Consumed = 0;
	while (!InCtx.Stream.IsEOF())
	{
		switch (InCtx.Stream.ReadChar())
		{
		default:
			InCtx.Stream.GoBack(1);
			return Consumed;

        case ' ':
        case '\t':
			++Consumed;
            break;
		}
	}

	return Consumed;
}

bool FSupertalkParser::LxToken(FLxContext& InCtx, FToken& OutToken)
{
	if (InCtx.Stream.IsEOF())
	{
		return false;
	}

	ConsumeEmptyLines(InCtx);
	ConsumeWhitespaceUpdateIndentation(InCtx);
	
	OutToken = FToken();
	OutToken.Type = ESupertalkTokenType::Unknown;
	OutToken.Context = InCtx.CreateContext();
	OutToken.Indentation = InCtx.CurrentIndentation;
	
	if (InCtx.Stream.IsEOF())
	{
		OutToken.Type = ESupertalkTokenType::Eof;
		return true;
	}

	TCHAR Char = InCtx.Stream.ReadChar();
	OutToken.Content = FString() + Char;
	switch (Char)
	{
	default:
		InCtx.Stream.GoBack(1);
		OutToken.Type = ESupertalkTokenType::Name;
		return LxTokenName(InCtx, OutToken, true);

	case Symbols::Separator:
		OutToken.Type = ESupertalkTokenType::Separator;
		return true;

	case Symbols::Member:
		OutToken.Type = ESupertalkTokenType::Member;
		return true;

	case Symbols::Assign:
		OutToken.Type = ESupertalkTokenType::Assign;
		return true;

	case Symbols::AttrStart:
		OutToken.Type = ESupertalkTokenType::AttrStart;
		return true;

	case Symbols::AttrEnd:
		OutToken.Type = ESupertalkTokenType::AttrEnd;
		return true;

	case Symbols::TextStart:
		OutToken.Type = ESupertalkTokenType::Text;
		if (!LxTokenText(InCtx, OutToken, ETextParseMode::MultiLine))
		{
			OutToken.Content = FString();
		}
		
		return true;

	case Symbols::SingleQuote:
	case Symbols::DoubleQuote:
		OutToken.Type = ESupertalkTokenType::Text;
		InCtx.Stream.GoBack(1);
		return LxTokenText(InCtx, OutToken, ETextParseMode::Quoted);

	case Symbols::AssetStart1:
	case Symbols::AssetStart2:
		OutToken.Type = ESupertalkTokenType::Asset;
		InCtx.Stream.GoBack(1);
		return LxTokenAsset(InCtx, OutToken);

	case Symbols::ChoiceStart:
		OutToken.Type = ESupertalkTokenType::Choice;
		if (!LxTokenText(InCtx, OutToken, ETextParseMode::SingleLine))
		{
			OutToken.Content = FString();
		}
		
		return true;

	case Symbols::SectionStart:
		OutToken.Type = ESupertalkTokenType::Section;
		return LxTokenName(InCtx, OutToken, true);

	case Symbols::Comment:
		Char = InCtx.Stream.ReadChar();
		OutToken.Content = OutToken.Content + Char;
		switch (Char)
		{
		default:
			InCtx.Stream.GoBack(2);
			return false;

		case Symbols::Comment:
			OutToken.Type = ESupertalkTokenType::Comment;
			return LxTokenComment(InCtx, OutToken);

		case Symbols::Jump:
			OutToken.Type = ESupertalkTokenType::Jump;
			return LxTokenName(InCtx, OutToken, true);
		}

	case Symbols::CommandStart:
		OutToken.Type = ESupertalkTokenType::Command;
		return LxTokenText(InCtx, OutToken, ETextParseMode::SingleLine);

	case Symbols::ParallelStart:
		OutToken.Type = ESupertalkTokenType::ParallelStart;
		return true;

	case Symbols::ParallelEnd:
		OutToken.Type = ESupertalkTokenType::ParallelEnd;
		return true;

	case Symbols::QueueStart:
		OutToken.Type = ESupertalkTokenType::QueueStart;
		return true;

	case Symbols::QueueEnd:
		OutToken.Type = ESupertalkTokenType::QueueEnd;
		return true;

	case Symbols::StatementEnd:
		OutToken.Type = ESupertalkTokenType::StatementEnd;
		return true;

	case Symbols::DirectiveStart:
		OutToken.Type = ESupertalkTokenType::Directive;
		return LxTokenDirective(InCtx, OutToken);

	case Symbols::LocalizationKeyStart:
		OutToken.Type = ESupertalkTokenType::LocalizationKey;
		return LxTokenLocalizationKey(InCtx, OutToken);
	}
}

bool FSupertalkParser::LxTokenName(FLxContext& InCtx, FToken& OutName, bool AllowWhitespace)
{
	FString Content;
	bool bHasStarted = false;
	while (!InCtx.Stream.IsEOF())
	{
		TCHAR Char = InCtx.Stream.ReadChar();
		if (!bHasStarted && FChar::IsWhitespace(Char))
		{
			continue;
		}
		if (IsTokenSymbol(Char) || Char == TEXT('\n') || (!AllowWhitespace && FChar::IsWhitespace(Char)))
		{
			if (Char != TEXT('\n'))
			{
				InCtx.Stream.GoBack(1);
			}
			
			break;
		}
		else
		{
			bHasStarted = true;
			Content += Char;
		}
	}

	Content.TrimStartAndEndInline();
	if (Content.IsEmpty())
	{
		return false;
	}

	OutName.Content = Content;
	return true;
}

bool FSupertalkParser::LxTokenText(FLxContext& InCtx, FToken& OutText, ETextParseMode Mode)
{
	FString Content;
	int32 InitialIndentation = InCtx.CurrentIndentation;
	bool bOnNewLine = false;

	TCHAR QuoteChar = Symbols::DoubleQuote;
	if (Mode == ETextParseMode::Quoted)
	{
		QuoteChar = InCtx.Stream.ReadChar();
	}
	
	while (!InCtx.Stream.IsEOF())
	{
		TCHAR Char = InCtx.Stream.ReadChar();
		
		switch (Char)
		{
		case Symbols::ChoiceStart:
			if (bOnNewLine)
			{
				InCtx.Stream.CurrentChar = 0;
				goto complete;
			}

			goto textChar;

		case Symbols::DoubleQuote:
		case Symbols::SingleQuote:
			if (Mode == ETextParseMode::Quoted && QuoteChar == Char)
			{
				goto complete;
			}

			goto textChar;

		textChar:
		default:
			bOnNewLine = false;
			Content += Char;
			break;

		case TEXT('\n'):
			if (Mode == ETextParseMode::MultiLine)
			{
				if (bOnNewLine)
				{
					Content += TEXT('\n');
				}

				if (InCtx.Stream.IsOnEmptyLine())
				{
					ConsumeWhitespace(InCtx);
				}
				else
				{
					if (!bOnNewLine)
					{
						Content += TEXT(' ');
					}

					if (ConsumeWhitespace(InCtx) <= InitialIndentation)
					{
						InCtx.Stream.CurrentChar = 0;
						goto complete;
					}
				}
				
				bOnNewLine = true;
				break;
			}
			else if (Mode == ETextParseMode::Quoted)
			{
				return false;
			}
			else
			{
				goto complete;
			}
		}
	}

complete:
	Content.TrimStartAndEndInline();
	if (Content.IsEmpty())
	{
		return false;
	}

	OutText.Content = Content;
	return true;
}

bool FSupertalkParser::LxTokenAsset(FLxContext& InCtx, FToken& OutAsset)
{
	FString Content = FString();
	while (!InCtx.Stream.IsEOF())
	{
		TCHAR Char = InCtx.Stream.ReadChar();
		if (FChar::IsIdentifier(Char) || Char == TEXT('/') || Char == TEXT('\\') || Char == TEXT(' ') || Char == TEXT('.'))
		{
			Content += Char;
		}
		else
		{
			if (Char != TEXT('\n'))
			{
				InCtx.Stream.GoBack(1);
			}
			break;
		}
	}

	Content.TrimStartAndEndInline();
	if (Content.IsEmpty())
	{
		return false;
	}

	OutAsset.Content = Content;
	return true;
}

bool FSupertalkParser::LxTokenComment(FLxContext& InCtx, FToken& OutComment)
{
	OutComment.Content = InCtx.Stream.ReadToEndOfLine();
	return true;
}

bool FSupertalkParser::LxTokenDirective(FLxContext& InCtx, FToken& OutDirective)
{
	FString Content = FString();
	while (!InCtx.Stream.IsEOF())
	{
		TCHAR Char = InCtx.Stream.ReadChar();
		if (Char == '\n')
		{
			break;
		}

		Content += Char;
	}

	Content.TrimStartAndEndInline();
	if (Content.IsEmpty())
	{
		return false;
	}

	OutDirective.Content = Content;
	return true;
}

bool FSupertalkParser::LxTokenLocalizationKey(FLxContext& InCtx, FToken& OutLocalizationKey)
{
	FString Content = FString();
	FString Namespace;
	while (!InCtx.Stream.IsEOF())
	{
		TCHAR Char = InCtx.Stream.ReadChar();
		if (Char == TEXT('/'))
		{
			if (!Namespace.IsEmpty())
			{
				return false;
			}

			Namespace = Content;
			Content = FString();
		}
		else if (Char == Symbols::SingleQuote
			|| Char == Symbols::DoubleQuote
			|| Char == Symbols::TextStart
			|| FChar::IsWhitespace(Char))
		{
			InCtx.Stream.GoBack(1);
			break;
		}
		else
		{
			Content += Char;
		}
	}
	
	Content.TrimStartAndEndInline();
	if (Content.IsEmpty())
	{
		return false;
	}

	Namespace.TrimStartAndEndInline();
	OutLocalizationKey.Content = Content;
	OutLocalizationKey.Namespace = Namespace;
	return true;
}

void FSupertalkParser::ParseTokenError(const FPaContext& InCtx, const FToken& Token, ESupertalkTokenType ExpectedTokenType) const
{
	FToken ExpectedToken;
	ExpectedToken.Type = ExpectedTokenType;

	ParseTokenError(InCtx, Token, ExpectedToken.GetDisplayName().ToString());
}

void FSupertalkParser::ParseTokenError(const FPaContext& InCtx, const FToken& Token, const FString& Expected) const
{
	FText TokenName = Token.GetDisplayName();
	ParseError(InCtx, Token, FText::Format(LOCTEXT("ParseTokenExpected", "expected token {0} but found {1}"), FText::FromString(Expected), TokenName));
}

void FSupertalkParser::ParseError(const FPaContext& InCtx, const FToken& Token, const FText& Message) const
{
	FString TokenPosition = FString::Printf(TEXT("%s:%d:%d"), *Token.Context.File, Token.Context.Line, Token.Context.Col);
	MessageLog->Error(FText::Format(LOCTEXT("ParseError", "{0} - parser: {1}"), FText::FromString(TokenPosition), Message));
}

void FSupertalkParser::ParseError(const FPaContext& InCtx, const FText& Message) const
{
	ParseError(InCtx, InCtx.Stream.PeekToken(), Message);
}

void FSupertalkParser::ParseWarning(const FPaContext& InCtx, const FToken& Token, const FText& Message) const
{
	FString TokenPosition = FString::Printf(TEXT("%s:%d:%d"), *Token.Context.File, Token.Context.Line, Token.Context.Col);
	MessageLog->Warning(FText::Format(LOCTEXT("ParseWarning", "{0} - parser: {1}"), FText::FromString(TokenPosition), Message));
}

void FSupertalkParser::ParseWarning(const FPaContext& InCtx, const FText& Message) const
{
	ParseWarning(InCtx, InCtx.Stream.PeekToken(), Message);
}

bool FSupertalkParser::PaTrySectionName(FPaContext& InCtx, FName& OutName)
{
	FToken Token = InCtx.Stream.ReadToken();
	if (Token.Type != ESupertalkTokenType::Section)
	{
		InCtx.Stream.GoBack(1);
		return false;
	}

	OutName = FName(Token.Content);
	return true;
}

bool FSupertalkParser::PaSection(FPaContext& InCtx, FName Name)
{
	FSupertalkSection Section;
	Section.Name = Name;
	
	while (!InCtx.Stream.IsEOF() && InCtx.Stream.PeekToken().Type != ESupertalkTokenType::Section)
	{
		FSupertalkAction Action;
		if (!PaAction(InCtx, Action))
		{
			return false;
		}

		if (Action.Operation != ESupertalkOperation::Noop)
		{
			Section.Actions.Add(Action);
		}
	}

	InCtx.Script->Sections.Add(Section.Name, Section);

	return true;
}

bool FSupertalkParser::PaAction(FPaContext& InCtx, FSupertalkAction& OutAction)
{
	FToken Token = InCtx.Stream.ReadToken();
	switch (Token.Type)
	{
	default:
		ParseTokenError(InCtx, Token, TEXT("Name, Command, Jump, ParallelStart, QueueStart"));
		return false;

	case ESupertalkTokenType::Directive:
		InCtx.Stream.GoBack(1);
		return PaDirective(InCtx);

	case ESupertalkTokenType::Name:
		{
			FToken Token2 = InCtx.Stream.ReadToken();
			switch (Token2.Type)
			{
			default:
				ParseTokenError(InCtx, Token, TEXT("Assign, AttrStart, Text"));
				return false;

			case ESupertalkTokenType::Assign:
				InCtx.Stream.GoBack(2);
				return PaAssign(InCtx, OutAction);

			case ESupertalkTokenType::Separator:
			case ESupertalkTokenType::AttrStart:
			case ESupertalkTokenType::LocalizationKey:
			case ESupertalkTokenType::Text:
				InCtx.Stream.GoBack(2);
				return PaLine(InCtx, OutAction);
			}
		}

	case ESupertalkTokenType::Command:
		InCtx.Stream.GoBack(1);
		return PaCommand(InCtx, OutAction);

	case ESupertalkTokenType::Jump:
		InCtx.Stream.GoBack(1);
		return PaJump(InCtx, OutAction);

	case ESupertalkTokenType::ParallelStart:
		InCtx.Stream.GoBack(1);
		return PaParallel(InCtx, OutAction);

	case ESupertalkTokenType::QueueStart:
		InCtx.Stream.GoBack(1);
		return PaQueue(InCtx, OutAction);
	}
}

bool FSupertalkParser::PaDirective(FPaContext& InCtx)
{
	FToken Token = InCtx.Stream.ReadToken();
	check(Token.Type == ESupertalkTokenType::Directive);

	FString Directive;
	FString Content;
	if (!Token.Content.Split(TEXT(" "), &Directive, &Content))
	{
		Directive = Token.Content;
	}

	if (Directive.Compare(TEXT("namespace"), ESearchCase::IgnoreCase) == 0)
	{
		Content.TrimStartAndEndInline();
		if (Content.IsEmpty())
		{
			ParseError(InCtx, Token, LOCTEXT("InvalidNamespaceDirective", "namespace directive cannot be passed empty text"));
			return false;
		}
		
		InCtx.DefaultNamespace = Content;
		return true;
	}
	
	ParseError(InCtx, Token, FText::Format(LOCTEXT("InvalidDirective", "unknown directive '{0}'"), FText::FromString(Directive)));
	return false;
}

bool FSupertalkParser::PaAssign(FPaContext& InCtx, FSupertalkAction& OutAction)
{
	FToken VarToken = InCtx.Stream.ReadToken();
	check(VarToken.Type == ESupertalkTokenType::Name);

	FName VarName = FName(VarToken.Content);
	if (VarName == NAME_None)
	{
		ParseError(InCtx, VarToken, LOCTEXT("InvalidVariableName", "invalid variable name"));
		return false;
	}

	FToken AssignToken = InCtx.Stream.ReadToken();
	if (AssignToken.Type != ESupertalkTokenType::Assign)
	{
		ParseTokenError(InCtx, AssignToken, ESupertalkTokenType::Assign);
		return false;
	}

	TObjectPtr<USupertalkValue> Value = nullptr;
	if (!PaValue(InCtx, Value))
	{
		return false;
	}

	OutAction.Operation = ESupertalkOperation::Assign;
	
	USupertalkAssignParams* Params = NewObject<USupertalkAssignParams>(InCtx.Script);
	Params->Variable = FName(VarToken.Content);
	Params->Value = Value;
	OutAction.Params = Params;

	return true;
}

bool FSupertalkParser::PaLine(FPaContext& InCtx, FSupertalkAction& OutAction)
{
	FSupertalkLine Line;
	if (!PaValue(InCtx, Line.Speaker))
	{
		return false;
	}

	FToken Token = InCtx.Stream.ReadToken();
	if (Token.Type == ESupertalkTokenType::Separator)
	{
		if (!PaValue(InCtx, Line.SpeakerNameOverride))
		{
			return false;
		}

		Token = InCtx.Stream.ReadToken();
	}

	if (Token.Type == ESupertalkTokenType::AttrStart)
	{
		InCtx.Stream.GoBack(1);
		if (!PaAttributeList(InCtx, Line.Attributes))
		{
			return false;
		}

		Token = InCtx.Stream.ReadToken();
	}

	if (Token.Type == ESupertalkTokenType::StatementEnd)
	{
		Line.bIsBlankLine = true;

		OutAction.Operation = ESupertalkOperation::Line;

		USupertalkPlayLineParams* Params = NewObject<USupertalkPlayLineParams>(InCtx.Script);
		Params->Line = Line;
		OutAction.Params = Params;
		
		return true;
	}

	FString Namespace = InCtx.DefaultNamespace;
	FString Key;
	if (Token.Type == ESupertalkTokenType::LocalizationKey)
	{
		if (!Token.Namespace.IsEmpty())
		{
			Namespace = Token.Namespace;
		}

		Key = Token.Content;

		Token = InCtx.Stream.ReadToken();
	}

	if (Token.Type != ESupertalkTokenType::Text)
	{
		ParseTokenError(InCtx, Token, ESupertalkTokenType::Text);
		return false;
	}

	// Is there a better way to initialize an FText with a variable namespace/key? The FText constructor we want isn't
	// accessible sadly, and double-constructing an FText (due to the FText::FromString call) seems inefficient.
	Line.Text = FText::ChangeKey(FTextKey(Namespace), FTextKey(Key.IsEmpty() ? Token.GetGeneratedLocalizationKey() : Key), FText::FromString(Token.Content));

	FToken NextToken = InCtx.Stream.PeekToken();
	if (NextToken.Type == ESupertalkTokenType::Choice && NextToken.Indentation >= Token.Indentation)
	{
		// Choice uses a different operation + params, so pass our line off to it.
		return PaChoice(InCtx, OutAction, Line);
	}

	OutAction.Operation = ESupertalkOperation::Line;

	USupertalkPlayLineParams* Params = NewObject<USupertalkPlayLineParams>(InCtx.Script);
	Params->Line = Line;
	OutAction.Params = Params;
	
	return true;
}

bool FSupertalkParser::PaChoice(FPaContext& InCtx, FSupertalkAction& OutAction, FSupertalkLine& Line)
{
	USupertalkPlayChoiceParams* Params = NewObject<USupertalkPlayChoiceParams>(InCtx.Script);
	Params->Line = Line;
	
	FToken Token = InCtx.Stream.ReadToken();
	check(Token.Type == ESupertalkTokenType::Choice);
	while (Token.Type == ESupertalkTokenType::Choice)
	{
		FSupertalkChoice Choice;
		Choice.Text = FText::FromString(Token.Content);
		if (InCtx.Stream.PeekToken().Indentation >= Token.Indentation)
		{
			if (!PaAction(InCtx, Choice.SubAction))
			{
				return false;
			}
		}
		else
		{
			Choice.SubAction.Operation = ESupertalkOperation::Noop;
		}

		Params->Choices.Add(Choice);

		Token = InCtx.Stream.ReadToken();
	}

	InCtx.Stream.GoBack(1);

	OutAction.Operation = ESupertalkOperation::Choice;
	OutAction.Params = Params;

	return true;
}

bool FSupertalkParser::PaCommand(FPaContext& InCtx, FSupertalkAction& OutAction)
{
	FToken CommandToken = InCtx.Stream.ReadToken();
	check(CommandToken.Type == ESupertalkTokenType::Command);
	
	OutAction.Operation = ESupertalkOperation::Call;
	USupertalkCallParams* Params = NewObject<USupertalkCallParams>(InCtx.Script);
	Params->Arguments = CommandToken.Content;
	OutAction.Params = Params;
	
	return true;
}

bool FSupertalkParser::PaJump(FPaContext& InCtx, FSupertalkAction& OutAction)
{
	FToken Token = InCtx.Stream.ReadToken();
	check(Token.Type == ESupertalkTokenType::Jump);

	OutAction.Operation = ESupertalkOperation::Jump;
	USupertalkJumpParams* Params = NewObject<USupertalkJumpParams>(InCtx.Script);
	Params->JumpTarget = FName(Token.Content);
	OutAction.Params = Params;

	InCtx.Jumps.Add(TTuple<FToken, FName>(Token, Params->JumpTarget));

	return true;
}

bool FSupertalkParser::PaParallel(FPaContext& InCtx, FSupertalkAction& OutAction)
{
	FToken Token = InCtx.Stream.ReadToken();
	check(Token.Type == ESupertalkTokenType::ParallelStart);

	OutAction.Operation = ESupertalkOperation::Parallel;
	USupertalkParallelParams* Params = NewObject<USupertalkParallelParams>(InCtx.Script);
	OutAction.Params = Params;

	while (!InCtx.Stream.IsEOF())
	{
		Token = InCtx.Stream.ReadToken();
		if (Token.Type == ESupertalkTokenType::ParallelEnd)
		{
			break;
		}

		InCtx.Stream.GoBack(1);

		FSupertalkAction SubAction;
		if (!PaAction(InCtx, SubAction))
		{
			return false;
		}

		Params->SubActions.Add(SubAction);
	}

	if (Token.Type != ESupertalkTokenType::ParallelEnd)
	{
		ParseTokenError(InCtx, Token, ESupertalkTokenType::ParallelEnd);
		return false;
	}

	return true;
}

bool FSupertalkParser::PaQueue(FPaContext& InCtx, FSupertalkAction& OutAction)
{
	FToken Token = InCtx.Stream.ReadToken();
	check(Token.Type == ESupertalkTokenType::QueueStart);

	OutAction.Operation = ESupertalkOperation::Queue;
	USupertalkQueueParams* Params = NewObject<USupertalkQueueParams>(InCtx.Script);
	OutAction.Params = Params;

	while (!InCtx.Stream.IsEOF())
	{
		Token = InCtx.Stream.ReadToken();
		if (Token.Type == ESupertalkTokenType::QueueEnd)
		{
			break;
		}

		InCtx.Stream.GoBack(1);

		FSupertalkAction SubAction;
		if (!PaAction(InCtx, SubAction))
		{
			return false;
		}

		Params->SubActions.Add(SubAction);
	}

	if (Token.Type != ESupertalkTokenType::QueueEnd)
	{
		ParseTokenError(InCtx, Token, ESupertalkTokenType::QueueEnd);
		return false;
	}

	return true;
}

bool FSupertalkParser::PaValue(FPaContext& InCtx, TObjectPtr<USupertalkValue>& OutValue)
{
	FToken Token = InCtx.Stream.PeekToken();
	switch (Token.Type)
	{
	default:
		ParseTokenError(InCtx, Token, TEXT("Name, Text, Asset"));
		return false;

	case ESupertalkTokenType::Name:
		return PaVariableValue(InCtx, OutValue);

	case ESupertalkTokenType::LocalizationKey:
	case ESupertalkTokenType::Text:
		return PaTextValue(InCtx, OutValue);

	case ESupertalkTokenType::Asset:
		return PaAssetValue(InCtx, OutValue);
	}
}

bool FSupertalkParser::PaVariableValue(FPaContext& InCtx, TObjectPtr<USupertalkValue>& OutValue)
{
	FToken Token = InCtx.Stream.ReadToken();
	check(Token.Type == ESupertalkTokenType::Name);

	FName VarName = FName(Token.Content);
	if (VarName == NAME_None)
	{
		OutValue = nullptr;
		return true;
	}

	if (InCtx.Stream.PeekToken().Type == ESupertalkTokenType::Member)
	{
		USupertalkMemberValue* Member = NewObject<USupertalkMemberValue>(InCtx.Script);
		Member->Variable = VarName;

		while (!InCtx.Stream.IsEOF() && InCtx.Stream.PeekToken().Type == ESupertalkTokenType::Member)
		{
			Token = InCtx.Stream.ReadToken();
			check(Token.Type == ESupertalkTokenType::Member);

			Token = InCtx.Stream.ReadToken();
			if (Token.Type != ESupertalkTokenType::Name)
			{
				ParseTokenError(InCtx, Token, ESupertalkTokenType::Name);
				return false;
			}

			Member->Members.Add(FName(Token.Content));
		}

		OutValue = Member;
		return true;
	}
	else
	{
		USupertalkVariableValue* Variable = NewObject<USupertalkVariableValue>(InCtx.Script);
		Variable->Variable = VarName;
		OutValue = Variable;
		return true;
	}
}

bool FSupertalkParser::PaTextValue(FPaContext& InCtx, TObjectPtr<USupertalkValue>& OutValue)
{
	FToken Token = InCtx.Stream.ReadToken();
	FString Namespace = InCtx.DefaultNamespace;
	FString Key;
	if (Token.Type == ESupertalkTokenType::LocalizationKey)
	{
		if (!Token.Namespace.IsEmpty())
		{
			Namespace = Token.Namespace;
		}

		Key = Token.Content;

		Token = InCtx.Stream.ReadToken();
	}
	
	check(Token.Type == ESupertalkTokenType::Text);

	USupertalkTextValue* Text = NewObject<USupertalkTextValue>(InCtx.Script);
	Text->Text = FText::ChangeKey(FTextKey(Namespace), FTextKey(Key.IsEmpty() ? Token.GetGeneratedLocalizationKey() : Key), FText::FromString(Token.Content));
	OutValue = Text;
	return true;
}

bool FSupertalkParser::PaAssetValue(FPaContext& InCtx, TObjectPtr<USupertalkValue>& OutValue)
{
	FToken Token = InCtx.Stream.ReadToken();
	check(Token.Type == ESupertalkTokenType::Asset);

	UObject* Asset = LoadObject<UObject>(nullptr, *Token.Content);
	//UObject* Asset = FindObject<UObject>(nullptr, *Token.Content, false);
	if (Asset == nullptr)
	{
		ParseWarning(InCtx, Token, FText::Format(LOCTEXT("AssetNotFound", "Failed to find asset '{0}'"), FText::FromString(Token.Content)));
		return true;
	}
	
	USupertalkObjectValue* Value = NewObject<USupertalkObjectValue>(InCtx.Script);
	Value->Object = Asset;
	OutValue = Value;
	return true;
}

bool FSupertalkParser::PaAttributeList(FPaContext& InCtx, TArray<FSupertalkAttribute>& OutAttributes)
{
	FToken Token = InCtx.Stream.ReadToken();
	check(Token.Type == ESupertalkTokenType::AttrStart);

	while (!InCtx.Stream.IsEOF())
	{
		Token = InCtx.Stream.ReadToken();
		if (Token.Type != ESupertalkTokenType::Name)
		{
			ParseTokenError(InCtx, Token, ESupertalkTokenType::Name);
			return false;
		}

		FSupertalkAttribute Attr;
		Attr.Name = FName(Token.Content);

		OutAttributes.Add(Attr);

		Token = InCtx.Stream.ReadToken();
		switch (Token.Type)
		{
		default:
			InCtx.Stream.GoBack(1);
			break;

		case ESupertalkTokenType::Separator:
			break;

		case ESupertalkTokenType::AttrEnd:
			goto complete;
		}
	}

complete:
	if (Token.Type != ESupertalkTokenType::AttrEnd)
	{
		ParseTokenError(InCtx, Token, ESupertalkTokenType::AttrEnd);
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
