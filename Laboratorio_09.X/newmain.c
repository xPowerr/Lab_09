/*
 * File:   newmain.c
 * Author: Gabriel Carrera 21216
 *
 * Created on April 21, 2023, 8:46 AM
 */

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

#include <xc.h>
#include <stdint.h>
#include <stdio.h>
#include <pic16f887.h>


#define _XTAL_FREQ 1000000 //frec de 1 MHz
//#define dirEEPROM 0x04

// --------------- Variables ---------------
uint8_t address = 0x01; // variable con la deireccion de los datodos en epeprom
int dormir = 0 ; //bandera para indicar si esta en modo sleep 

uint8_t pot;

// --------------- Prototipos --------------- 
void setup(void);
void setupADC(void);
void EEPROMWRITE(uint8_t data, uint8_t address); // Escritura de eeprom
uint8_t EEPROMREAD(uint8_t adress); // Lectura del eeprom


// --------------- Rutina de  interrupciones --------------- 
void __interrupt() isr(void){
    
    // ---- INTERRUPCION DEL ADC --------
    if (PIR1bits.ADIF == 1){ // Chequear bandera del conversor ADC
        if (ADCON0bits.CHS == 0b0000){ // si está en ADC AN0, RA1
            pot = ADRESH; // Pasar valor del ADRESH a variable pot
            PORTC = pot; // Pasar el valor de la variable al puerto C
            }
        PIR1bits.ADIF = 0; // limpiar la bandera de la interrupcion 
    }
    
    // --------------- Interrupcion PORTB --------------- 
    if (INTCONbits.RBIF){ // Chequear bandera del PORTB
        if (PORTBbits.RB7 == 0){ // Chequear si se presiona RB7
            dormir = 0; // Establecer la bandera de dormir como 0 
            PORTEbits.RE0 = 1;
        }
        else if (PORTBbits.RB6 == 0){ // Chequear si se presiona RB6
            dormir = 1; // Establecer la bandera de dormir como 1
            SLEEP(); // Activar modo sleep
            PORTEbits.RE0 = 0;
        }
        else if (PORTBbits.RB5 == 0){
            dormir = 0 ; // Establecer la bandera de dormir como 0 
            EEPROMWRITE(address, pot); // Escribir valor del potenciometro en el eeprom
            PORTEbits.RE0 = 1;
        }
        INTCONbits.RBIF = 0; // limpiar la bandera de la interrupcion
    }
}


// --------------- main ---------------
void main(void){
    setup();
    setupADC();
    
    while(1){
        __delay_ms(1);
        if (dormir == 0){ // Revisar si no esta een modo sleep con la bandera
            if (ADCON0bits.GO == 0){ // Si la lectura del ADC se desactiva
                ADCON0bits.GO = 1; // Activar lectura del ADC
                __delay_us(20);
            }   
        }
        PORTD = EEPROMREAD(address); // Asignar al PORTD el valor que se lea de la EEPROM 
    }
}


// --------------- Setup General ---------------
void setup(void){

// --------------- Definir analogicas ---------------
    ANSEL = 0b00000001; // Habilitar AN0 como analogica
    ANSELH = 0x00;

// --------------- Configurar puertos ---------------    
    TRISBbits.TRISB5 = 1; // RB5 como entrada
    TRISBbits.TRISB6 = 1; // RB6 como entrada
    TRISBbits.TRISB7 = 1; // RB7 como entrada
    TRISC = 0;            // Habilitar PORTC como salida
    TRISD = 0;            // Habilitar PORTD como salida
    TRISE = 0;            // Habilitar PORTE como salida
    
// --------------- Limpiar puertos ---------------    
    PORTA = 0;
    PORTB = 0;
    PORTC = 0;
    PORTD = 0;
    PORTE = 0;

// --------------- Habilitar pullups --------------- 
    OPTION_REGbits.nRBPU = 0; 
    WPUBbits.WPUB5 = 1;
    WPUBbits.WPUB6 = 1;
    WPUBbits.WPUB7 = 1; 

// --------------- Banderas e interrupciones --------------- 
    INTCONbits.GIE = 1;   // Habilitar interrupciones globales
    INTCONbits.PEIE = 1;  // Habilitar interrupciones de perifericas
    INTCONbits.RBIE = 1;  // Habilitar interrupciones en PORTB
    
    IOCBbits.IOCB5 = 1;   // Habilitar interrupciones en RB5
    IOCBbits.IOCB6 = 1;   // Habilitar interrupciones en RB6
    IOCBbits.IOCB7 = 1;   // Habilitar interrupciones en RB7
    
    INTCONbits.RBIF = 0;  // Limpiar bandera de interrupcion de PORTB
    PIR1bits.ADIF = 0;    // Limpiar bandera de interrupcion de ADC
    PIE1bits.ADIE = 1;    // Habilitar interrupcion del ADC 

// --------------- Oscilador --------------- 
    OSCCONbits.IRCF = 0b100 ; // establecerlo en 1 MHz
    OSCCONbits.SCS = 1; // utilizar oscilador interno
 
}

// --------------- Setup del ADC ---------------
void setupADC(void){
    // --------------- Configura el canal --------------- 
    ADCON0bits.CHS = 0b0000; // seleccionar AN0
    
            
    // --------------- Seleccion voltaje referencia --------------- 
    ADCON1bits.VCFG1 = 0; // Voltaje de referencia de 0V
    ADCON1bits.VCFG0 = 0; // Voltaje de referencia de 5V
            
    // --------------- Seleccion de reloj ---------------
    ADCON0bits.ADCS = 0b01; // Fosc/8
            
    // --------------- Habilitar interrupciones del ADC ---------------
    
            
    // --------------- Asignar 8 bits, justificado izquierda ---------------
    ADCON1bits.ADFM = 0;        
            
    //--------------- Iniciar el ADC ---------------
    ADCON0bits.ADON = 1;  
    __delay_ms(10);
}

// --------------- Definir funciones EEPROM ---------------
void EEPROMWRITE(uint8_t address, uint8_t data){
    EEADR = address;
    EEDAT = data;
    
    EECON1bits.EEPGD = 0; // Escribir en la memoria de datos
    EECON1bits.WREN = 1;  // habilitar escritura en eeprom 
    
    INTCONbits.GIE = 0;   // Deshabilita las interrupciones 
    

    //obligatorio
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1; // Habilitar escritura
    
    EECON1bits.WREN = 0; // Apagar la escritura
    
    INTCONbits.RBIF = 0; // limpiar bandera en el puerto b
    INTCONbits.GIE = 1; // habilitar interrupciones globales 
            
}

uint8_t EEPROMREAD(uint8_t address){
    EEADR = address ;
    EECON1bits.EEPGD = 0; // Seleccionar la memoria de datos
    EECON1bits.RD = 1;    // Habilitar lectura de datos
    return EEDAT;         // Return de lo que se encontraba en data
}
