// Copyright (c) MissiveArts LLC

#include "SupertalkExpression.h"
#include "SupertalkValue.h"
#include "SupertalkPlayer.h"

const USupertalkValue* ResolveExpression(USupertalkPlayer* Player, USupertalkExpression* Expr)
{
	if (IsValid(Expr))
	{
		const USupertalkValue* Value = Expr->Evaluate(Player);
		if (IsValid(Value))
		{
			return Value->GetResolvedValue(Player);
		}
	}

	return nullptr;
}

const USupertalkValue* USupertalkExpression_Value::Evaluate(USupertalkPlayer* Player)
{
	return Value;
}

const USupertalkValue* USupertalkExpression_Equality::Evaluate(USupertalkPlayer* Player)
{
	if (SubExpressions.Num() == 0)
	{
		return nullptr;
	}

	check(SubExpressions.Num() == Operations.Num() + 1);
	const USupertalkValue* Result = ResolveExpression(Player, SubExpressions[0]);
	for (int32 Idx = 1; Idx < SubExpressions.Num(); ++Idx)
	{
		USupertalkBooleanValue* BoolResult = NewObject<USupertalkBooleanValue>(Player);
		if (IsValid(Result))
		{
			BoolResult->bValue = Result->IsValueEqualTo(ResolveExpression(Player, SubExpressions[Idx]));
		}
		else
		{
			BoolResult->bValue = !ResolveExpression(Player, SubExpressions[Idx]);
		}

		if (Operations[Idx - 1] == ESupertalkExpression_Equality_Operation::NotEqual)
		{
			BoolResult->bValue = !BoolResult->bValue;
		}

		Result = BoolResult;
	}

	return Result;
}

const USupertalkValue* USupertalkExpression_Not::Evaluate(USupertalkPlayer* Player)
{
	const USupertalkValue* EvaluatedValue = IsValid(Value) ? Value->Evaluate(Player) : nullptr;
	EvaluatedValue = EvaluatedValue ? EvaluatedValue->GetResolvedValue(Player) : nullptr;

	bool Result;
	if (EvaluatedValue)
	{
		if (const USupertalkBooleanValue* BoolValue = Cast<USupertalkBooleanValue>(EvaluatedValue))
		{
			Result = BoolValue->bValue;
		}
		else
		{
			Result = true;
		}
	}
	else
	{
		Result = false;
	}

	Result = !Result;
	
	USupertalkBooleanValue* ResultValue = NewObject<USupertalkBooleanValue>(Player);
	ResultValue->bValue = Result;
	return ResultValue;
}
