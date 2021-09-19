// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
#include "SupertalkExpression.generated.h"

UCLASS(Abstract)
class SUPERTALK_API USupertalkExpression : public UObject
{
	GENERATED_BODY()

public:
	virtual const class USupertalkValue* Evaluate(class USupertalkPlayer* Player) PURE_VIRTUAL(USupertalkExpression::Evaluate,return nullptr;)
};

UCLASS()
class SUPERTALK_API USupertalkExpression_Value : public USupertalkExpression
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<class USupertalkValue> Value;
	
	virtual const USupertalkValue* Evaluate(class USupertalkPlayer* Player) override;
};

UENUM()
enum class ESupertalkExpression_Equality_Operation
{
	Equal,
	NotEqual
};

UCLASS()
class SUPERTALK_API USupertalkExpression_Equality : public USupertalkExpression
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<TObjectPtr<USupertalkExpression>> SubExpressions;

	UPROPERTY()
	TArray<ESupertalkExpression_Equality_Operation> Operations;

	virtual const class USupertalkValue* Evaluate(class USupertalkPlayer* Player) override;
};

UCLASS()
class SUPERTALK_API USupertalkExpression_Not : public USupertalkExpression
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<USupertalkExpression> Value;

	virtual const class USupertalkValue* Evaluate(USupertalkPlayer* Player) override;
};