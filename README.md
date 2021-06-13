# Supertalk

Welcome to Supertalk! This is a simple dialogue scripting language for Unreal Engine created due to frustration with
visual dialogue tree workflows.

The plugin is made up of two parts: an editor module and a runtime module. The editor module handles importing supertalk
scripts (.sts files) into a form that the engine can use. The runtime module includes a supertalk "player" which handles
playback for imported scripts.

Supertalk is currently configured for Unreal Engine 5.0, but can easily be made to work with earlier versions by replacing
its usages of `TObjectPtr`.

**Supertalk is not a ready-to-use solution for dialogue systems!** It requires a good amount of integration work and comes
with no UI. While it does integrate well with blueprint, initial integration with a game requires knowledge of C++ and no
support will be offered beyond this documentation.

## Known Issues

* Currently there is no syntax to assign namespaces/keys for dialogue text for i18n. This is planned for some point in the future.
* Recursive calls into `USupertalkPlayer::PlayScript` are not currently allowed, though they should technically be possible. This
  somewhat limits how dynamic scripts can be (you can't change what section is playing based on external data) but this will be
  fixed in the future.
* The way the supertalk parser/importer emits errors is a bit odd, haven't quite figured out how to deal with the message log the
  right way. It generally works fine but might not be implemented the "right" way.

## Scripting Syntax + Features

Scripts have the extension `.sts` and can be imported once this plugin is enabled simply by dragging one into the content browser.
Alternatively, you may turn on auto-import which is the more ideal workflow as it will automatically re-import changes made to the
script files.

```
-- Comments are preceded by two dashes

-- Very simple variables can be used. You can reference assets within a script which can be used
-- to define who is speaking a line of dialogue, or if they implement the ISupertalkDisplayInterface
-- they can be embedded within lines of dialogue themselves.
-- In my own game, I have a "speaker" asset that would be used here. Speakers define a character's name,
-- "voice" if there's audio associated with them, and portrait.
Person1 = /Game/MyGame/Characters/Person1
Person2 = /Game/MyGame/Characters/Person2

-- This will play a few lines of dialogue.
Person1: Hello, world!

-- You can use FString::Format syntax to reference variables.
Person2: Hello, {Person2}!

-- You can pass just a string as a speaker's name
"Some third person": I'm not predefined in a variable!

-- If you don't have any special characters, you don't need to quote the name (though you may still want to
-- in order to differentiate it from an actual variable).
Person3: I will appear as "Person3".

-- You can pass an alternate name for a predefined speaker
Person1,"An alternate name": I'm still {Person1} but with a different name.

-- You can pass along attributes with a line - each attribute, separated by a comma, is passed as an FName
-- as part of the line of dialogue. I use this to define a position for character portraits to appear, along with
-- which portrait should be displayed.
Person2 [Left, Sad]: I'm sad now :(

-- You can add linebreaks freely without breaking up the dialogue lines after the first one indented. Line breaks
-- will only be added to the final output if you have an empty line (a la markdown).
Person1: This sentence will span
         multiple lines in the script
         but will appear as a single
         one when played.

         This will appear as a second
         line to go along with the first.

-- Supertalk supports calling events on blueprints and UFUNCTIONs on C++ UObjects. See the integration guide for an
-- example of how to do this. Supertalk even supports latent actions here (events that take time) and will wait on them
-- if they have been configured appropriately.
> Wait 10
> DoSomethingCool

-- In some cases you may want to do multiple things at a time. For example, display a line of dialogue at the same time
-- that you have a character walk to a new position. You can do this with a "parallel" block. Note that this is not truly
-- parallel in the sense of threading - this simply runs all statements inside it in order *and then* waits for all of them
-- to complete if any are latent.
{
    > WalkToNewPosition
    Person2: I need to say something while someone walks to a new position.
}

-- Supertalk supports passing along a list of choices with a dialogue line. You can then define what happens with each choice.
-- By default you may only pass along a single statement to execute with a choice - if you want to pass multiple you can use
-- parenthesis (*not* curly braces - that would be a parallel block instead) to group multiple statements together.
Person1: Do you like cake or pie?
* Cake
  Person1: You chose cake!
* Pie
  (
      Person1: You chose pie!
      > DoSomethingCool
  )

-- Depending on the game you might find it useful for emit "blank" lines to your integration. This syntax will cause
-- FSupertalkLine::bIsBlankLine to be set to true. You can specify attributes here as well. The primary use-case is, for example,
-- to setup multiple character portraits without requiring them to actually speak.
Person1 [Sad];
Person2 [Happy];

-- Supertalk supports having multiple "sections" in a single script. Only the first section will execute by default, but you can
-- either include a jump (using an arrow ->) or from C++ tell the supertalk player to run an alternate section. Jumps are normal
-- statements like commands or dialogue lines and as such can be used pretty much anywhere.
-- Note that there is *not* any fall-through from one section to another. If you reach the end of a section and there is no jump,
-- the next section will *not* play automatically.
-- The first section of a script will *always* play by default and if you don't explicitly give it a name it'll be named "Default".

-> Section2

# Section2

Person2: Which section do you want to go to?
* 3
  -> Section3
* 4
  -> Section4

# Section3

Person2: I'm in section 3!
-> Section5

# Section4

Person2: I'm in section 4!
-> Section5

# Section5

Person1: That's the end of the supertalk script overview. Goodbye for now!

-- You can end a script's execution either by having no more statements in a section or by jumping to 'None'
-> None
```

## VSCode Syntax Highlighting

A basic syntax highlighting extension for Visual Studio Code can be found in the `Extras` directory.

## Integration Guide

This is an overview of the work necessary to integrate Supertalk into a project.

### Major Types

All of these types are in the `Supertalk` module - generally you will not have to use anything in `SupertalkEditor` as it exists purely
to provide the editor with an importer for supertalk script files.

#### `USupertalkPlayer`

This is your primary interaction point with Supertalk and implements almost all of the major functionality. It is a very simple "VM" that
executes supertalk scripts.

Each Supertalk player contains its own state of execution and list of variables. You can get and set variables on it manually as well.

If you want to expose variables from your game without manually setting variables, you can subclass `USupertalkPlayer` and implement
`USupertalkPlayer::GetExternalVariable` which will be called if a variable cannot be found.

#### `USupertalkScript`

This is the main asset type for Supertalk.

#### `FSupertalkLine`

Dialogue lines are emitted via this struct. It contains a value representing who is speaking the line, a value for a name override for the
speaker, a list of attributes applied to the line, 

#### `USupertalkValue`

Base class for a "value" which can be anything - an object, text, datatable, etc.

#### `ISupertalkDisplayInterface`

Implement on custom UObjects to provide a way to override how Supertalk will display those objects in dialogue lines. You can also override
`ISupertalkDisplayInterface::GetSupertalkMember` in order to provide a way to get sub-values of an object.

#### `FSupertalkTableRow`

Any data table used within Supertalk should use this row type, which allows storing text that can be reused in multiple scripts.

#### `FSupertalkLatentFunctionFinalizer`

Supertalk uses this object as a way to tell when latent functions are being called. Calling `FSupertalkLatentFunctionFinalizer::Complete()` will tell
the Supertalk player that the action has completed and it may continue. These are also usable in blueprint (to allow for calling latent actions in
events) - a blueprint event being called from Supertalk should call `USupertalkPlayer::MakeLatentFunction` to receive a finalizer and then call
`USupertalkPlayer::CompleteFunction` when it has finished.

If an event is not latent (i.e. it doesn't take up any time) then it should not call `MakeLatentFunction` at all.

### Integrating the Supertalk Player

The Supertalk player is your primary entrypoint into Supertalk - it represents the "state" of a script. You can have any number of them at a time
and they can share scripts, but each one maintains its own state completely separate from the others.

You can create a Supertalk player with `NewObject<USupertalkPlayer>()`. Once created, you can bind events to handle when dialogue lines (or set of choices)
are played, and you can register handlers to receive 

```cpp
// In some function somewhere - maybe the initialization for a cutscene class.
USupertalkPlayer* STPlayer = NewObject<USupertalkPlayer>(this);
STPlayer->OnPlayLineEvent.BindUObject(this, &ThisClass::OnPlayLine);
STPlayer->OnPlayChoiceEvent.BindUObject(this, &ThisClass::OnPlayChoice);

// If you want to be able to call functions on the current object (or some other object) from within a script, you need to add a function call receiver.
// Any public UFUNCTION (or blueprint event or function) will be callable, and the syntax for calling a command is the same as the "ke" console command.
STPlayer->AddFunctionCallReceiver(this);
```

`OnPlayLineEvent` and `OnPlayChoiceEvent` are important to implement or else you will not find out when a dialogue line/choice has been played. When you are
done playing the dialogue line/choice (which generally entails showing it on-screen and then waiting for user input to continue) you should call the `Completed`
delegate.

For `OnPlayChoiceEvent` you must also pass an integer representing the index of the selected choice. If an invalid choice (or no choice, if that's allowed) is
selected then you should pass `INDEX_NONE` as the index. Note that this will result in no choice statements being executed - the Supertalk player will simply
continue to the next statement after the current set of choices.

## VM Internals

The Supertalk parser is *only* used for asset importing and as such is editor-only. It is a very simple handwritten lexer and parser combo - The primary entrypoint
is `USupertalkParser::Parse` which calls into both the lexer (`USupertalkParser::RunLexer`) and the parser (`USupertalkParser::RunParser`). Lexer functions are prefixed
by `Lx` while parser functions are prefixed by `Pa`. The result of the parser is a `USupertalkScript` object which can be saved to disk as an asset.

The Supertalk VM/Player is incredibly simple - a script asset is effectively just the AST of the original supertalk script with a little bit of extra processing
done on it. The list of possible action types can be found in `ESupertalkOperation`, while actions themselves are implemented in `USupertalkPlayer`. Each action can
also have additional parameters, implemented by subclassing `USupertalkOperationParams`. Actions themselves are represented by `FSupertalkAction`.

The VM contains a list of stacks (`FSupertalkStack`). Each stack contains a list of actions to be executed and a single active action. Each stack is assigned an id,
as is each action. Ids are used throughout the VM to retrieve stacks and to validate that an operation is happening on the correct stack/action. When a script is
played the VM will start with a single stack and it will queue all the actions for the initial section of that script.

When a latent action is executed (either due to a dialogue line/choice executing or due to `MakeLatentFunction` being called) the current stack is paused. When the action is
completed (via a `Completed` delegate call or via `FSupertalkLatentFunctionFinalizer::Complete()`) the stack is notified and continues.

When a parallel block is encountered a set of new stacks is created - one for each action (or group of actions) inside the block. These stacks are assigned unique ids and the
original stack stores these ids in a waiting list (`FSupertalkStack::WaitingOn`) and pauses itself. When a stack completes (usually due to running out of actions to execute) it
removes itself from the original stack's waiting list. Once that waiting list is empty the original stack will resume execution.

Execution of queue blocks (lists of actions to be executed sequentially inside parenthesis) is not given any special handling - actions are simply added to the current stack
in the appropriate order.

Section jumps cause the current stack to be emptied and then refilled from the given section. The one case where this doesn't happen is upon a section jump to `None`, in which
case the stack is simply emptied - thus ending that stack's execution.