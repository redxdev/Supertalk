// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"

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

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void HandleScriptAssetPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);
	void OnTextChanged(const FText& NewText);

	void RefreshLineNumbers();

	USupertalkScript* ScriptAsset = nullptr;


	FMessageLog MessageLog;
	TSharedPtr<class SMultiLineEditableTextWithScrollExposed> EditableText;

	// This method of implementing line numbers is horrible and will hopefully be replaced one day... maybe.
	TSharedPtr<STextBlock> LineNumbers;
	TSharedPtr<SScrollBox> LineNumbersScroll;
	FString NumberCache;
	int32 CacheLen = 0;
	int32 PreviousNumbers = 0;
};
