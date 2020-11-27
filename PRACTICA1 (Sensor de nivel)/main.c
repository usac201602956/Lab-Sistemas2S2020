/*--------------------------------------------LIBRERÍAS DE C-------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
/*-----------------------------------------LIBRERÍAS PARA LA TIVA C------------------------------------------*/
#include "inc/hw_memmap.h"
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"      //Periféricos de entrada y salida
#include "inc/hw_gpio.h"         //Para desbloquear pines
#include "driverlib/interrupt.h" //Para interrupciones
#include "driverlib/timer.h"     //Para activar el timer
#include "driverlib/uart.h"      //Para protocolo UART
#include "driverlib/pin_map.h"   //Configurar pines de GPIO
#include "HASLCD_JR.h"

/*-----------------------------------------------VARIABLES----------------------------------------------------*/
int StateTrigger = 0;
int Echo = 1;
uint32_t Load = 8000000, T1, T2;
double Time;

double DistTotal = 60, DistMedida = 0, NivelAgua = 0, Porcentaje = 0;
char DistTotal_H[] = "", DistMedida_d[] = "", NivelAgua_N[] = "", Porcentaje_P[] = "";
int mostrar = 0;
bool flag = true;

/*-----------------------------------------RUTINA DE INTERRUPCI�N---------------------------------------------*/
void Int_Trigger(void){
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);              //Limpiar interrupci�n por fin de conteo de subTimer A - Timer 0
    if (StateTrigger == 0 ){
        StateTrigger = 128;
        Load = 800;                                              //Carga para que subTimero A de Timer 0 = 10us
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, StateTrigger); //Establecer Pin PA7 (Trigger) en alto
    }else{
        StateTrigger = 0;
        Load = 8000000;                                          //Carga para que subTimer A de Timer 0 = 100ms
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, StateTrigger); //Establecer Pin PA7 (Trigger) en bajo
    }
    TimerDisable(TIMER0_BASE, TIMER_A);  //Deshabilitar subTimer A de Timer 0
    TimerEnable(TIMER0_BASE, TIMER_A);   //Habilitar subTimer A de Timer 0
}

void BotonesTivaC(void){
    uint32_t status;

    status = GPIOIntStatus(GPIO_PORTF_BASE, true);
    GPIOIntClear(GPIO_PORTF_BASE, status);         //Limpiar interrupci�n por PF0 � PF4.

        if(status == 1){
            mostrar++;
            if(mostrar > 2){
                mostrar = 0;
            }
            has_lcd_erase();
            SysCtlDelay((SysCtlClockGet()*0.3)/3);
        }else if(status == 16){
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
            flag = false;
        }
}

void Int_GPIO(void){
    uint32_t Status;
    Status = GPIOIntStatus(GPIO_PORTA_BASE, true);
    GPIOIntClear(GPIO_PORTA_BASE, Status);         //Limpiar interrupci�n por PA6

    if (Echo == 1){
        T1 = TimerValueGet(TIMER1_BASE, TIMER_A);
        Echo = 0;
    }else{
        T2 = TimerValueGet(TIMER1_BASE, TIMER_A);
        Echo = 1;
        Time = T1 - T2;
        DistMedida = ((Time*340000)/(2*SysCtlClockGet())) - 45;  //Calculo de la distancia medida en mm.
            if(DistTotal > DistMedida){
                NivelAgua = (DistTotal - DistMedida);            //Calculo del nivel medido en mm.
            }else{
                NivelAgua = 0;
            }
        Porcentaje = (NivelAgua*100)/DistTotal;
        if(Porcentaje > 80 & flag){
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 2);
        }else if(Porcentaje < 80 & !flag){
            flag = true;
        }

    }
}

/*----------------------------------------------PROCEDIMIENTOS-------------------------------------------------*/
void GPIO_Config(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);  //Activar puerto A
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);  //Activar puerto F

    //Desbloqueo de Pin F0
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;

    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_6);                                                  //PA6 = Echo, como entrada
    GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_7);                                                 //PA7 = Trigger, como salida
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4);                                       //PF0, PF4 como entradas.
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);                                                 //PF1 como salida.
    GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_6, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);            //PA6 con resistencia PULL-DOWN.
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); //PF0, PF4 con resistencia PULL-UP.
}

void ECHO_interrup(void) {
    IntMasterEnable();                                                         //Habilitar interrupciones globales del perif�rico NVICs.
    IntEnable(INT_GPIOA);                                                      //Habilitar las interrupciones de puerto A.
    IntEnable(INT_GPIOF);                                                      //Habilitar las interrupciones de puerto F.
    GPIOIntEnable(GPIO_PORTA_BASE, GPIO_INT_PIN_6);                            //Habilitar la interrupci�n por PA6 (Echo).
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_0|GPIO_INT_PIN_4);             //Habilitar la interrupci�n por PF0 y PF4.
    GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_6, GPIO_BOTH_EDGES);              //Tipo de interrupcion por flanco de subida y bajada para PA6 (Echo).
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4, GPIO_FALLING_EDGE); //Tipo de interrupcion por flanco de bajada para PF0 y PF4.
    IntPrioritySet(INT_GPIOA, 0);                                              //Prioridad de interrupcion 0.
    IntPrioritySet(INT_GPIOF, 2);                                              //Prioridad de interrupcion 2.
}

void TIMERS_Config(void) {
    //TIMER0A - Disparo
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);    //Habilitar Timer 0
    TimerConfigure(TIMER0_BASE, TIMER_CFG_ONE_SHOT); //Configurar el tipo de Timer como "un disparo"
    TimerLoadSet(TIMER0_BASE, TIMER_A, Load);        //Cargar subTimer A
    TimerEnable(TIMER0_BASE, TIMER_A);               //Habilitar subTimer A de Timer 0
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT); //Interrupci�n por fin de conteo
    IntEnable(INT_TIMER0A);                          //Habilitar interrupci�n por subTimer A de Timer 0
    IntPrioritySet(INT_TIMER0A, 1);                  //Prioridad de interrupcion 1

    //TIMER1A - Peri�dico
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);         //Habilitar Timer 1
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);      //Configurar el tipo de Timer como "peri�dico"
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet()); //Cargar subTimer A con 80M
    TimerEnable(TIMER1_BASE, TIMER_A);                    //Habilitar subTimer A de Timer 1
}

/*--------------------------------------------------MAIN---------------------------------------------------------*/

int main(void) {
    SysCtlClockSet(SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ|SYSCTL_SYSDIV_2_5); //Reloj de Tiva C a 80MHz
    GPIO_Config();
    ECHO_interrup();
    TIMERS_Config();
    has_lcd_4bitsetup();
    has_lcd_erase();

    while(true) {

        switch(mostrar){
        case 1:
            has_lcd_write(1,1,"TOTAL :");
            ltoa(DistTotal, DistTotal_H, 10);
            has_lcd_write(1,9,DistTotal_H);
            has_lcd_write(1,13,"mm");

            has_lcd_write(2,1,"MEDIDA:");
            ltoa(DistMedida, DistMedida_d, 10);
            has_lcd_write(2,9,DistMedida_d);
            has_lcd_write(2,13,"mm");
            break;
        case 0:
            has_lcd_write(1,1,"NIVEL :");
            ltoa(NivelAgua, NivelAgua_N, 10);
            has_lcd_write(1,9,NivelAgua_N);
            has_lcd_write(1,13,"mm");

            has_lcd_write(2,1,"PORCEN:");
            ltoa(Porcentaje, Porcentaje_P, 10);
            has_lcd_write(2,9,Porcentaje_P);
            has_lcd_write(2,13,"% ");
            break;
        default:
            break;
        }
    }
}
