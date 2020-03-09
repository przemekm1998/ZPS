/*
 * Projekt z Zastosowania Procesor�w Sygna�owych
 * Szablon projektu dla DSP TMS320C5535
 * Maciej Nosek i Przemek Micha�ek p�ytka nr 3
 */


// Do��czenie wszelkich potrzebnych plik�w nag��wkowych
#include "usbstk5515.h"
#include "usbstk5515_led.h"
#include "aic3204.h"
#include "PLL.h"
#include "bargraph.h"
#include "oled.h"
#include "pushbuttons.h"
#include "dsplib.h"


// Cz�stotliwo�� pr�bkowania (48 kHz)
#define CZESTOTL_PROBKOWANIA 48000L

// Wzmocnienie wej�cia w dB: 0 dla wej�cia liniowego, 30 dla mikrofonowego
#define WZMOCNIENIE_WEJSCIA_dB 30

DATA bufor[5];
const int wsp_iir[] = {62, 124, 62, -28801, 12665};

// G�owna procdura programu

void main(void) {

	// Inicjalizacja uk�adu DSP
	USBSTK5515_init();			// BSL
	pll_frequency_setup(100);	// PLL 100 MHz
	aic3204_hardware_init();	// I2C
	aic3204_init();				// kodek d�wi�ku AIC3204
	USBSTK5515_ULED_init();		// diody LED
	SAR_init_pushbuttons();		// przyciski
	oled_init();				// wy�wielacz LED 2x19 znak�w

	// ustawienie cz�stotliwo�ci pr�bkowania i wzmocnienia wej�cia
	set_sampling_frequency_and_gain(CZESTOTL_PROBKOWANIA, WZMOCNIENIE_WEJSCIA_dB);

	// wypisanie komunikatu na wy�wietlaczu
	// 2 linijki po 19 znak�w, tylko wielkie angielskie litery
	oled_display_message("PROJEKT ZPS        ", "                   ");

	// 'krok' oznacza tryb pracy wybrany przyciskami
	unsigned int krok = 0;
	unsigned int poprzedni_krok = 99;

	// zmienne do przechowywania warto�ci pr�bek
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

	// Przetwarzanie pr�bek sygna�u w p�tli
	while (1) {

		// odczyt pr�bek audio, zamiana na mono
		aic3204_codec_read(&lewy_wejscie, &prawy_wejscie);
		mono_wejscie = (lewy_wejscie >> 1) + (prawy_wejscie >> 1);

		// sprawdzamy czy wci�ni�to przycisk
		// argument: maksymalna liczba obs�ugiwanych tryb�w
		krok = pushbuttons_read(4);
		if (krok == 0) // oba wci�ni�te - wyj�cie
			break;
		else if (krok != poprzedni_krok) {
			// nast�pi�a zmiana trybu - wci�ni�to przycisk
			USBSTK5515_ULED_setall(0x0F); // zgaszenie wszystkich di�d
			if (krok == 1) {
				// wypisanie informacji na wy�wietlaczu
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


		// zadadnicze przetwarzanie w zale�no�ci od wybranego kroku

		if (krok == 1) {
			// tryb podstawowy - kopiowanie sygna�u
			lewy_wyjscie = lewy_wejscie;
			prawy_wyjscie = prawy_wejscie;
			// Zadanie 1: st�umienie sygna�u mono_wejscie o 10 dB
//			lewy_wyjscie = (int)((spadek_10_db * (long)mono_wejscie) >> 15);
			lewy_wyjscie = _smpy(mono_wejscie, spadek_10_db);

			// Zadanie 2: przetworzenie sygna�u mono_wejscie przez filtr IIR
			// o wsp�czynnikach Q15:
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

		// zapisanie warto�ci na wyj�cie audio
		aic3204_codec_write(lewy_wyjscie, prawy_wyjscie);

	}


	// zako�czenie pracy - zresetowanie kodeka
	aic3204_disable();
	oled_display_message("KONIEC PRACY       ", "                   ");
	//SW_BREAKPOINT;
}
