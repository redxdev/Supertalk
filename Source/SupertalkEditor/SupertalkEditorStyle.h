// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class FSupertalkEditorStyle
{
public:
	static void Initialize();
	static void Shutdown();

	static const ISlateStyle& Get();

	static const FName& GetStyleSetName();

private:
	static TSharedPtr<class FSlateStyleSet> StyleSet;
};