// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

class SSupertalkScriptAssetEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSupertalkScriptAssetEditor)
		:
		_IsReadOnly(false)
		{}
		SLATE_ARGUMENT(bool, IsReadOnly)
	SLATE_END_ARGS()

	SSupertalkScriptAssetEditor();
	virtual ~SSupertalkScriptAssetEditor();

	void Construct(const FArguments& InArgs, class USupertalkScript* InScriptAsset);

private:
	void HandleScriptAssetPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);
	void OnTextChanged(const FText& NewText);

	USupertalkScript* ScriptAsset = nullptr;


	FMessageLog MessageLog;
	TSharedPtr<SMultiLineEditableTextBox> TextBox;
};
