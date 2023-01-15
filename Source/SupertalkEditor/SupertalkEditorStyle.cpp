// Copyright (c) MissiveArts LLC

#include "SupertalkEditorStyle.h"

#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FSupertalkEditorStyle::StyleSet = nullptr;

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

void FSupertalkEditorStyle::Initialize()
{
	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShared<FSlateStyleSet>("SupertalkEditor");
	StyleSet->SetContentRoot( FPaths::EngineContentDir() / TEXT("Editor/Slate") );
	StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	const FSlateFontInfo EditorFont = DEFAULT_FONT("Mono", 11);

	const FTextBlockStyle NormalText = FTextBlockStyle()
		.SetFont(EditorFont)
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black)
		.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f))
		.SetHighlightShape(BOX_BRUSH("Common/TextBlockHighlightShape", FMargin(3.f/8.f)));

	{
		StyleSet->Set("TextEditor.NormalText", NormalText);

		StyleSet->Set("SyntaxHighlight.STS.Normal", NormalText);
		StyleSet->Set("SyntaxHighlight.STS.Comment", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xff009615))));
		StyleSet->Set("SyntaxHighlight.STS.Keyword", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xff67b1f5))));
		StyleSet->Set("SyntaxHighlight.STS.Operator", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xffc07cf7))));
		StyleSet->Set("SyntaxHighlight.STS.Value", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xffaad1fa))));
		StyleSet->Set("SyntaxHighlight.STS.String", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xfffc9d53))));
		StyleSet->Set("SyntaxHighlight.STS.Section", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xffda8bfc))));
		StyleSet->Set("SyntaxHighlight.STS.Command", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xfff7d67c))));
		StyleSet->Set("SyntaxHighlight.STS.Jump", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xff7dfa9e))));
		StyleSet->Set("SyntaxHighlight.STS.Error", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xffff7083))));

		const FEditableTextBoxStyle EditableTextBoxStyle = FEditableTextBoxStyle()
			.SetTextStyle(NormalText)
			.SetBackgroundColor(FLinearColor::Black)
			.SetBackgroundImageNormal( FSlateNoResource() )
			.SetBackgroundImageHovered( FSlateNoResource() )
			.SetBackgroundImageFocused( FSlateNoResource() )
			.SetBackgroundImageReadOnly( FSlateNoResource() );

		StyleSet->Set("TextEditor.EditableTextBox", EditableTextBoxStyle);

		StyleSet->Set("TextEditor.LineNumberText", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(FColor(0xffbbbbbb)))));
	}

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef DEFAULT_FONT

void FSupertalkEditorStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

const ISlateStyle& FSupertalkEditorStyle::Get()
{
	return *StyleSet.Get();
}

const FName& FSupertalkEditorStyle::GetStyleSetName()
{
	return StyleSet->GetStyleSetName();
}

