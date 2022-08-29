// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

class SSupertalkScriptAssetEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSupertalkScriptAssetEditor)
		{
		}

	SLATE_END_ARGS()

	virtual ~SSupertalkScriptAssetEditor();

	void Construct(const FArguments& InArgs, class USupertalkScript* InScriptAsset);

private:
	void HandleScriptAssetPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);

	USupertalkScript* ScriptAsset = nullptr;

	TSharedPtr<SMultiLineEditableTextBox> TextBox;
};
