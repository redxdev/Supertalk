// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
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

	virtual FText ToDisplayText() const PURE_VIRTUAL(USupertalkValue::ToDisplayText,return FText();)
	virtual const USupertalkValue* GetMember(FName MemberName) const PURE_VIRTUAL(USupertalkValue::GetMember,return nullptr;)

protected:
	virtual const USupertalkValue* ResolveValue(const USupertalkPlayer* Player) const;
};

UCLASS()
class SUPERTALK_API USupertalkTextValue : public USupertalkValue
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	FText Text;

	virtual FText ToDisplayText() const override;
	virtual const USupertalkValue* GetMember(FName MemberName) const override;
};

UCLASS()
class SUPERTALK_API USupertalkVariableValue : public USupertalkValue
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	FName Variable;

	virtual FText ToDisplayText() const override;
	virtual const USupertalkValue* GetMember(FName MemberName) const override;

protected:
	virtual const USupertalkValue* ResolveValue(const USupertalkPlayer* Player) const override;
};

UCLASS()
class SUPERTALK_API USupertalkMemberValue : public USupertalkVariableValue
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	TArray<FName> Members;

	virtual FText ToDisplayText() const override;
	virtual const USupertalkValue* GetMember(FName MemberName) const override;

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
	virtual const USupertalkValue* GetMember(FName MemberName) const override;
};

USTRUCT(BlueprintType)
struct SUPERTALK_API FSupertalkTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Value;
};