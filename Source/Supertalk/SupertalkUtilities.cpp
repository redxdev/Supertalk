// Copyright (c) MissiveArts LLC

#include "SupertalkUtilities.h"
#include "SupertalkPlayer.h"
#include "SupertalkValue.h"

namespace FSupertalkUtilities
{
	bool IsMemberExpression(const FString& Input)
	{
		return Input.Contains(TEXT("."));
	}

	FText FormatText(const FText& Format, const USupertalkPlayer* Player, bool bIsDisplayText)
	{
		check(Player);

		TArray<FString> ParameterNames;
		FText::GetFormatPatternParameters(Format, ParameterNames);

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

				if (bIsDisplayText)
				{
					FormatArgs.Add(Param, MemberValue->ToResolvedDisplayText(Player));
				}
				else
				{
					FormatArgs.Add(Param, FText::FromString(MemberValue->ToResolvedInternalString(Player)));
				}
			}
			else
			{
				FName VarName(Param);
				const USupertalkValue* Value = Player->GetVariable(VarName);
				if (Value)
				{
					if (bIsDisplayText)
					{
						FormatArgs.Add(Param, Value->ToResolvedDisplayText(Player));
					}
					else
					{
						FormatArgs.Add(Param, FText::FromString(Value->ToResolvedInternalString(Player)));
					}
				}
				else
				{
					FormatArgs.Add(Param, FText::FromName(VarName));
				}
			}
		}
	
		return FText::Format(Format, FormatArgs);
	}
}
