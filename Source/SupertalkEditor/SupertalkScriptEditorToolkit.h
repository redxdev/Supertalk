// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"

class FSupertalkScriptEditorToolkit : public FAssetEditorToolkit, public FGCObject
{
public:
	virtual ~FSupertalkScriptEditorToolkit();

	void Initialize(class USupertalkScript* InScriptAsset, const EToolkitMode::Type InMode, const TSharedPtr<class IToolkitHost>& InToolkitHost);

	virtual FString GetDocumentationLink() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	virtual FText GetBaseToolkitName() const override;
	virtual FName GetToolkitFName() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

private:
	void BindCommands();

	TSharedRef<SDockTab> HandleTabManagerSpawnTab(const FSpawnTabArgs& Args, FName TabIdentifier);

	void CompileScript();

	USupertalkScript* ScriptAsset = nullptr;
};