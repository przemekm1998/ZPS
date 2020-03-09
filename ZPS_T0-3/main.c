/*
 * Projekt z Zastosowania Procesorów Sygna³owych
 * Szablon projektu dla DSP TMS320C5535
 * Maciej Nosek i Przemek Micha³ek p³ytka nr 3
 */


// Do³¹czenie wszelkich potrzebnych plików nag³ówkowych
#include "usbstk5515.h"
#include "usbstk5515_led.h"
#include "aic3204.h"
#include "PLL.h"
#include "bargraph.h"
#include "oled.h"
#include "pushbuttons.h"
#include "dsplib.h"


// Czêstotliwoœæ próbkowania (48 kHz)
#define CZESTOTL_PROBKOWANIA 48000L

// Wzmocnienie wejœcia w dB: 0 dla wejœcia liniowego, 30 dla mikrofonowego
#define WZMOCNIENIE_WEJSCIA_dB 30

DATA bufor[5];
const int wsp_iir[] = {62, 124, 62, -28801, 12665};

// G³owna procdura programu

void main(void) {

	// Inicjalizacja uk³adu DSP
	USBSTK5515_init();			// BSL
	pll_frequency_setup(100);	// PLL 100 MHz
	aic3204_hardware_init();	// I2C
	aic3204_init();				// kodek dŸwiêku AIC3204
	USBSTK5515_ULED_init();		// diody LED
	SAR_init_pushbuttons();		// przyciski
	oled_init();				// wyœwielacz LED 2x19 znaków

	// ustawienie czêstotliwoœci próbkowania i wzmocnienia wejœcia
	set_sampling_frequency_and_gain(CZESTOTL_PROBKOWANIA, WZMOCNIENIE_WEJSCIA_dB);

	// wypisanie komunikatu na wyœwietlaczu
	// 2 linijki po 19 znaków, tylko wielkie angielskie litery
	oled_display_message("PROJEKT ZPS        ", "                   ");

	// 'krok' oznacza tryb pracy wybrany przyciskami
	unsigned int krok = 0;
	unsigned int poprzedni_krok = 99;

	// zmienne do przechowywania wartoœci próbek
	Int16 lewy_wejscie;
	Int16 prawy_wejscie;
	Int16 lewy_wyjscie;
	Int16 prawy_wyjscie;
	Int16 mono_wejscie;
	const Int16 spadek_10_db = 10362;

	int i = 0;
	for (i = 0; i < 5; i++){
		bufor[i] = 0;
	}

	// Przetwarzanie próbek sygna³u w pêtli
	while (1) {

		// odczyt próbek audio, zamiana na mono
		aic3204_codec_read(&lewy_wejscie, &prawy_wejscie);
		mono_wejscie = (lewy_wejscie >> 1) + (prawy_wejscie >> 1);

		// sprawdzamy czy wciœniêto przycisk
		// argument: maksymalna liczba obs³ugiwanych trybów
		krok = pushbuttons_read(4);
		if (krok == 0) // oba wciœniête - wyjœcie
			break;
		else if (krok != poprzedni_krok) {
			// nast¹pi³a zmiana trybu - wciœniêto przycisk
			USBSTK5515_ULED_setall(0x0F); // zgaszenie wszystkich diód
			if (krok == 1) {
				// wypisanie informacji na wyœwietlaczu
				oled_display_message("PROJEKT ZPS        ", "KROK 1             ");
				// zapalenie diody nr 1
				USBSTK5515_ULED_on(0);
			} else if (krok == 2) {
				oled_display_message("PROJEKT ZPS        ", "KROK 2             ");
				USBSTK5515_ULED_on(1);
			} else if (krok == 3) {
				oled_display_message("PROJEKT ZPS        ", "KROK 3             ");
				USBSTK5515_ULED_on(2);
			} else if (krok == 4) {
				oled_display_message("PROJEKT ZPS        ", "KROK 4             ");
				USBSTK5515_ULED_on(3);
			}
			// zapisujemy nowo ustawiony tryb
			poprzedni_krok = krok;
		}


		// zadadnicze przetwarzanie w zale¿noœci od wybranego kroku

		if (krok == 1) {
			// tryb podstawowy - kopiowanie sygna³u
			lewy_wyjscie = lewy_wejscie;
			prawy_wyjscie = prawy_wejscie;
			// Zadanie 1: st³umienie sygna³u mono_wejscie o 10 dB
//			lewy_wyjscie = (int)((spadek_10_db * (long)mono_wejscie) >> 15);
			lewy_wyjscie = _smpy(mono_wejscie, spadek_10_db);

			// Zadanie 2: przetworzenie sygna³u mono_wejscie przez filtr IIR
			// o wspó³czynnikach Q15:
			// 62, 124, 62, -28801, 12665

			ushort oflag = iircas51(&mono_wejscie, (DATA*) wsp_iir, &lewy_wyjscie, bufor, 1, 1);
			prawy_wyjscie = lewy_wyjscie;

		} else if (krok == 2) {
			lewy_wyjscie = 0;
			prawy_wyjscie = 0;

		} else if (krok == 3) {
			lewy_wyjscie = 0;
			prawy_wyjscie = 0;

		} else if (krok == 4) {
			lewy_wyjscie = 0;
			prawy_wyjscie = 0;
		}

		// zapisanie wartoœci na wyjœcie audio
		aic3204_codec_write(lewy_wyjscie, prawy_wyjscie);

	}


	// zakoñczenie pracy - zresetowanie kodeka
	aic3204_disable();
	oled_display_message("KONIEC PRACY       ", "                   ");
	//SW_BREAKPOINT;
}
