#include <avr/io.h> /*this include is unnecessary as the library is automatically included by the IDE; however, it is important to note that all register defines
                      used in this program can be found in this header*/ 


#define cutoffTemp 21 //sets the temperature (in degrees celcius) at which the fan will be powered on should this number be met or exceeded 

uint8_t H = 72; /*ASCII numeric representation for the letter "H" (for "High") meant to communicate "High Temperature" or "High Power"
                  that will be displayed on the serial monitor if the cutoffTemp is met or exceeded. 8 bit unsigned integer is required by the USART
                  serial register*/
uint8_t L = 76; /*ASCII numeric represetation for the letter "L" (for "Low") meant to communicate "Low Temperature" or "Low Power".
                Please reference above line for significance of uint8_t*/
uint8_t bits[5];  /*this array is used as a buffer to store binary information received by the DHT unit. The data, which includes temperature and humidity information,
                  is transmitted in a total number of 40 bits. Therefore, 5 8 bit integers are needed in this buffer*/
uint8_t temperature = 0; /*variable will be used to store the binary information from the "high temperature" bit received from the DHT 
                          therefore making it necessary to assign it as an unsigned 8-bit integer*/ 

                         

void setup() /*function call necessary to run the code using the Arduino's IDE/compiler. Typically, the code in this function would be placed in the "int main()" function of a
              standard C/C++ IDE/compiler code*/
{
  

_SFR_IO8(0x04) = 0b00000010; /*The macro function _SFR_I08() takes the Data Direction Register of Port B (DDRB) address (0x04) as an argument and sets the regirster as an output by 
                                setting bit "1" to the value "1"  via the bitmask 0b00000010.  This allows pin 9 on the Arduino Uno to be used to toggle the power sent to the fan's DC motor.
                               NOTE: further information regarding DDRB can be found in section 14.4.2 on pg. 91 of the Atmel AVR Datasheet*/

UCSR0B = (1<<TXEN0); /*this line enables the USART Transmitter thus allowing communication between the Arduino serial monitor and the Atmega328P by setting "bit 3" (TXEN0 - "Transmitter Enable" bit) of the 
                       USART Control Status Register B (UCSR0B) to the value "1" using bit shift left on the value of TXEN0. (pg.193 of the Atmel AVR Datasheet)*/
UBRR0L = 103; /*sets serial communication baud rate (the number of bits transmitted per second) to 9600 (high enough for the requirments of this program). 
              Because the Arduino Uno uses a 16 Mhz clockspeed, the USART Baud Rate Register (UBRR0) needs to be set to a value of 103 to achieve a Baud Rate of 9600 
              according to the table on pg. 190. Since 103 has a binary value of 01100111, it is contained within 8 bits which requires the setting of the UBRR0L ("L" for lower)
              register according to section 20.11.5 on pg. 195 set the Baud Rate*/
              
              
               


}
void loop() //second function call necessary to run the code through Arduino's compiler. This is essentially a While loop take the single argument 1 (e.g. While(1) )
{
   

  volatile uint8_t *PIR = portInputRegister(0x04); //pointer that dereferences the address of the Input Register for Port B; this value will be ANDed and ORed against a mask when recieving DHT data
  uint8_t index = 0; //counter integer used to increment the bits buffer to store data
  uint8_t mask = 0b10000000; //bit mask used to bitwise AND/OR incoming DHT binary data 

    for (uint8_t i = 0; i < 5; i++) bits[i] = 0; //initializes the bits buffer to ensure an empty set each time data is received 

    _SFR_IO8(0x0A) = 0b10000000; /*The macro function _SFR_I08() takes the Data Direction Register of Port D (DDRD) address (0x0A) as an argument and sets the register as an output by 
                                setting bit "1" to the value "1"  via the bitmask 0b10000000.  This sends a voltage from pin 7 to the DHT and starts the signal chain 
                               NOTE: information regarding DDRD is outlined in section 14.4.9 on pg. 92 of the Atmel AVR Datasheet and signal chain information
                               is found on the Data Timing Diagram on pg. 5 of the DHT data sheet*/
    
    _SFR_IO8(0x0B) &= 0b00000000; /*The macro function _SFR_I08() takes the address (0x0B) of the Port D Data Register as an argument and sets the output to "low" by
                                ANDing its value against the bitmask 0b00000000 which sets the any "activated" bit in the register to 0. 
                               NOTE: information regarding PORTD is found in section 14.4.8 on pg. 92 of the Atmel AVR Datasheet */
    
    delay(18); //the DHT Microprocessor requires at least 18 milliseconds with low output signal (ref. pg. 6 on DHT Data Sheet)
    
    _SFR_IO8(0x0B) |= 0b10000000; /*The macro function _SFR_I08() takes the address (0x0B) of the Port D Data Register as an argument and sets the output to "high" by
                                ORing its value against the bitmask 0b10000000 (which sets bit 7 to high) allowing the 5v signal to be sent to the DHT 
                               NOTE: information regarding PORTD is found in section 14.4.8 on pg. 92 of the Atmel AVR Datasheet and signal chain information
                               is found on the Data Timing Diagram on pg. 5 of the DHT data sheet*/
    
    delayMicroseconds(40);
    
    _SFR_IO8(0x0A) = 0b00000000; /*The macro function _SFR_I08() takes the Data Direction Register of Port D (DDRD) address (0x0A) as an argument and sets the register as an input by 
                                setting bit "7" to the value "0"  via the bitmask 0b00000000.  This activates the Atmel's internal pull-up resistor via pin 7 so data can be received.
                               NOTE: information regarding DDRD is outlined in section 14.4.9 on pg. 92 of the Atmel AVR Datasheet and signal chain information
                               is found on the Data Timing Diagram on pg. 5 of the DHT data sheet*/



    uint16_t loopCntLOW = (F_CPU/40000); //amount of time allowed for the while loop to execute based on the CPU speed 
    while ((*PIR & 0b10000000) == 0 )  //Halts the program until the input register receives high voltage from the DHT 
    {
        if (--loopCntLOW == 0) return -2; //returns an error if the loop runs too long 
    }

    uint16_t loopCntHIGH = (F_CPU/40000); 
    while ((*PIR & 0b10000000) != 0 )  //Halts the program until the input register recieves low voltage from the DHT 
    {
        if (--loopCntHIGH == 0) return -2;
    }
    
    for (uint8_t i = 40; i != 0; i--) /*Loop counts down from 40 in order to read 5 sets of bytes one bit at a time */ 
    {
        loopCntLOW = (F_CPU/40000);
        while ((*PIR & 0b10000000) == 0 ) //Halts the program until the input register receives high voltage from the DHT 
        {
            if (--loopCntLOW == 0) return -2; ////returns an error if the loop runs too long 
        }

        uint32_t t = micros(); //the micros function tests how long (in microseconds) the system has been running the program.  Declaring "t" here sets up a "checkpoint" to be reference at a later time

        loopCntHIGH = (F_CPU/40000);
        while ((*PIR & 0b10000000) != 0 ) /*Using the micros() function times how long this loop receives voltage which in turn determines whether the bit being sent 
                                            is a "1" or a "0" (ref diagram on pg. 7 of the DHT datasheet*/
                                            
        {
            if (--loopCntHIGH == 0) return -2; //returns an error if the loop runs too long 
        }

        if ((micros() - t) > 40) /*if the Input Register was in a state of high voltage for longer than 40 seconds, it is determined that the data being sent was a "1" */
        { 
            bits[index] |= mask; //the "1" is set into bits[] according to the position of the mask (i.e. the first bit of the humidity reading)
        }
        mask >>= 1; //shifts the bit in mask one place to the right (i.e. 10000000 ==> 01000000) and returns to the start of the for loop
        if (mask == 0)   // once the bit has been shifted completely, the index is incremented so that data can be written to the next byte
        {
            mask = 0b10000000; //bit mask is reset and the index is incremented
            index++; //increment the index
        }
    }
    

    temperature = bits[2]; /* because the third element of the bits buffer is reserved for the temperature reading (according to page 5 on the DHT data sheet), 
                              temperature is set to the value of bits[2] */
                          

  delay(500); //stops the program for 500 milliseconds to ensure enough time between enabling the USART Transmitter 


//------------------------------------------------------------------
//                               DC MOTOR SET

  

  if (temperature >= cutoffTemp) //if the value of bits[2] matches or exceeds the cutoff temperature, the fan is activated. Otherwise it maintains low power output
  {    
     _SFR_IO8(0x05) |= B00000010; /*The macro function _SFR_I08() takes the address (0x05) of the Port B Data Register as an argument and sets the output to "high" by
                                ORing its value against the bitmask 0b00000010 (which sets bit 1 to high) allowing the 3.3v signal to be sent through the circuit via pin 8 powering on the fan
                               NOTE: information regarding PORTB is found in section 14.4.2 on pg. 92 of the Atmel AVR Datasheet */
    UDR0 = H; /*the Arduino IDE serial monitor will read 1 byte from the USART DATA Register (UDR0) and convert the number according to the ASCII table. Setting this 
                to 'H' or 72 writes the 8-bit value 01001000 to both the Transmit Data Buffer Register and the Receive Data Buffer Register. The serial monitor reads
                the value from the receive Data Buffer Register and converts it to the ASCII character
                NOTE: information regarding the UDR is found in section 20.11.1 on pg. 191 of the Ateml AVR Datasheet */
    
    
  }
  else
  {
    _SFR_IO8(0x05)  &= B00000000; /*The macro function _SFR_I08() takes the address (0x05) of the Port B Data Register as an argument and sets the output to "high" by
                                ORing its value against the bitmask 0b00000010 (which sets bit 1 to high) allowing the 3.3v signal to be sent through the circuit via pin 8 powering on the fan
                               NOTE: information regarding PORTB is found in section 14.4.2 on pg. 92 of the Atmel AVR Datasheet */
    UDR0 = L; /*see the above lines "UDR0 = H" for explanation */ 

  }


 delay(1000); //wait 1 second before restarting the loop to allow ample start up time for the DC motor 
  
  
};











