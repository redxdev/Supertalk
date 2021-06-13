// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
#include "SupertalkLine.generated.h"

USTRUCT(BlueprintType)
struct SUPERTALK_API FSupertalkAttribute
{
	GENERATED_BODY()
	
	UPROPERTY()
	FName Name;
};

USTRUCT(BlueprintType)
struct SUPERTALK_API FSupertalkLine
{
	GENERATED_BODY()

	FSupertalkLine()
	{
		Speaker = nullptr;
		SpeakerNameOverride = nullptr;
		bIsBlankLine = false;
	}

	UPROPERTY()
	TObjectPtr<class USupertalkValue> Speaker;

	UPROPERTY()
	TObjectPtr<class USupertalkValue> SpeakerNameOverride;

	UPROPERTY()
	TArray<FSupertalkAttribute> Attributes;

	UPROPERTY()
	FText Text;

	// "Blank" doesn't mean that Text is blank, but that the line should not cause a full line to appear.
	// This can be useful for applying attributes without actually playing a line.
	UPROPERTY()
	uint32 bIsBlankLine : 1;

	FText GetSpeakerName(const class USupertalkPlayer* Player) const;
	FText FormatText(const class USupertalkPlayer* Player) const;
};