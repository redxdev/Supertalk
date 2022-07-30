// Copyright (c) MissiveArts LLC

#include "SupertalkPlayer.h"
#include "Supertalk.h"
#include "SupertalkExpression.h"
#include "SupertalkUtilities.h"
#include "SupertalkValue.h"
#include "EditorFramework/AssetImportData.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "Supertalk"

#if WITH_EDITORONLY_DATA
void USupertalkScript::PostInitProperties()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}

	Super::PostInitProperties();
}

void USupertalkScript::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	if (AssetImportData)
	{
		OutTags.Add(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
	}

	Super::GetAssetRegistryTags(OutTags);
}

void USupertalkScript::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading() && Ar.UEVer() < VER_UE4_ASSET_IMPORT_DATA_AS_JSON && !AssetImportData)
	{
		// AssetImportData should always be valid
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
}
#endif

#if WITH_EDITOR
void USupertalkScript::OpenSourceFileInExternalProgram()
{
	if (AssetImportData)
	{
		const FString Filename = AssetImportData->GetFirstFilename();
		if (FPaths::FileExists(Filename))
		{
			FPlatformProcess::LaunchFileInDefaultExternalApplication(*Filename);
		}
	}
}
#endif

void USupertalkAssignParams::PostLoad()
{
	Super::PostLoad();

	if (IsValid(Value_DEPRECATED))
	{
		if (!IsValid(Expression))
		{
			USupertalkExpression_Value* ValueExpr = NewObject<USupertalkExpression_Value>(Value_DEPRECATED->GetOuter());
			ValueExpr->Value = Value_DEPRECATED;
			Expression = ValueExpr;
		}
		else
		{
			UE_LOG(LogSupertalk, Warning, TEXT("USupertalkAssignParams::PostLoad() - Both Value_DEPRECATED and Expression are set, removing Value_DEPRECATED."));
		}

		Value_DEPRECATED = nullptr;
	}
}

void USupertalkConditionalParams::PostLoad()
{
	Super::PostLoad();

	if (IsValid(Value_DEPRECATED))
	{
		if (!IsValid(Expression))
		{
			USupertalkExpression_Value* ValueExpr = NewObject<USupertalkExpression_Value>(Value_DEPRECATED->GetOuter());
			ValueExpr->Value = Value_DEPRECATED;
			Expression = ValueExpr;
		}
		else
		{
			UE_LOG(LogSupertalk, Warning, TEXT("USupertalkConditionalParams::PostLoad() - Both Value_DEPRECATED and Expression are set, removing Value_DEPRECATED."));
		}

		Value_DEPRECATED = nullptr;
	}
}

USupertalkPlayer::USupertalkPlayer()
{
	NextActionId = 1;
	NextStackId = 1;
}

void USupertalkPlayer::SetVariable(FName Name, const USupertalkValue* Value)
{
	if (!IsValid(Value))
	{
		Variables.Remove(Name);
	}
	else
	{
		// We don't allow aliasing (pointers/references), so always resolve values.
		// TODO: can we store const TObjectPtrs? Is that a thing?
		Variables.Add(Name, const_cast<USupertalkValue*>(Value->GetResolvedValue(this)));
	}
}

void USupertalkPlayer::SetVariable(FName Name, bool Value)
{
	USupertalkBooleanValue* BoolValue = NewObject<USupertalkBooleanValue>(this);
	BoolValue->bValue = Value;
	SetVariable(Name, BoolValue);
}

void USupertalkPlayer::SetVariable(FName Name, FText Value)
{
	USupertalkTextValue* TextValue = NewObject<USupertalkTextValue>(this);
	TextValue->Text = Value;
	SetVariable(Name, TextValue);
}

const USupertalkValue* USupertalkPlayer::GetVariable(FName Name) const
{
	const TObjectPtr<USupertalkValue>* VarValue = Variables.Find(Name);
	if (VarValue != nullptr)
	{
		return *VarValue;
	}

	for (const FSupertalkVariableProviderObject& Provider : VariableProviderObjects)
	{
		if (!Provider.Object)
		{
			continue;
		}

		UClass* Class = Provider.Object->GetClass();
		if (FProperty* Property = Class->FindPropertyByName(Name))
		{
			UClass* OwnerClass = Property->GetOwnerClass();
			if (Provider.ClassFilter && (!OwnerClass || OwnerClass == Provider.ClassFilter || !OwnerClass->IsChildOf(Provider.ClassFilter)))
			{
				continue;
			}

			USupertalkValue* Value = nullptr;
			if (USupertalkValue::PropertyToValue(const_cast<USupertalkPlayer*>(this), Provider.Object, Provider.Object, Property, true, Value))
			{
				return Value;
			}
		}
	}

	for (const FSupertalkProvideVariableDelegate& Provider : VariableProviderDelegates)
	{
		if (Provider.IsBound())
		{
			const USupertalkValue* Result = Provider.Execute(this, Name);
			if (Result != nullptr)
			{
				return Result;
			}
		}
	}

	return nullptr;
}

void USupertalkPlayer::ClearVariables()
{
	Variables.Empty();
}

void USupertalkPlayer::AddFunctionCallReceiver(UObject* Obj)
{
	check(Obj);
	FunctionCallReceivers.AddUnique(Obj);
}

void USupertalkPlayer::AddVariableProvider(FSupertalkProvideVariableDelegate Provider)
{
	check(Provider.IsBound());
	VariableProviderDelegates.Add(Provider);
}

void USupertalkPlayer::AddVariableProvider(UObject* Object, UClass* ClassFilter)
{
	if (ensure(Object))
	{
		check(!ClassFilter || Object->IsA(ClassFilter));
		VariableProviderObjects.Add({ Object, ClassFilter });
	}
}

void USupertalkPlayer::RunScript(const USupertalkScript* Script, FName InitialSection)
{
	FMessageLog MessageLog(SupertalkMessageLogName);
	
	if (ensureAlwaysMsgf(!IsRunningScript(), TEXT("Cannot run a script on a USupertalkPlayer when a script is already running")))
	{
		if (!IsValid(Script))
		{
			UE_LOG(LogSupertalk, Error, TEXT("Cannot run invalid script"));
			return;
		}
	
		if (InitialSection == NAME_None)
		{
			InitialSection = Script->DefaultSection;
		}

		const FSupertalkSection* Section = Script->Sections.Find(InitialSection);
		if (Section == nullptr)
		{
			MessageLog.Error(FText::Format(LOCTEXT("UnknownSectionError", "Unable to find section '{0}' in script '{1}"), FText::FromName(InitialSection), FText::FromString(Script->GetName())));
			return;
		}
	
		if (Section->Actions.Num() == 0)
		{
			MessageLog.Warning(FText::Format(LOCTEXT("NoActionsWarning", "Section '{0}' in script '{1}' has no actions"), FText::FromName(Section->Name), FText::FromString(Script->GetName())));
			return;
		}
		
		FSupertalkStack& Stack = CreateNewStack();
		
		PushActions(Stack.StackId, Script, Section->Actions);
		TickStack(Stack.StackId);
	}
	else
	{
		MessageLog.Error(LOCTEXT("ScriptAlreadyRunningError", "Cannot run a script on a USupertalkPlayer when a script is already running"));
	}
}

void USupertalkPlayer::Stop()
{
	Stacks.Empty();
}

FSupertalkLatentFunctionFinalizer USupertalkPlayer::MakeLatentFunction()
{
	if (CurrentFunctionFinalizer && !bIsFunctionCallLatent)
	{
		bIsFunctionCallLatent = true;
		return *CurrentFunctionFinalizer;
	}

	return FSupertalkLatentFunctionFinalizer();
}

void USupertalkPlayer::CompleteFunction(FSupertalkLatentFunctionFinalizer Finalizer)
{
	Finalizer.Complete();
}

void USupertalkPlayer::OnPlayLine(const FSupertalkLine& Line, FSupertalkEventCompletedDelegate Completed)
{
	FMessageLog MessageLog(SupertalkMessageLogName);
	
	if (OnPlayLineEvent.IsBound())
	{
		OnPlayLineEvent.Execute(Line, Completed);
	}
	else
	{
		MessageLog.Warning(LOCTEXT("OnPlayLineUnimplemented", "OnPlayLine has not been implemented"));
		Completed.ExecuteIfBound();
	}
}

void USupertalkPlayer::OnPlayChoice(const FSupertalkLine& Line, const TArray<FText>& Choices, FSupertalkChoiceCompletedDelegate Completed)
{
	FMessageLog MessageLog(SupertalkMessageLogName);
	
	if (OnPlayChoiceEvent.IsBound())
	{
		OnPlayChoiceEvent.Execute(Line, Choices, Completed);
	}
	else
	{
		MessageLog.Warning(LOCTEXT("OnPlayChoiceUnimplemented", "OnPlayChoice has not been implemented"));
		Completed.ExecuteIfBound(INDEX_NONE);
	}
}

FSupertalkStack& USupertalkPlayer::CreateNewStack()
{
	FSupertalkStack Stack;
	Stack.StackId = GetNewStackId();
	return Stacks.Add(Stack.StackId, Stack);
}

uint32 USupertalkPlayer::GetNewActionId()
{
	const uint32 NewId = NextActionId++;
	if (NextActionId == 0)
	{
		NextActionId = 1;
	}

	return NewId;
}

uint32 USupertalkPlayer::GetNewStackId()
{
	const uint32 NewId = NextStackId++;
	if (NextStackId == 0)
	{
		NextStackId = 1;
	}

	return NewId;
}

void USupertalkPlayer::PushActions(uint32 StackId, const USupertalkScript* Script, const TArray<FSupertalkAction>& Actions)
{
	check(Script);
	check(Actions.Num() > 0);

	FSupertalkStack& Stack = Stacks.FindChecked(StackId);
	Stack.QueuedActions.Reserve(Stack.QueuedActions.Num() + Actions.Num());
	for (int32 Idx = Actions.Num() - 1; Idx >= 0; --Idx)
	{
		const FSupertalkAction& Action = Actions[Idx]; 
		PushAction(Stack, Script, Action);
	}
}

void USupertalkPlayer::PushAction(FSupertalkStack& Stack, const USupertalkScript* Script, const FSupertalkAction& Action)
{
	check(Script);

	FSupertalkActionWithContext Context;
	Context.Source = Script;
	Context.Action = Action;
	Context.Key.StackId = Stack.StackId;
	Context.Key.ActionId = GetNewActionId();
	Stack.QueuedActions.Add(Context);
}

void USupertalkPlayer::PushAction(uint32 StackId, const USupertalkScript* Script, const FSupertalkAction& Action)
{
	check(Script);

	FSupertalkStack& Stack = Stacks.FindChecked(StackId);
	PushAction(Stack, Script, Action);
}

void USupertalkPlayer::CompleteActionAndTick(FSupertalkActionKey Key)
{
	CompleteAction(Key);
	TickStack(Key.StackId);
}

void USupertalkPlayer::CompleteAction(FSupertalkActionKey Key)
{
	FSupertalkStack* Stack = Stacks.Find(Key.StackId);
	if (Stack == nullptr)
	{
		UE_LOG(LogSupertalk, Warning, TEXT("CompleteAction called with unknown stack id %u"), Key.StackId);
		return;
	}

	if (!Key.IsValid() || Stack->ActiveAction.Key != Key)
	{
		UE_LOG(LogSupertalk, Warning, TEXT("CompleteAction called with unknown action id %u (stack %u expects %u)"), Key.ActionId, Key.StackId, Stack->ActiveAction.Key.ActionId);
		return;
	}

	Stack->ActiveAction = FSupertalkActionWithContext();
}

void USupertalkPlayer::TickStack(uint32 StackId)
{
	FSupertalkStack* Stack = Stacks.Find(StackId);
	if (Stack == nullptr)
	{
		UE_LOG(LogSupertalk, Error, TEXT("TickStack called with unknown stack id %u"), StackId);
		return;
	}
	
	// Prevent recursive ticking
	if (Stack->bIsTicking)
	{
		return;
	}

	Stack->bIsTicking = true;
	
	while (Stack != nullptr && !Stack->ActiveAction.Key.IsValid() && Stack->WaitingOn.Num() == 0)
	{
		if (Stack->QueuedActions.Num() > 0)
		{
			Stack->ActiveAction = Stack->QueuedActions.Last();
			Stack->QueuedActions.RemoveAt(Stack->QueuedActions.Num() - 1);
		
			ExecuteAction(Stack->ActiveAction);
		}
		else
		{
			uint32 ExitingId = Stack->StackId;
			uint32 WaitingId = Stack->SourceId;
			
			Stacks.Remove(StackId);
			Stack = nullptr;
			
			if (WaitingId != 0)
			{
				FinishWaitingOnStack(ExitingId, WaitingId);
			}

			break;
		}

		// Need to re-find just in case Stacks was modified during execution.
		Stack = Stacks.Find(StackId);
	}

	if (Stack != nullptr)
	{
		Stack->bIsTicking = false;
	}
}

void USupertalkPlayer::ExecuteAction(const FSupertalkActionWithContext& Context)
{
	check(Context.Source);
	check(Stacks.Contains(Context.Key.StackId));
	check(Stacks[Context.Key.StackId].ActiveAction.Key == Context.Key);

	switch (Context.Action.Operation)
	{
	default:
		checkNoEntry();
		// fall-through on purpose, count this as a no-op if checks are disabled.

	case ESupertalkOperation::Noop:
		CompleteAction(Context.Key);
		break;

	case ESupertalkOperation::Line:
		HandlePlayLine(Context);
		break;

	case ESupertalkOperation::Choice:
		HandlePlayChoice(Context);
		break;

	case ESupertalkOperation::Assign:
		HandleAssign(Context);
		break;

	case ESupertalkOperation::Call:
		HandleCall(Context);
		break;

	case ESupertalkOperation::Jump:
		HandleJump(Context);
		break;

	case ESupertalkOperation::Parallel:
		HandleParallel(Context);
		break;

	case ESupertalkOperation::Queue:
		HandleQueue(Context);
		break;

	case ESupertalkOperation::Conditional:
		HandleConditional(Context);
		break;
	}
}

void USupertalkPlayer::HandlePlayLine(const FSupertalkActionWithContext& Context)
{
	USupertalkPlayLineParams* Params = CastChecked<USupertalkPlayLineParams>(Context.Action.Params);
	
	FSupertalkEventCompletedDelegate Completed;
	Completed.BindUObject(this, &ThisClass::CompleteActionAndTick, Context.Key);
	
	OnPlayLine(Params->Line, Completed);
}

void USupertalkPlayer::HandlePlayChoice(const FSupertalkActionWithContext& Context)
{
	USupertalkPlayChoiceParams* Params = CastChecked<USupertalkPlayChoiceParams>(Context.Action.Params);
	check(Params->Choices.Num() > 0);
	
	FSupertalkChoiceCompletedDelegate Completed;
	Completed.BindUObject(this, &ThisClass::ReceiveChoice, Context.Key);

	TArray<FText> Choices;
	Choices.Reserve(Params->Choices.Num());
	
	for (const FSupertalkChoice& Choice : Params->Choices)
	{
		Choices.Add(Choice.Text);
	}
	
	OnPlayChoice(Params->Line, Choices, Completed);
}

void USupertalkPlayer::ReceiveChoice(int32 ChoiceIndex, FSupertalkActionKey Key)
{
	FSupertalkStack* Stack = Stacks.Find(Key.StackId);
	if (Stack == nullptr)
	{
		UE_LOG(LogSupertalk, Warning, TEXT("ReceiveChoice called with unknown stack id %u"), Key.StackId);
		return;
	}

	if (!Key.IsValid() || Stack->ActiveAction.Key != Key)
	{
		UE_LOG(LogSupertalk, Warning, TEXT("ReceiveChoice called with unknown action id %u (stack %u expects %u)"), Key.ActionId, Key.StackId, Stack->ActiveAction.Key.ActionId);
		return;
	}

	USupertalkPlayChoiceParams* Params = CastChecked<USupertalkPlayChoiceParams>(Stack->ActiveAction.Action.Params);
	if (ChoiceIndex < 0)
	{
		CompleteActionAndTick(Key);
		return;
	}
	
	if (ChoiceIndex >= Params->Choices.Num())
	{
		UE_LOG(LogSupertalk, Error, TEXT("ReceiveChoice called with invalid choice index %d (expected < %d)"), ChoiceIndex, Params->Choices.Num());
	}

	const FSupertalkChoice& Choice = Params->Choices[ChoiceIndex];
	PushAction(*Stack, Stack->ActiveAction.Source, Choice.SubAction);
	
	CompleteActionAndTick(Key);
}

void USupertalkPlayer::HandleAssign(const FSupertalkActionWithContext& Context)
{
	USupertalkAssignParams* Params = CastChecked<USupertalkAssignParams>(Context.Action.Params);

	const USupertalkValue* Value = nullptr;
	if (IsValid(Params->Expression))
	{
		Value = Params->Expression->Evaluate(this);
	}
	else
	{
		Value = Params->Value_DEPRECATED;
	}

	Value = Value ? Value->GetResolvedValue(this) : nullptr;

	SetVariable(Params->Variable, Value);

	// Not necessary to tick, this can only happen as the result of an ongoing tick.
	CompleteAction(Context.Key);
}

void USupertalkPlayer::HandleCall(const FSupertalkActionWithContext& Context)
{
	FMessageLog MessageLog(SupertalkMessageLogName);
	
	USupertalkCallParams* Params = CastChecked<USupertalkCallParams>(Context.Action.Params);
	
	if (!ensure(!CurrentFunctionFinalizer))
	{
		MessageLog.Error(FText::Format(LOCTEXT("EventCallFinalizerExists", "Finalizer already set, was there a recursive event call? Skipping function call: {0}"), FText::FromString(Params->Arguments)));
		CompleteAction(Context.Key);
		return;
	}

	FSupertalkLatentFunctionFinalizer Finalizer;
	Finalizer.Completed.BindUObject(this, &ThisClass::CompleteActionAndTick, Context.Key);
	CurrentFunctionFinalizer = &Finalizer;
	bIsFunctionCallLatent = false;

	// TODO: shouldn't be using FText for this. It's slow, it's converting back and forth between FText/FString.
	// Function calls need to be rewritten to support actual objects at some point and not strings, so this will go away whenever that happens.
	const FString FormattedArgs = FSupertalkUtilities::FormatText(FText::FromString(Params->Arguments), this, false).ToString();
	
	bool bCalledFunction = false;
	for (UObject* Receiver : FunctionCallReceivers)
	{
		if (Receiver->CallFunctionByNameWithArguments(*FormattedArgs, *GLog, nullptr, true))
		{
			bCalledFunction = true;
			break;
		}
	}
	
	CurrentFunctionFinalizer = nullptr;

	if (!bCalledFunction)
	{
		MessageLog.Error(FText::Format(LOCTEXT("FunctionCallFail", "Failed to call function from script: {0}"), FText::FromString(Params->Arguments)));
		CompleteAction(Context.Key);
		return;
	}

	if (!bIsFunctionCallLatent)
	{
		// MakeLatentFunction was not called, complete this event immediately
		CompleteAction(Context.Key);
	}
}

void USupertalkPlayer::HandleJump(const FSupertalkActionWithContext& Context)
{
	check(Context.Source);
	
	FMessageLog MessageLog(SupertalkMessageLogName);
	
	USupertalkJumpParams* Params = CastChecked<USupertalkJumpParams>(Context.Action.Params);

	FSupertalkStack* Stack = Stacks.Find(Context.Key.StackId);
	if (Stack == nullptr)
	{
		UE_LOG(LogSupertalk, Error, TEXT("Cannot jump using unknown stack %u"), Context.Key.StackId);
		CompleteAction(Context.Key);
		return;
	}

	if (Params->JumpTarget == NAME_None)
	{
		// Forcefully end this stack
		Stack->QueuedActions.Empty();
		CompleteAction(Context.Key);
		return;
	}
	
	const FSupertalkSection* NewSection = Context.Source->Sections.Find(Params->JumpTarget);
	if (NewSection == nullptr)
	{
		MessageLog.Error(FText::Format(LOCTEXT("JumpSectionError", "Cannot jump to unknown section '{0}'"), FText::FromName(Params->JumpTarget)));
		CompleteAction(Context.Key);
		return;
	}

	Stack->QueuedActions.Empty();
	PushActions(Stack->StackId, Context.Source, NewSection->Actions);
	CompleteAction(Context.Key);
}

void USupertalkPlayer::HandleParallel(const FSupertalkActionWithContext& Context)
{
	USupertalkParallelParams* Params = CastChecked<USupertalkParallelParams>(Context.Action.Params);
	if (Params->SubActions.Num() == 0)
	{
		UE_LOG(LogSupertalk, Warning, TEXT("Parallel action with no subactions, skipped"));
		CompleteAction(Context.Key);
		return;
	}

	FSupertalkStack* SourceStack = Stacks.Find(Context.Key.StackId);
	if (SourceStack == nullptr)
	{
		UE_LOG(LogSupertalk, Error, TEXT("Unknown stack %u when setting up parallel execution"), Context.Key.StackId);
		CompleteAction(Context.Key);
		return;
	}

	for (const FSupertalkAction& Action : Params->SubActions)
	{
		FSupertalkStack& Stack = CreateNewStack();
		Stack.SourceId = SourceStack->StackId;
		SourceStack->WaitingOn.Add(Stack.StackId);

		PushAction(Stack, Context.Source, Action);
		TickStack(Stack.StackId);
	}

	CompleteAction(Context.Key);
}

void USupertalkPlayer::FinishWaitingOnStack(uint32 ExitingId, uint32 WaitingId)
{
	FSupertalkStack* Stack = Stacks.Find(WaitingId);
	if (Stack == nullptr)
	{
		UE_LOG(LogSupertalk, Error, TEXT("FinishWaitingOnStack was given an invalid WaitingId of %u"), WaitingId);
		return;
	}

	if (!Stack->WaitingOn.Remove(ExitingId))
	{
		UE_LOG(LogSupertalk, Error, TEXT("FinishWaitingOnStack was given an invalid ExitingId of %u for stack %u"), ExitingId, WaitingId);
		return;
	}

	TickStack(WaitingId);
}

void USupertalkPlayer::HandleQueue(const FSupertalkActionWithContext& Context)
{
	USupertalkQueueParams* Params = CastChecked<USupertalkQueueParams>(Context.Action.Params);
	if (Params->SubActions.Num() == 0)
	{
		CompleteAction(Context.Key);
		return;
	}

	PushActions(Context.Key.StackId, Context.Source, Params->SubActions);
	CompleteAction(Context.Key);
}

void USupertalkPlayer::HandleConditional(const FSupertalkActionWithContext& Context)
{
	USupertalkConditionalParams* Params = CastChecked<USupertalkConditionalParams>(Context.Action.Params);

	const USupertalkValue* Value = nullptr;
	if (IsValid(Params->Expression))
	{
		Value = Params->Expression->Evaluate(this);
	}
	else
	{
		Value = Params->Value_DEPRECATED;
	}

	Value = Value ? Value->GetResolvedValue(this) : nullptr;

	bool bConditionalValue;
	if (Value)
	{
		const USupertalkBooleanValue* BoolValue = Cast<USupertalkBooleanValue>(Value);
		if (!BoolValue)
		{
			UE_LOG(LogSupertalk, Warning, TEXT("Conditional action received non-boolean value '%s', skipping (non-boolean values are not supported at this time)"), *Value->ToDisplayText().ToString());
			CompleteAction(Context.Key);
			return;
		}

		bConditionalValue = BoolValue->bValue;
	}
	else
	{
		bConditionalValue = false;
	}

	PushAction(Context.Key.StackId, Context.Source, bConditionalValue ? Params->TrueAction : Params->FalseAction);
	CompleteAction(Context.Key);
}

#undef LOCTEXT_NAMESPACE
