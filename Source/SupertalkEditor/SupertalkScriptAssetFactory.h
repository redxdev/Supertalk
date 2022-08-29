// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "EditorReimportHandler.h"
#include "SupertalkScriptAssetFactory.generated.h"

UCLASS(HideCategories=(Object))
class USupertalkScriptAssetFactory : public UFactory, public FReimportHandler
{
	GENERATED_BODY()

public:
	USupertalkScriptAssetFactory();
	
	virtual FText GetDisplayName() const override;
	virtual bool FactoryCanImport(const FString& Filename) override;

	virtual UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn) override;

	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
};

class FAssetTypeActions_SupertalkScript : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override;
	virtual bool IsImportedAsset() const override;
	virtual void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;
};