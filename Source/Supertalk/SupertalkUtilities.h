// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"

class USupertalkPlayer;

namespace FSupertalkUtilities
{
	SUPERTALK_API bool IsMemberExpression(const FString& Input);
	SUPERTALK_API FText FormatText(const FText& Format, const USupertalkPlayer* Player, bool bIsDisplayText = true);
}
