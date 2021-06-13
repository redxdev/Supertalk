// Copyright (c) MissiveArts LLC

#include "SupertalkLine.h"
#include "Supertalk.h"
#include "SupertalkPlayer.h"
#include "SupertalkValue.h"
#include "Internationalization/TextFormatter.h"

FText FSupertalkLine::GetSpeakerName(const USupertalkPlayer* Player) const
{
	check(Player);
	
	if (IsValid(SpeakerNameOverride))
	{
		return SpeakerNameOverride->ToResolvedDisplayText(Player);
	}

	if (IsValid(Speaker))
	{
		return Speaker->ToResolvedDisplayText(Player);
    }

	return FText();
}

bool IsMemberExpression(const FString& Input)
{
	return Input.Contains(TEXT("."));
}

FText FSupertalkLine::FormatText(const USupertalkPlayer* Player) const
{
	check(Player);
	
	FTextFormat Fmt(Text);
	
	TArray<FString> ParameterNames;
	FText::GetFormatPatternParameters(Fmt, ParameterNames);

	FFormatNamedArguments FormatArgs;
	for (const FString& Param : ParameterNames)
	{
		if (IsMemberExpression(Param))
		{
			TArray<FString> MemberStrings;
			Param.ParseIntoArrayWS(MemberStrings, TEXT("."));
			check(MemberStrings.Num() > 0);

			FName VarName = FName(MemberStrings[0]);
			MemberStrings.RemoveAt(0);

			USupertalkMemberValue* MemberValue = NewObject<USupertalkMemberValue>();
			MemberValue->Variable = VarName;
			for (const FString& Member : MemberStrings)
			{
				MemberValue->Members.Add(FName(Member));
			}

			FormatArgs.Add(Param, MemberValue->ToResolvedDisplayText(Player));
		}
		else
		{
			FName VarName(Param);
			const USupertalkValue* Value = Player->GetVariable(VarName);
			if (Value)
			{
				FormatArgs.Add(Param, Value->ToResolvedDisplayText(Player));
			}
			else
			{
				FormatArgs.Add(Param, FText::FromName(VarName));
			}
		}
	}
	
	return FText::Format(Fmt, FormatArgs);
}
