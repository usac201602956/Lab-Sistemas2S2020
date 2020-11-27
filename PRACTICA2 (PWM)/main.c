/********************LIBRERIAS DE C*************/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "HASLCD_JR.h"

/**********LIBRERIAS************************/
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"

/**********DEFINICIONES********************/
#define Freq 1000

/*************VARIABLES********************/
uint32_t Load, PWMClock;
int Ciclo_trabajo = 0;
bool Cambio_giro = false;

/*************RUTINA DE INTERRUPCIÓN*********************/

void BotonesTivaC(void){
    uint32_t status;

    status = GPIOIntStatus(GPIO_PORTF_BASE, true);
    GPIOIntClear(GPIO_PORTF_BASE, status);         //Limpiar interrupción por PF0 ó PF4.

        if(status == GPIO_INT_PIN_0){
            Ciclo_trabajo++;
            if(Ciclo_trabajo > 3){
                Ciclo_trabajo = 0;
            }
//            has_lcd_erase();
//            SysCtlDelay((SysCtlClockGet()*0.3)/3);
        }else if(status == GPIO_INT_PIN_4){
            if(Cambio_giro){
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2|GPIO_PIN_3, 0);
                SysCtlDelay((SysCtlClockGet()*0.5)/3);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2|GPIO_PIN_3, 8);
                Cambio_giro = false;
            }else{
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2|GPIO_PIN_3, 0);
                SysCtlDelay((SysCtlClockGet()*0.5)/3);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2|GPIO_PIN_3, 4);
                Cambio_giro = true;
            }
        }
}

/*************MÉTODOS*********************/
void Configuraciones(void){
    SysCtlClockSet(SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ|SYSCTL_SYSDIV_2_5);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //Desbloqueo de Pin F0
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2|GPIO_PIN_3);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4);                                       //PF0, PF4 como entradas.
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); //PF0, PF4 con resistencia PULL-UP.
}

void ConfigINTERRUPCIONES(void) {
    IntMasterEnable();                                                         //Habilitar interrupciones globales del periférico NVICs.
    IntEnable(INT_GPIOF);                                                      //Habilitar las interrupciones de puerto F.
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_0|GPIO_INT_PIN_4);             //Habilitar la interrupción por PF0 y PF4.
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4, GPIO_FALLING_EDGE); //Tipo de interrupcion por flanco de bajada para PF0 y PF4.
}

void configPWM(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
    SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);
    GPIOPinConfigure(GPIO_PF1_M1PWM5);

    PWMClock = SysCtlClockGet()/64;
    Load = (PWMClock/Freq)-1;

    PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, Load);
    PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);
    PWMGenEnable(PWM1_BASE, PWM_GEN_2);
}

/*************MAIN***********************/
int main(void) {

    Configuraciones();
    ConfigINTERRUPCIONES();
    configPWM();
    has_lcd_4bitsetup();
    has_lcd_erase();

    while(true){

        switch(Ciclo_trabajo){
        case 0:
            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, (10*Load)/100);
            has_lcd_write(1,1,"DUTY CYCLE:");
            has_lcd_write(2,3,"10");
            has_lcd_write(2,5,"%");
            break;
        case 1:
            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, (50*Load)/100);
            has_lcd_write(1,1,"DUTY CYCLE:");
            has_lcd_write(2,3,"50");
            has_lcd_write(2,5,"%");
            break;
        case 2:
            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, (75*Load)/100);
            has_lcd_write(1,1,"DUTY CYCLE:");
            has_lcd_write(2,3,"75");
            has_lcd_write(2,5,"%");
            break;
        case 3:
            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, (90*Load)/100);
            has_lcd_write(1,1,"DUTY CYCLE:");
            has_lcd_write(2,3,"90");
            has_lcd_write(2,5,"%");
            break;
        default:
            break;
        }
    }
}
