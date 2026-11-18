#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class ARatPlayerController;
class ARatMenuPlayerController;
class SEditableTextBox;

/** Menu dentro da partida: pausa (Esc) ou botões da tela de resultado. */
class RATUCHO_API SRatInGameMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRatInGameMenu) : _bEndScreen(false) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ARatPlayerController>, Owner)
		SLATE_ARGUMENT(bool, bEndScreen)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TWeakObjectPtr<ARatPlayerController> Owner;
};

/** Menu principal: Iniciar → mapa, bots, arma → começar. Também entrar em partida LAN e gráficos. */
class RATUCHO_API SRatMainMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRatMainMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ARatMenuPlayerController>, Owner)
		SLATE_ARGUMENT(int32, MaxBots)
		SLATE_ARGUMENT(int32, DefaultBots)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TSharedRef<SWidget> BuildHome();
	TSharedRef<SWidget> BuildSetup();
	TSharedRef<SWidget> BuildJoin();
	TSharedRef<SWidget> BuildWeaponPicker();

	TWeakObjectPtr<ARatMenuPlayerController> Owner;
	TSharedPtr<SEditableTextBox> AddressBox;
	int32 Page = 0;
	int32 MaxBots = 7;
	int32 NumBots = 5;
	int32 Weapon = 0;
};
