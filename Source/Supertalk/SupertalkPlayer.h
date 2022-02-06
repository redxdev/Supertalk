// Copyright (c) MissiveArts LLC

#pragma once

#include "CoreMinimal.h"
#include "SupertalkLine.h"
#include "SupertalkPlayer.generated.h"

class USupertalkValue;
class USupertalkExpression;
class USupertalkPlayer;

UENUM()
enum class ESupertalkOperation : uint8
{
	Noop,
    Line,
    Choice,
    Assign,
    Call,
    Jump,
	Parallel,
	Queue,
	Conditional
};

UCLASS(MinimalAPI)
class USupertalkOperationParams : public UObject
{
	GENERATED_BODY()
};

USTRUCT()
struct SUPERTALK_API FSupertalkAction
{
	GENERATED_BODY()

	FSupertalkAction()
	{
		Operation = ESupertalkOperation::Noop;
		Params = nullptr;
	}

	UPROPERTY(VisibleAnywhere)
	ESupertalkOperation Operation;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USupertalkOperationParams> Params;
};

USTRUCT()
struct SUPERTALK_API FSupertalkSection
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FName Name;

	UPROPERTY(VisibleAnywhere)
	TArray<FSupertalkAction> Actions;
};

UCLASS(BlueprintType, HideCategories=(Object))
class SUPERTALK_API USupertalkScript : public UObject
{
	GENERATED_BODY()

public:
	
	UPROPERTY(VisibleAnywhere, Category = Script)
	FName DefaultSection;
	
	UPROPERTY(VisibleAnywhere, Category = Script)
	TMap<FName, FSupertalkSection> Sections;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Instanced, Category=ImportSettings)
	TObjectPtr<class UAssetImportData> AssetImportData;

	virtual void PostInitProperties() override;
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	virtual void Serialize(FArchive& Ar) override;
#endif

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = Commands)
	void OpenSourceFileInExternalProgram();
#endif
};

UCLASS()
class SUPERTALK_API USupertalkPlayLineParams : public USupertalkOperationParams
{
	GENERATED_BODY()

public:
	
	UPROPERTY()
	FSupertalkLine Line;
};

USTRUCT()
struct SUPERTALK_API FSupertalkChoice
{
	GENERATED_BODY()

	UPROPERTY()
	FText Text;

	UPROPERTY()
	FSupertalkAction SubAction;
};

UCLASS()
class SUPERTALK_API USupertalkPlayChoiceParams : public USupertalkPlayLineParams
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TArray<FSupertalkChoice> Choices;
};

UCLASS()
class SUPERTALK_API USupertalkAssignParams : public USupertalkOperationParams
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName Variable;

	UPROPERTY()
	TObjectPtr<USupertalkValue> Value_DEPRECATED;

	UPROPERTY()
	TObjectPtr<USupertalkExpression> Expression;

	virtual void PostLoad() override;
};

UCLASS()
class SUPERTALK_API USupertalkCallParams : public USupertalkOperationParams
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString Arguments;
};

UCLASS()
class SUPERTALK_API USupertalkJumpParams : public USupertalkOperationParams
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName JumpTarget;
};

UCLASS()
class SUPERTALK_API USupertalkParallelParams : public USupertalkOperationParams
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FSupertalkAction> SubActions;
};

UCLASS()
class SUPERTALK_API USupertalkQueueParams : public USupertalkOperationParams
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FSupertalkAction> SubActions;
};

UCLASS()
class SUPERTALK_API USupertalkConditionalParams : public USupertalkOperationParams
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<USupertalkValue> Value_DEPRECATED;

	UPROPERTY()
	TObjectPtr<USupertalkExpression> Expression;

	UPROPERTY()
	FSupertalkAction TrueAction;

	UPROPERTY()
	FSupertalkAction FalseAction;

	virtual void PostLoad() override;
};

struct FSupertalkActionKey
{
	friend class USupertalkPlayer;
	
	FSupertalkActionKey()
	{
		StackId = 0;
		ActionId = 0;
	}

	FORCEINLINE bool IsValid() const
	{
		return StackId > 0 && ActionId > 0;
	}

	friend bool operator==(const FSupertalkActionKey& Lhs, const FSupertalkActionKey& Rhs)
	{
		return Lhs.StackId == Rhs.StackId && Lhs.ActionId == Rhs.ActionId;
	}

	friend bool operator!=(const FSupertalkActionKey& Lhs, const FSupertalkActionKey& Rhs)
	{
		return !(Lhs == Rhs);
	}

private:
	uint32 StackId;
	uint32 ActionId;
};

USTRUCT()
struct FSupertalkActionWithContext
{
	GENERATED_BODY()

	FSupertalkActionWithContext()
	{
		Key = FSupertalkActionKey();
		Source = nullptr;
		Action = FSupertalkAction();
	}

	UPROPERTY()
	TObjectPtr<const class USupertalkScript> Source;

	UPROPERTY()
	FSupertalkAction Action;

	FSupertalkActionKey Key;
};

USTRUCT()
struct FSupertalkStack
{
	GENERATED_BODY()

	FSupertalkStack()
	{
		StackId = 0;
		SourceId = 0;
		bIsTicking = false;
	}

	uint32 StackId;

	// Id of the stack that created this one.
	uint32 SourceId;

	// Ids that this stack is waiting on.
	TSet<uint32> WaitingOn;

	uint32 bIsTicking : 1;

	UPROPERTY()
	FSupertalkActionWithContext ActiveAction;

	UPROPERTY()
	TArray<FSupertalkActionWithContext> QueuedActions;
};

DECLARE_DELEGATE(FSupertalkEventCompletedDelegate);

DECLARE_DELEGATE_OneParam(FSupertalkChoiceCompletedDelegate, int32);
DECLARE_DELEGATE_TwoParams(FSupertalkPlayLineDelegate, const FSupertalkLine&, FSupertalkEventCompletedDelegate Completed);
DECLARE_DELEGATE_ThreeParams(FSupertalkPlayChoiceDelegate, const FSupertalkLine&, const TArray<FText>& Choices, FSupertalkChoiceCompletedDelegate Completed);

// Used to let a script know that a latent function has completed.
USTRUCT(BlueprintType)
struct SUPERTALK_API FSupertalkLatentFunctionFinalizer
{
	GENERATED_BODY()

	friend class USupertalkPlayer;

	FORCEINLINE void Complete() { Completed.ExecuteIfBound(); }

	private:
	FSupertalkEventCompletedDelegate Completed;
};

UCLASS()
class SUPERTALK_API USupertalkPlayer : public UObject
{
	GENERATED_BODY()

public:
	USupertalkPlayer();
	
	void SetVariable(FName Name, const USupertalkValue* Value);
	void SetVariable(FName Name, bool Value);
	void SetVariable(FName Name, FText Value);
	
	const USupertalkValue* GetVariable(FName Name) const;
	void ClearVariables();

	void AddFunctionCallReceiver(UObject* Obj);

	void RunScript(const class USupertalkScript* Script, FName InitialSection = NAME_None);
	void Stop();

	FORCEINLINE bool IsRunningScript() const { return Stacks.Num() > 0; }

	FSupertalkPlayLineDelegate OnPlayLineEvent;
	FSupertalkPlayChoiceDelegate OnPlayChoiceEvent;

	// When called from a function that was executed by a running script, this will let the function
	// become latent. Use the returned finalizer
	UFUNCTION(BlueprintCallable)
	FSupertalkLatentFunctionFinalizer MakeLatentFunction();

	UFUNCTION(BlueprintCallable)
	static void CompleteFunction(FSupertalkLatentFunctionFinalizer Finalizer);
	
protected:

	// Called if GetVariable fails, meant to be overridden.
	virtual const USupertalkValue* GetExternalVariable(FName Name) const;

	virtual void OnPlayLine(const FSupertalkLine& Line, FSupertalkEventCompletedDelegate Completed);
	virtual void OnPlayChoice(const FSupertalkLine& Line, const TArray<FText>& Choices, FSupertalkChoiceCompletedDelegate Completed);

private:
	uint32 NextActionId;
	uint32 NextStackId;

	UPROPERTY()
	TMap<uint32, FSupertalkStack> Stacks;

	UPROPERTY()
	TMap<FName, TObjectPtr<class USupertalkValue>> Variables;

	UPROPERTY()
	TArray<TObjectPtr<UObject>> FunctionCallReceivers;

	bool bIsFunctionCallLatent;
	FSupertalkLatentFunctionFinalizer* CurrentFunctionFinalizer = nullptr;

	FSupertalkStack& CreateNewStack();

	uint32 GetNewActionId();
	uint32 GetNewStackId();

	// Pushes actions onto the top of the stack. They will execute next, in the order given in the array.
	void PushActions(uint32 StackId, const USupertalkScript* Script, const TArray<FSupertalkAction>& Actions);

	// Pushes a single action onto the top of the stack. It will execute next.
	void PushAction(FSupertalkStack& Stack, const USupertalkScript* Script, const FSupertalkAction& Action);
	void PushAction(uint32 StackId, const USupertalkScript* Script, const FSupertalkAction& Action);

	void CompleteActionAndTick(FSupertalkActionKey Key);
	void CompleteAction(FSupertalkActionKey Key);
	void TickStack(uint32 StackId);

	void ExecuteAction(const FSupertalkActionWithContext& Context);

	void HandlePlayLine(const FSupertalkActionWithContext& Context);
	
	void HandlePlayChoice(const FSupertalkActionWithContext& Context);
	void ReceiveChoice(int32 ChoiceIndex, FSupertalkActionKey Key);
	
	void HandleAssign(const FSupertalkActionWithContext& Context);
	
	void HandleCall(const FSupertalkActionWithContext& Context);
	
	void HandleJump(const FSupertalkActionWithContext& Context);
	
	void HandleParallel(const FSupertalkActionWithContext& Context);
	void FinishWaitingOnStack(uint32 ExitingId, uint32 WaitingId);

	void HandleQueue(const FSupertalkActionWithContext& Context);

	void HandleConditional(const FSupertalkActionWithContext& Context);
};
