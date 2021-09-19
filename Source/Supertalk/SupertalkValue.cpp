// Copyright (c) MissiveArts LLC

#include "SupertalkValue.h"
#include "SupertalkPlayer.h"

#define LOCTEXT_NAMESPACE "SupertalkValue"

const USupertalkValue* USupertalkValue::GetResolvedValue(const USupertalkPlayer* Player) const
{
	check(Player);

	const USupertalkValue* GoodValue = this;
	const USupertalkValue* Value = this;
	while (IsValid(Value))
	{
		GoodValue = Value;
		Value = Value->ResolveValue(Player);
	}

	return GoodValue;
}

FText USupertalkValue::ToResolvedDisplayText(const USupertalkPlayer* Player) const
{
	const USupertalkValue* Value = GetResolvedValue(Player);
	if (IsValid(Value))
	{
		return Value->ToDisplayText();
	}

	return ToDisplayText();
}

const USupertalkValue* USupertalkValue::ResolveValue(const USupertalkPlayer* Player) const
{
	return nullptr;
}

FText USupertalkBooleanValue::ToDisplayText() const
{
	return bValue ? LOCTEXT("True", "true") : LOCTEXT("False", "false");
}

const USupertalkValue* USupertalkBooleanValue::GetMember(FName MemberName) const
{
	return nullptr;
}

bool USupertalkBooleanValue::IsValueEqualTo(const USupertalkValue* Other) const
{
	if (!IsValid(Other))
	{
		return false;
	}

	if (const USupertalkBooleanValue* OtherBool = Cast<USupertalkBooleanValue>(Other))
	{
		return bValue == OtherBool->bValue;
	}

	return true;
}

FText USupertalkTextValue::ToDisplayText() const
{
	return Text;
}

const USupertalkValue* USupertalkTextValue::GetMember(FName MemberName) const
{
	return nullptr;
}

bool USupertalkTextValue::IsValueEqualTo(const USupertalkValue* Other) const
{
	if (!IsValid(Other))
	{
		return false;
	}

	if (const USupertalkTextValue* OtherText = Cast<USupertalkTextValue>(Other))
	{
		return Text.EqualTo(OtherText->Text);
	}

	return true;
}

FText USupertalkVariableValue::ToDisplayText() const
{
	return FText::FromName(Variable);
}

const USupertalkValue* USupertalkVariableValue::GetMember(FName MemberName) const
{
	return nullptr;
}

const USupertalkValue* USupertalkVariableValue::ResolveValue(const USupertalkPlayer* Player) const
{
	return Player->GetVariable(Variable);
}

FText USupertalkMemberValue::ToDisplayText() const
{
	FString Str = Super::ToDisplayText().ToString();
	for (FName Member : Members)
	{
		Str += TEXT(".") + Member.ToString();
	}

	return FText::FromString(Str);
}

const USupertalkValue* USupertalkMemberValue::GetMember(FName MemberName) const
{
	USupertalkMemberValue* NewMember = DuplicateObject<USupertalkMemberValue>(this, GetOuter());
	NewMember->Members.Add(MemberName);
	return NewMember;
}

const USupertalkValue* USupertalkMemberValue::ResolveValue(const USupertalkPlayer* Player) const
{
	const USupertalkValue* CurrentValue = Super::ResolveValue(Player);
	if (!IsValid(CurrentValue))
	{
		return nullptr;
	}

	for (FName Member : Members)
	{
		CurrentValue = CurrentValue->GetMember(Member);
		if (!IsValid(CurrentValue))
		{
			return nullptr;
		}

		CurrentValue = CurrentValue->GetResolvedValue(Player);
		if (!IsValid(CurrentValue))
		{
			return nullptr;
		}
	}

	return CurrentValue;
}

FText ISupertalkDisplayInterface::GetSupertalkDisplayText() const
{
	return FText();
}

const USupertalkValue* ISupertalkDisplayInterface::GetSupertalkMember(FName MemberName) const
{
	return nullptr;
}

FText USupertalkObjectValue::ToDisplayText() const
{
	if (IsValid(Object))
	{
		if (ISupertalkDisplayInterface* DisplayInterface = Cast<ISupertalkDisplayInterface>(Object))
		{
			return DisplayInterface->GetSupertalkDisplayText();
		}
		
		return FText::FromString(Object->GetName());
	}

	return FText();
}

const USupertalkValue* USupertalkObjectValue::GetMember(FName MemberName) const
{
	if (IsValid(Object))
	{
		if (ISupertalkDisplayInterface* DisplayInterface = Cast<ISupertalkDisplayInterface>(Object))
		{
			return DisplayInterface->GetSupertalkMember(MemberName);
		}
		else if (UDataTable* DataTable = Cast<UDataTable>(Object))
		{
			// TODO: Can't subclass UDataTable for now, so we have to handle it separately :(
			FSupertalkTableRow* Row = DataTable->FindRow<FSupertalkTableRow>(MemberName, TEXT("SupertalkObjectValue::GetMember"));
			if (Row != nullptr)
			{
				USupertalkTextValue* TextValue = NewObject<USupertalkTextValue>();
				TextValue->Text = Row->Value;
				return TextValue;
			}
		}
	}

	return nullptr;
}

bool USupertalkObjectValue::IsValueEqualTo(const USupertalkValue* Other) const
{
	if (!IsValid(Other))
	{
		return false;
	}

	if (const USupertalkObjectValue* OtherObj = Cast<USupertalkObjectValue>(Other))
	{
		return Object == OtherObj->Object;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
