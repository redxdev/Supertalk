﻿// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
#include "SupertalkPlayer.h"
#include "Engine/DataTable.h"
#include "SupertalkValue.generated.h"

class USupertalkPlayer;

UCLASS(Abstract)
class SUPERTALK_API USupertalkValue : public UObject
{
	GENERATED_BODY()

public:
	const USupertalkValue* GetResolvedValue(const USupertalkPlayer* Player) const;
	FText ToResolvedDisplayText(const USupertalkPlayer* Player) const;
	FString ToResolvedInternalString(const USupertalkPlayer* Player) const;

	// Text meant for display to the user.
	virtual FText ToDisplayText() const PURE_VIRTUAL(USupertalkValue::ToDisplayText,return FText();)

	// Text meant for passing data around - i.e. object paths. This will eventually be removed
	// once we don't require string handling to call functions.
	virtual FString ToInternalString() const PURE_VIRTUAL(USupertalkValue::ToInternalText,return FString();)
	
	virtual const USupertalkValue* GetMember(FName MemberName) const PURE_VIRTUAL(USupertalkValue::GetMember,return nullptr;)

	virtual bool IsValueEqualTo(const USupertalkValue* Other) const { return this == Other; }

protected:
	virtual const USupertalkValue* ResolveValue(const USupertalkPlayer* Player) const;
};

UCLASS()
class SUPERTALK_API USupertalkBooleanValue : public USupertalkValue
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	uint8 bValue : 1;

	virtual FText ToDisplayText() const override;
	virtual FString ToInternalString() const override;
	virtual const USupertalkValue* GetMember(FName MemberName) const override;

	virtual bool IsValueEqualTo(const USupertalkValue* Other) const override;
};

UCLASS()
class SUPERTALK_API USupertalkTextValue : public USupertalkValue
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	FText Text;

	virtual FText ToDisplayText() const override;
	virtual FString ToInternalString() const override;
	virtual const USupertalkValue* GetMember(FName MemberName) const override;

	virtual bool IsValueEqualTo(const USupertalkValue* Other) const override;
};

UCLASS()
class SUPERTALK_API USupertalkVariableValue : public USupertalkValue
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	FName Variable;

	virtual FText ToDisplayText() const override;
	virtual FString ToInternalString() const override;
	virtual const USupertalkValue* GetMember(FName MemberName) const override;

	virtual bool IsValueEqualTo(const USupertalkValue* Other) const override { checkNoEntry(); return false; }

protected:
	virtual const USupertalkValue* ResolveValue(const USupertalkPlayer* Player) const override;
};

// This should be replaced with an expression at some point.
UCLASS()
class SUPERTALK_API USupertalkMemberValue : public USupertalkVariableValue
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	TArray<FName> Members;

	virtual FText ToDisplayText() const override;
	virtual FString ToInternalString() const override;
	virtual const USupertalkValue* GetMember(FName MemberName) const override;

	virtual bool IsValueEqualTo(const USupertalkValue* Other) const override { checkNoEntry(); return false; }

protected:
	virtual const USupertalkValue* ResolveValue(const USupertalkPlayer* Player) const override;
};

UINTERFACE()
class USupertalkDisplayInterface : public UInterface
{
	GENERATED_BODY()
};

class SUPERTALK_API ISupertalkDisplayInterface
{
	GENERATED_BODY()

public:
	// Get the text to be displayed for this object for a supertalk line.
	virtual FText GetSupertalkDisplayText() const;

	// Get a sub-member of this object.
	virtual const USupertalkValue* GetSupertalkMember(FName MemberName) const;
};

UCLASS()
class SUPERTALK_API USupertalkObjectValue : public USupertalkValue
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UObject> Object;

	virtual FText ToDisplayText() const override;
	virtual FString ToInternalString() const override;
	virtual const USupertalkValue* GetMember(FName MemberName) const override;

	virtual bool IsValueEqualTo(const USupertalkValue* Other) const override;
};

USTRUCT(BlueprintType)
struct SUPERTALK_API FSupertalkTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Value;

#if WITH_EDITORONLY_DATA

	// Notes for developers, stripped out of non-editor builds.
	UPROPERTY(EditAnywhere, meta = (MultiLine = true))
	FString Notes;

#endif
};