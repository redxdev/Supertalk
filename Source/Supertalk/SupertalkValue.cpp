// Copyright (c) MissiveArts LLC

#include "SupertalkValue.h"

#include "Supertalk.h"
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

FString USupertalkValue::ToResolvedInternalString(const USupertalkPlayer* Player) const
{
	const USupertalkValue* Value = GetResolvedValue(Player);
	if (IsValid(Value))
	{
		return Value->ToInternalString();
	}

	return ToInternalString();
}

bool USupertalkValue::PropertyToValue(USupertalkPlayer* Player, void* ValuePtr, UObject* Owner, FProperty* Property, bool bValueIsContainer, USupertalkValue*& OutResult)
{
	check(ValuePtr);
	check(Property);

	UObject* Outer = IsValid(Player) ? static_cast<UObject*>(Player) : static_cast<UObject*>(GetTransientPackage());

	if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Property))
	{
		USupertalkObjectValue* Value = NewObject<USupertalkObjectValue>(Outer);
		Value->Object = bValueIsContainer ? ObjProp->GetObjectPropertyValue_InContainer(ValuePtr) : ObjProp->GetObjectPropertyValue(ValuePtr);
		OutResult = Value;
		return true;
	}
	else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		USupertalkBooleanValue* Value = NewObject<USupertalkBooleanValue>(Outer);
		Value->bValue = bValueIsContainer ? BoolProp->GetPropertyValue_InContainer(ValuePtr) : BoolProp->GetPropertyValue(ValuePtr);
		OutResult = Value;
		return true;
	}
	else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
	{
		USupertalkTextValue* Value = NewObject<USupertalkTextValue>(Outer);
		Value->Text = bValueIsContainer ? TextProp->GetPropertyValue_InContainer(ValuePtr) : TextProp->GetPropertyValue(ValuePtr);
		OutResult = Value;
		return true;
	}
	else if (FMapProperty* MapProp = CastField<FMapProperty>(Property))
	{
		USupertalkMapPropertyValue* Value = NewObject<USupertalkMapPropertyValue>(Outer);
		Value->Owner = Owner;
		Value->TargetProperty = MapProp;
		OutResult = Value;
		return true;
	}
	else
	{
		FString Str;
		if (bValueIsContainer)
		{
			Property->ExportText_InContainer(0, Str, ValuePtr, ValuePtr, nullptr, PPF_None);
		}
		else
		{
			Property->ExportText_Direct(Str, ValuePtr, ValuePtr, nullptr, PPF_None);
		}

		USupertalkTextValue* Value = NewObject<USupertalkTextValue>(Outer);
		Value->Text = FText::FromString(Str);
		OutResult = Value;
		return true;
	}

	return false;
}

const USupertalkValue* USupertalkValue::ResolveValue(const USupertalkPlayer* Player) const
{
	return nullptr;
}

FText USupertalkBooleanValue::ToDisplayText() const
{
	return bValue ? LOCTEXT("True", "true") : LOCTEXT("False", "false");
}

FString USupertalkBooleanValue::ToInternalString() const
{
	return bValue ? TEXT("1") : TEXT("0");
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

FString USupertalkTextValue::ToInternalString() const
{
	FString Result = Text.ToString();
	Result.ReplaceCharWithEscapedCharInline();
	return Result;
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

FString USupertalkVariableValue::ToInternalString() const
{
	checkNoEntry();
	return ToDisplayText().ToString();
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

FString USupertalkMemberValue::ToInternalString() const
{
	checkNoEntry();
	return ToDisplayText().ToString();
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

FString USupertalkObjectValue::ToInternalString() const
{
	return IsValid(Object) ? Object.GetPath() : TEXT("None");
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
		else if (FProperty* Prop = Object->GetClass()->FindPropertyByName(MemberName))
		{
			USupertalkValue* Value;
			if (PropertyToValue(Cast<USupertalkPlayer>(GetOuter()), Object, Object, Prop, true, Value))
			{
				return Value;
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

FText USupertalkMapPropertyValue::ToDisplayText() const
{
	return FText::FromString(ToInternalString());
}

FString USupertalkMapPropertyValue::ToInternalString() const
{
	// Is there a string representation for maps?
	return IsValid(Owner) ?
		FString::Printf(TEXT("%s.%s"), *Owner->GetName(), TargetProperty ? *TargetProperty->GetName() : TEXT("None"))
	:   TEXT("None");
}

const USupertalkValue* USupertalkMapPropertyValue::GetMember(FName MemberName) const
{
	if (!IsValid(Owner) || !TargetProperty)
	{
		return nullptr;
	}

	FScriptMapHelper Helper(TargetProperty, TargetProperty->ContainerPtrToValuePtr<uint8>(Owner));
	if (FNameProperty* NameProp = CastField<FNameProperty>(TargetProperty->KeyProp))
	{
		if (uint8* ValuePtr = Helper.FindValueFromHash(&MemberName))
		{
			USupertalkValue* Result = nullptr;
			if (PropertyToValue(Cast<USupertalkPlayer>(GetOuter()), ValuePtr, Owner, TargetProperty->ValueProp, false, Result))
			{
				return Result;
			}
		}

		return nullptr;
	}
	else if (FStrProperty* StrProp = CastField<FStrProperty>(TargetProperty->KeyProp))
	{
		FString Key = MemberName.ToString();
		if (uint8* ValuePtr = Helper.FindValueFromHash(&Key))
		{
			USupertalkValue* Result = nullptr;
			if (PropertyToValue(Cast<USupertalkPlayer>(GetOuter()), ValuePtr, Owner, TargetProperty->ValueProp, false, Result))
			{
				return Result;
			}
		}
	}
	else if (FTextProperty* TextProp = CastField<FTextProperty>(TargetProperty->KeyProp))
	{
		FText Key = FText::FromString(MemberName.ToString());
		if (uint8* ValuePtr = Helper.FindValueFromHash(&Key))
		{
			USupertalkValue* Result = nullptr;
			if (PropertyToValue(Cast<USupertalkPlayer>(GetOuter()), ValuePtr, Owner, TargetProperty->ValueProp, false, Result))
			{
				return Result;
			}
		}
	}

	UE_LOG(LogSupertalk, Warning, TEXT("Unable to access member '%s' of '%s.%s': key type of map is not a name, string, or text."), *MemberName.ToString(), *Owner->GetName(), *TargetProperty->GetName());
	return nullptr;
}

bool USupertalkMapPropertyValue::IsValueEqualTo(const USupertalkValue* Other) const
{
	if (!IsValid(Other))
	{
		return false;
	}

	if (const USupertalkMapPropertyValue* OtherObj = Cast<USupertalkMapPropertyValue>(Other))
	{
		return Owner == OtherObj->Owner && TargetProperty == OtherObj->TargetProperty;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
