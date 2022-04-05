// Copyright (c) MissiveArts LLC

#include "SupertalkLine.h"
#include "SupertalkPlayer.h"
#include "SupertalkUtilities.h"
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

FText FSupertalkLine::FormatText(const USupertalkPlayer* Player) const
{
	return FSupertalkUtilities::FormatText(Text, Player, true);
}
