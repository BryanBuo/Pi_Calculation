/*
 * Calculating_PI.c
 *
 * Created: 27.10.2023 18:32:07
 * Author : Bryan Buonocore
 */ 

#include "math.h"
#include "string.h"
#include "stdio.h"
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"
#include "ButtonHandler.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"
#include "semphr.h"
#include "mem_check.h"

#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"

//Für Event Bits für Button konfiguration 
#define EVBUTTONS_S1    1<<0
#define EVBUTTONS_S2    1<<1
#define EVBUTTONS_S3    1<<2
#define EVBUTTONS_S4    1<<3
#define EVBUTTONS_CLEAR 0xFF

//Event Gruppen
EventGroupHandle_t evButtonEvents;
EventGroupHandle_t vStartBitLeibniz;
EventGroupHandle_t vStartBitNilkantha;
EventGroupHandle_t vStopBit;

// Gobale Variabeln für Leibniz
float pi_Leibniz = 0;
float pi4 = 1;			
float n = 4;

// Gobale Variabeln für Nilkantha
float pi_Nilkantha = 0.0;
float pi = 3.0;
uint32_t k = 0;
float sign = 1;

//Globale Variable für Zeitmessung
int runningLeibniz = 0;
int runningNilkantha = 0;

//Die Task Handle für von Task zu Task switchen
TaskHandle_t LeibnizTask = NULL;
TaskHandle_t NilkanthaTask = NULL;

//TickType Variabeln für die Zeitmessung
TickType_t timeLeibniz = 0;
TickType_t timeNilkantha = 0;
TickType_t time = 0;

// Definitzion für Methoden
typedef enum{
	Leibniz,
	Nilkantha
} Algorithmus;
	
// Aktuelle Algorithmus	
Algorithmus aktuellAlgorithmus = Leibniz;

//Funktionen Deklatrion
extern void vApplicationIdleHook( void );
void vLeibnizTask(void *pvParameters);
void vInterfaceTask(void *pvParameters);
void vNilkanthaTask(void *pvParameters);
void vButtonTask(void *pvParameters);

void vApplicationIdleHook( void ){	
	//momentan leer
}

// Hauptfunktion
int main(void){

	vInitClock();  //Zeit inizialisierung	
	vInitDisplay();//Display inizialisierung
	
	//Event Gruppen herstellen 
	evButtonEvents = xEventGroupCreate();
	vStartBitLeibniz = xEventGroupCreate();
	vStartBitNilkantha = xEventGroupCreate();
	vStopBit = xEventGroupCreate();
	
	//FreeRTOS Tasks mit Prioritäten
	xTaskCreate(vLeibnizTask, "LeibnizTask", configMINIMAL_STACK_SIZE + 200, NULL, 1, &LeibnizTask);
	xTaskCreate(vNilkanthaTask, "NilkanthaTask", configMINIMAL_STACK_SIZE + 200, NULL,1, &NilkanthaTask);
	xTaskCreate(vButtonTask, "ButtonTask", configMINIMAL_STACK_SIZE + 50, NULL, 3, NULL);
	xTaskCreate(vInterfaceTask, "InterfaceTask", configMINIMAL_STACK_SIZE + 200, NULL, 2, NULL);

	//Start für den Scheduler
	vTaskStartScheduler();
	
	return 0;
}

//Berechnungen von der Leibniz-Reihe für PI
void vLeibnizTask(void *pvParameters){
	
	for(;;){
		//Button Drücken damit die Berechnung startet
		if (xEventGroupGetBits(vStartBitLeibniz) == 1){
			
			//Schaut das die Zeit nur einmal anfängt zu laufen
			if(runningLeibniz == 1 ){
				time = xTaskGetTickCount();
				runningLeibniz = 0;
			}
			
				//Pi formel
				pi4 = pi4-(1.0/(n-1))+(1.0/(n+1));
				n += 4;
				pi_Leibniz = pi4 * 4;
				
			//Zeit Stop sobald Ip vollständig berechnet ist
			if(pi_Leibniz > 3.14159 && pi_Leibniz < 3.14160){
				timeLeibniz = xTaskGetTickCount() - time;
				vTaskDelay(portMAX_DELAY);
			}
			
			// stop Signal
			if (xEventGroupGetBits(vStopBit) == 1) {
				xEventGroupSetBits(vStartBitLeibniz, 0);
			}
		}	
	}
}

//Berechnungen von der Methode von Nilkantha für PI
void vNilkanthaTask(void *pvParameters){
	
	for(;;){
		//Button Drücken damit die Berechnung startet
		if (xEventGroupGetBits(vStartBitNilkantha) == 1){
			
			//Schaut das die Zeit nur einmal anfängt zu laufen
			if(runningNilkantha == 1 ){
				time = xTaskGetTickCount();
				runningNilkantha = 0;
			}
			
				//Pi formel
				pi += sign * (4.0 / ((2*k+2) * (2*k+3) * (2*k+4)));
				
				sign = -sign;
				k++;	
				
				pi_Nilkantha = pi;
				
			//Zeit Stop sobald Ip vollständig berechnet ist
			if(pi_Nilkantha > 3.14159 && pi_Nilkantha < 3.14160){
				timeNilkantha  = xTaskGetTickCount() - time;
				vTaskDelay(portMAX_DELAY);
			}
			
			// stop Signal
			if (xEventGroupGetBits(vStopBit) == 1) {
				xEventGroupSetBits(vStartBitNilkantha, 0);
			}
		}
	}
}

// Button Handling und setzen von event Bits
void vButtonTask(void *pvParameters){
	initButtons();   //Buttons inizialisieren
	for(;;) {
		updateButtons(); //Updaten der Button
		
		//Button state setzten und für den entsprechenden Button druck den richtigen Eventbit setzten
		if(getButtonPress(BUTTON1) == SHORT_PRESSED) {
			xEventGroupSetBits(evButtonEvents, EVBUTTONS_S1);
		}
		if(getButtonPress(BUTTON2) == SHORT_PRESSED) {
			xEventGroupSetBits(evButtonEvents, EVBUTTONS_S2);
		}
		if(getButtonPress(BUTTON3) == SHORT_PRESSED) {
			xEventGroupSetBits(evButtonEvents, EVBUTTONS_S3);
		}
		if(getButtonPress(BUTTON4) == SHORT_PRESSED) {
			xEventGroupSetBits(evButtonEvents, EVBUTTONS_S4);
		}
		vTaskDelay((1000/BUTTON_UPDATE_FREQUENCY_HZ)/portTICK_RATE_MS); //Buttonupdate Delay
	}
}

// Task für Display switch case mit Buttons und event Bits
void vInterfaceTask(void *pvParameters){
	
	for (;;){

		uint32_t buttonState = (xEventGroupGetBits(evButtonEvents)) & 0x000000FF; // Button State
		xEventGroupClearBits(evButtonEvents, EVBUTTONS_CLEAR);  // Buttons clearen
		
		switch(buttonState){
			
			case EVBUTTONS_S1:   
			if (aktuellAlgorithmus == Leibniz){ // Start Leibniz
				xEventGroupSetBits(vStartBitLeibniz, 1);
				runningLeibniz = 1;
				
			}else if(aktuellAlgorithmus == Nilkantha){ // Start Nilkantha
				xEventGroupSetBits(vStartBitNilkantha, 1);
				runningNilkantha = 1;
			}
			break;
			
			case EVBUTTONS_S2:   // Stop	
			xEventGroupSetBits(vStopBit, 1);
			break;
			
			case EVBUTTONS_S3:   // Reset
			if (aktuellAlgorithmus == Leibniz){
				timeLeibniz = 0;
				pi_Leibniz = 0.0;
				pi4 = 1;
				n = 4;
				xEventGroupSetBits(vStartBitLeibniz, 0);
				
			}else if(aktuellAlgorithmus == Nilkantha){
				timeNilkantha = 0;
				pi_Nilkantha = 0.0;
				pi = 3.0;
				k = 0;
				sign = 1;
				xEventGroupSetBits(vStartBitNilkantha, 0);
			}
			break;
			
			case EVBUTTONS_S4:	 // Gewünschter Algorithmus setzten
			if (aktuellAlgorithmus == Leibniz){
				vTaskSuspend(LeibnizTask);
				vTaskResume(NilkanthaTask);
				aktuellAlgorithmus = Nilkantha;
			}else if(aktuellAlgorithmus == Nilkantha){
				vTaskSuspend(NilkanthaTask);
				vTaskResume(LeibnizTask);
				aktuellAlgorithmus = Leibniz;
			}
			break;
			
		}
		
		// Ausgabe der Zeit und von Pi auf den Bildschirm für Leibniz
		if (aktuellAlgorithmus == Leibniz){
			
			char pistring_Leibniz[20];
			char timestring_Leibniz[20];
			
			vDisplayClear();
			vDisplayWriteStringAtPos(0,0,"PI-Leibniz");
			
			sprintf(&pistring_Leibniz[0], "Pi: %.5f", pi_Leibniz);
			vDisplayWriteStringAtPos(1,0, "%s", pistring_Leibniz);
			
			sprintf(&timestring_Leibniz[0], "Time: %.2i ms", timeLeibniz);
			vDisplayWriteStringAtPos(2,0, "%s", timestring_Leibniz);
			
		}
		
		// Ausgabe der Zeit und von Pi auf den Bildschirm für Nilkantha
		if (aktuellAlgorithmus == Nilkantha){
			
			char pistring_Nilkantha[20];
			char timestring_Nilkantha[20];
			
			vDisplayClear();
			vDisplayWriteStringAtPos(0,0,"PI-Nilkantha");
			
			sprintf(&pistring_Nilkantha[0], "Pi: %.5f", pi_Nilkantha);
			vDisplayWriteStringAtPos(1,0, "%s", pistring_Nilkantha);
			
			sprintf(&timestring_Nilkantha[0], "Time: %.2i ms", timeNilkantha);
			vDisplayWriteStringAtPos(2,0, "%s", timestring_Nilkantha);
			
		}
		
			vDisplayWriteStringAtPos(3,0,"|STR |STP |RES |SET");
		
		vTaskDelay(pdMS_TO_TICKS(500));   //Update von Angezeigter Wert
	}
}
	
	
