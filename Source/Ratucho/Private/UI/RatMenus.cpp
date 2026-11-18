#include "UI/RatMenus.h"

#include "Game/RatuchoMenuGameMode.h"
#include "Game/RatuchoTypes.h"
#include "Player/RatPlayerController.h"
#include "Styling/CoreStyle.h"
#include "UI/RatSfx.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "RatuchoMenu"

namespace RatUI
{
	const FLinearColor Cream(0.98f, 0.93f, 0.82f);
	const FLinearColor Cheese(1.f, 0.78f, 0.12f);
	const FLinearColor Brown(0.32f, 0.2f, 0.12f);
	const FLinearColor Selected(1.f, 0.72f, 0.1f);
	const FLinearColor Button(0.22f, 0.15f, 0.1f);

	FSlateFontInfo Font(int32 Size, bool bBold = true)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}

	const FSlateBrush* White()
	{
		return FCoreStyle::Get().GetBrush("GenericWhiteBox");
	}

	/** Botão padrão do Ratucho. Todo botão visível executa uma ação. */
	TSharedRef<SWidget> MakeButton(const FText& Label, FOnClicked OnClicked, TAttribute<FSlateColor> Color = FSlateColor(Button), float Width = 420.f, int32 FontSize = 22)
	{
		return SNew(SBox)
			.WidthOverride(Width)
			.Padding(FMargin(0.f, 6.f))
			[
				SNew(SButton)
				.ButtonColorAndOpacity_Lambda([Color]() { return Color.Get().GetSpecifiedColor(); })
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.ContentPadding(FMargin(18.f, 12.f))
				.OnClicked_Lambda([OnClicked]()
				{
					RatSfx::PlayCue(nullptr, ERatSfx::UiClick);
					return OnClicked.Execute();
				})
				[
					SNew(STextBlock)
					.Text(Label)
					.Font(Font(FontSize))
					.ColorAndOpacity(FSlateColor(FLinearColor::White))
				]
			];
	}

	TSharedRef<SWidget> MakeLabel(const TAttribute<FText>& Txt, int32 Size, const FLinearColor& Color, bool bBold = true)
	{
		return SNew(STextBlock).Text(Txt).Font(Font(Size, bBold)).ColorAndOpacity(FSlateColor(Color)).Justification(ETextJustify::Center);
	}
}

// ============================================================================
// SRatInGameMenu
// ============================================================================

void SRatInGameMenu::Construct(const FArguments& InArgs)
{
	Owner = InArgs._Owner;
	const bool bEnd = InArgs._bEndScreen;
	TWeakObjectPtr<ARatPlayerController> WeakOwner = Owner;

	TSharedRef<SVerticalBox> Buttons = SNew(SVerticalBox);

	if (bEnd)
	{
		const bool bHost = WeakOwner.IsValid() && WeakOwner->CanRestart();
		if (bHost)
		{
			Buttons->AddSlot().AutoHeight().HAlign(HAlign_Center)
			[
				RatUI::MakeButton(LOCTEXT("PlayAgain", "Jogar novamente"), FOnClicked::CreateLambda([WeakOwner]()
				{
					if (WeakOwner.IsValid()) { WeakOwner->RequestRestart(); }
					return FReply::Handled();
				}), FSlateColor(FLinearColor(0.85f, 0.55f, 0.05f)))
			];
		}
		else
		{
			Buttons->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 8.f)
			[
				RatUI::MakeLabel(LOCTEXT("WaitHost", "Aguardando o host iniciar uma nova rodada..."), 18, RatUI::Cream, false)
			];
		}
	}
	else
	{
		Buttons->AddSlot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeLabel(LOCTEXT("Paused", "MENU"), 40, RatUI::Cheese)
		];
		Buttons->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 12.f)
		[
			RatUI::MakeLabel(LOCTEXT("NoPause", "A partida continua rodando (multiplayer)."), 16, RatUI::Cream, false)
		];
		Buttons->AddSlot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeButton(LOCTEXT("Resume", "Continuar"), FOnClicked::CreateLambda([WeakOwner]()
			{
				if (WeakOwner.IsValid()) { WeakOwner->ClosePauseMenu(); }
				return FReply::Handled();
			}), FSlateColor(FLinearColor(0.85f, 0.55f, 0.05f)))
		];
	}

	Buttons->AddSlot().AutoHeight().HAlign(HAlign_Center)
	[
		RatUI::MakeButton(LOCTEXT("BackToMenu", "Voltar ao menu"), FOnClicked::CreateLambda([WeakOwner]()
		{
			if (WeakOwner.IsValid()) { WeakOwner->ReturnToMainMenu(); }
			return FReply::Handled();
		}))
	];
	if (!bEnd)
	{
		Buttons->AddSlot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeButton(LOCTEXT("Quit", "Sair do jogo"), FOnClicked::CreateLambda([WeakOwner]()
			{
				if (WeakOwner.IsValid()) { WeakOwner->QuitGame(); }
				return FReply::Handled();
			}), FSlateColor(FLinearColor(0.45f, 0.12f, 0.1f)))
		];
	}

	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(bEnd ? VAlign_Bottom : VAlign_Center)
	.Padding(FMargin(0.f, 0.f, 0.f, bEnd ? 60.f : 0.f))
	[
		SNew(SBorder)
		.BorderImage(RatUI::White())
		.BorderBackgroundColor(FLinearColor(0.08f, 0.05f, 0.03f, bEnd ? 0.0f : 0.88f))
		.Padding(FMargin(40.f, 28.f))
		[
			Buttons
		]
	];
}

// ============================================================================
// SRatMainMenu
// ============================================================================

void SRatMainMenu::Construct(const FArguments& InArgs)
{
	Owner = InArgs._Owner;
	MaxBots = FMath::Max(0, InArgs._MaxBots);
	NumBots = FMath::Clamp(InArgs._DefaultBots, 0, MaxBots);

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(RatUI::White()).ColorAndOpacity(FSlateColor(FLinearColor(0.93f, 0.8f, 0.6f)))
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			// "piso" xadrez estilizado
			SNew(SBox).HeightOverride(220.f)
			[
				SNew(SImage).Image(RatUI::White()).ColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.42f, 0.26f)))
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("Title", "RATUCHO")).Font(RatUI::Font(110)).ColorAndOpacity(FSlateColor(RatUI::Cheese))
				.ShadowOffset(FVector2D(5.f, 5.f)).ShadowColorAndOpacity(FLinearColor(0.35f, 0.18f, 0.05f, 1.f))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 30.f)
			[
				RatUI::MakeLabel(LOCTEXT("Tagline", "Cheese. Chaos. Survive."), 24, RatUI::Brown)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(SBorder)
				.BorderImage(RatUI::White())
				.BorderBackgroundColor(FLinearColor(0.1f, 0.06f, 0.03f, 0.85f))
				.Padding(FMargin(36.f, 24.f))
				[
					SNew(SWidgetSwitcher)
					.WidgetIndex_Lambda([this]() { return Page; })
					+ SWidgetSwitcher::Slot()[BuildHome()]
					+ SWidgetSwitcher::Slot()[BuildSetup()]
					+ SWidgetSwitcher::Slot()[BuildJoin()]
				]
			]
		]
	];
}

TSharedRef<SWidget> SRatMainMenu::BuildHome()
{
	TWeakObjectPtr<ARatMenuPlayerController> W = Owner;
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeButton(LOCTEXT("Start", "Iniciar"), FOnClicked::CreateLambda([this]() { Page = 1; return FReply::Handled(); }), FSlateColor(FLinearColor(0.85f, 0.55f, 0.05f)), 420.f, 28)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeButton(LOCTEXT("Join", "Entrar em partida (LAN)"), FOnClicked::CreateLambda([this]() { Page = 2; return FReply::Handled(); }))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SBox).WidthOverride(420.f).Padding(FMargin(0.f, 6.f))
			[
				SNew(SButton)
				.ButtonColorAndOpacity(RatUI::Button)
				.HAlign(HAlign_Center)
				.ContentPadding(FMargin(18.f, 12.f))
				.OnClicked_Lambda([W]()
				{
					RatSfx::PlayCue(nullptr, ERatSfx::UiClick);
					if (W.IsValid()) { W->CycleGraphicsQuality(); }
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Font(RatUI::Font(22))
					.ColorAndOpacity(FSlateColor(FLinearColor::White))
					.Text_Lambda([W]()
					{
						return FText::Format(LOCTEXT("Graphics", "Gráficos: {0}"), W.IsValid() ? W->GetGraphicsQualityLabel() : FText::GetEmpty());
					})
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeButton(LOCTEXT("QuitGame", "Sair"), FOnClicked::CreateLambda([W]()
			{
				if (W.IsValid()) { W->QuitGame(); }
				return FReply::Handled();
			}), FSlateColor(FLinearColor(0.45f, 0.12f, 0.1f)))
		];
}

TSharedRef<SWidget> SRatMainMenu::BuildWeaponPicker()
{
	auto WeaponButton = [this](int32 Index)
	{
		const FRatWeaponStats Stats = RatuchoDefaults::MakeWeaponStats(static_cast<ERatWeaponType>(Index));
		return SNew(SBox).WidthOverride(300.f).Padding(FMargin(6.f, 4.f))
		[
			SNew(SButton)
			.ButtonColorAndOpacity_Lambda([this, Index]() { return Weapon == Index ? RatUI::Selected : RatUI::Button; })
			.ContentPadding(FMargin(12.f, 10.f))
			.OnClicked_Lambda([this, Index]()
			{
				RatSfx::PlayCue(nullptr, ERatSfx::UiClick);
				Weapon = Index;
				return FReply::Handled();
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock).Text(Stats.DisplayName).Font(RatUI::Font(18)).ColorAndOpacity(FSlateColor(FLinearColor::White))
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 0.f)
				[
					SNew(STextBlock).Text(Stats.Description).Font(RatUI::Font(12, false)).ColorAndOpacity(FSlateColor(RatUI::Cream))
					.AutoWrapText(true).Justification(ETextJustify::Center)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("WeaponStats", "Dano {0} × {1}  •  Pente {2}"), FText::AsNumber(FMath::RoundToInt(Stats.Damage)), FText::AsNumber(Stats.ShotsPerTrigger), FText::AsNumber(Stats.MagazineSize)))
					.Font(RatUI::Font(11, false)).ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.85f, 0.7f)))
				]
			]
		];
	};

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[WeaponButton(0)]
		+ SHorizontalBox::Slot().AutoWidth()[WeaponButton(1)];
}

TSharedRef<SWidget> SRatMainMenu::BuildSetup()
{
	TWeakObjectPtr<ARatMenuPlayerController> W = Owner;
	return SNew(SVerticalBox)
		// Mapa
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeLabel(LOCTEXT("MapLabel", "MAPA"), 16, RatUI::Cheese)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SBox).WidthOverride(300.f).Padding(FMargin(0.f, 4.f, 0.f, 14.f))
			[
				SNew(SButton)
				.ButtonColorAndOpacity(RatUI::Selected)
				.HAlign(HAlign_Center)
				.ContentPadding(FMargin(12.f, 8.f))
				.OnClicked_Lambda([]() { RatSfx::PlayCue(nullptr, ERatSfx::UiClick); return FReply::Handled(); }) // único mapa: já selecionado
				[
					SNew(STextBlock).Text(LOCTEXT("Kitchen", "Cozinha (selecionado)")).Font(RatUI::Font(20)).ColorAndOpacity(FSlateColor(FLinearColor::White))
				]
			]
		]
		// Bots
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeLabel(LOCTEXT("BotsLabel", "BOTS"), 16, RatUI::Cheese)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				RatUI::MakeButton(LOCTEXT("Minus", "-"), FOnClicked::CreateLambda([this]() { NumBots = FMath::Max(0, NumBots - 1); return FReply::Handled(); }), FSlateColor(RatUI::Button), 70.f, 24)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.f, 0.f)
			[
				SNew(SBox).WidthOverride(60.f)
				[
					SNew(STextBlock).Font(RatUI::Font(30)).ColorAndOpacity(FSlateColor(FLinearColor::White)).Justification(ETextJustify::Center)
					.Text_Lambda([this]() { return FText::AsNumber(NumBots); })
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				RatUI::MakeButton(LOCTEXT("Plus", "+"), FOnClicked::CreateLambda([this]() { NumBots = FMath::Min(MaxBots, NumBots + 1); return FReply::Handled(); }), FSlateColor(RatUI::Button), 70.f, 24)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 14.f)
		[
			SNew(STextBlock).Font(RatUI::Font(12, false)).ColorAndOpacity(FSlateColor(RatUI::Cream))
			.Text(FText::Format(LOCTEXT("BotsHint", "Máximo inicial: {0} bots (limite de desempenho testado). Você não conta como bot."), FText::AsNumber(MaxBots)))
		]
		// Arma
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeLabel(LOCTEXT("WeaponLabel", "ARMA"), 16, RatUI::Cheese)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 18.f)
		[
			BuildWeaponPicker()
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeButton(LOCTEXT("Go", "Começar partida"), FOnClicked::CreateLambda([this, W]()
			{
				if (W.IsValid()) { W->StartMatch(NumBots, Weapon); }
				return FReply::Handled();
			}), FSlateColor(FLinearColor(0.85f, 0.55f, 0.05f)), 420.f, 26)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeButton(LOCTEXT("Back", "Voltar"), FOnClicked::CreateLambda([this]() { Page = 0; return FReply::Handled(); }), FSlateColor(RatUI::Button), 260.f, 18)
		];
}

TSharedRef<SWidget> SRatMainMenu::BuildJoin()
{
	TWeakObjectPtr<ARatMenuPlayerController> W = Owner;
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeLabel(LOCTEXT("HostIP", "ENDEREÇO DO HOST"), 16, RatUI::Cheese)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f)
		[
			SNew(SBox).WidthOverride(420.f)
			[
				SAssignNew(AddressBox, SEditableTextBox)
				.Text(FText::FromString(TEXT("127.0.0.1")))
				.Font(RatUI::Font(22, false))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 12.f)
		[
			SNew(STextBlock).Font(RatUI::Font(12, false)).ColorAndOpacity(FSlateColor(RatUI::Cream)).AutoWrapText(true)
			.Text(LOCTEXT("JoinHint", "O host precisa ter iniciado uma partida. Mesma máquina: 127.0.0.1. Outra máquina na LAN: IP dela (porta UDP 7777 liberada no firewall)."))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeLabel(LOCTEXT("WeaponLabel2", "ARMA"), 16, RatUI::Cheese)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 18.f)
		[
			BuildWeaponPicker()
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeButton(LOCTEXT("Connect", "Conectar"), FOnClicked::CreateLambda([this, W]()
			{
				if (W.IsValid()) { W->JoinMatch(AddressBox.IsValid() ? AddressBox->GetText().ToString() : FString(), Weapon); }
				return FReply::Handled();
			}), FSlateColor(FLinearColor(0.85f, 0.55f, 0.05f)))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			RatUI::MakeButton(LOCTEXT("Back2", "Voltar"), FOnClicked::CreateLambda([this]() { Page = 0; return FReply::Handled(); }), FSlateColor(RatUI::Button), 260.f, 18)
		];
}

#undef LOCTEXT_NAMESPACE
