// Pre-processing directives
#include "main.h"

#include "Time_Delays.h"
#include "Clk_Config.h"
#include "LCD_Display.h"

#include <stdio.h>
#include <string.h>

#include "Small_7.h"
#include "Arial_9.h"
#include "Arial_12.h"
#include "Arial_24.h"

/*
A2
This program allows the user to read the temperature from the mbed application shield temperature sensor,
stores it in the Payload field of a packet of an experimental protocol, writes the packet to EEPROM and 
reads it back again. 
The packet has the following fields: MAC dest, MAC src, Length, Payload and FCS.
The following joystick presses perform the listed functions, after performing a function it flashes a success
message on the LCD.
Centre: Read temperature from sensor, store it in Payload field of the packet and calculate CRC value (Nucleo 
provides a CRC unit for calculating CRC values), then store it in the FCS field of the packet
Right: Write packet to EEPROM
Left: Read packet from EEPROM
Up: Display new field of the packet
Down: Display new field of the packet 

Furthermore, acknowledge polling is used for EEPROM's internal write operation to go between successive page write
cycles
*/


// Temperature Sensor I2C Address
#define TEMPADR 0x90

// EEPROM I2C Address
#define EEPROMADR 0xA0

// GPIO
void configure_gpio(void);

// Joystick Configuration
void joystick_configure(void);
uint32_t joystick_up(void); 
uint32_t joystick_down(void); 
uint32_t joystick_left(void);
uint32_t joystick_right(void);
uint32_t joystick_centre(void);

// I2C
void i2c_1_configure(void);

// Temperature
uint16_t read_temperature(void);

// Payload structure (46 byte field)
struct P {
	uint16_t sample; // 2 bytes
	unsigned char pl[44]; // 44 bytes
}; 

// Packet structure (the members of the structure are the fields of the packet)
struct Pack {
	unsigned char MAC_dest[6]; // 6 byte field
	unsigned char MAC_src[6]; // 6 byte field
	uint16_t length; //2 byte field
	struct P payload; // 46 byte field
	uint32_t FCS; //4 byte field
};

// EEPROM
void eeprom_write(struct Pack data);
struct Pack eeprom_read(void);

// CRC calculation function
uint32_t calculate_CRC(struct Pack pkt);
		
int main(void){
    // Init
    SystemClock_Config();// Configure the system clock to 84.0 MHz
	SysTick_Config_MCE2(us);	

	// Configure LCD
	Configure_LCD_Pins();
	Configure_SPI1();
	Activate_SPI1();
	Clear_Screen();
	Initialise_LCD_Controller();
	set_font((unsigned char*) Arial_12);
		
	// Configure GPIO
	configure_gpio();
	joystick_configure();
	
	// Configure I2C and set up the GPIO pins it uses
	i2c_1_configure(); 
	
	// Initializations and declarations
	char outputString[18]; //Buffer to store text in for LCD
	struct Pack packet; //Packet
	
	// Packet initializations
	for (int i=0; i<6; i++){
		packet.MAC_dest[i]= 0xaa; // MAC dest and MAC src initialization
		packet.MAC_src[i]= 0xbb;
	}
	
	packet.length= 0x2e;// payload length initialization
	
	packet.payload.sample= 0; //payload sample initialization
	
	for (int i=0; i<44; i++){
	    packet.payload.pl[i]=0;
	}
	
	packet.FCS= 0x00; // FCS initialization
	
	//Display MAC dest:
	put_string(0,0,"             ");
	put_string(0,15,"             ");
	put_string(0,0,"MAC dest:");
	
	for(int i=0; i<6; i++){
	    sprintf(outputString, "%x", packet.MAC_dest[i]); // Print to LCD
	    put_string(22*i,15,outputString);
	}
	
	int current=1; // Index for 'joystick up' and 'joystick down' (takes values from 1 to 6)
	//Main Loop
    while (1){
		if(joystick_centre()){			
			LL_mDelay(100000); // Delay for switch bounce
				
			packet.payload.sample = read_temperature(); // Reads temperature sensor
				
			// Each time temperature is read, CRC is calculated
			packet.FCS= calculate_CRC(packet);
			
			put_string(0,0,"             "); // Report successful temperature read
			put_string(0,0,"Sampled");
			put_string(0,15,"             ");
				
			LL_mDelay(500000);
			
			put_string(0,0,"             ");
			put_string(0,15,"             ");
			put_string (0,0, "Temp:");
			sprintf(outputString, "%f", packet.payload.sample*0.125f); // Print temperature to LCD
			put_string(0,15,outputString);
			current=4; // It is showing the payload sample so the index is set accordingly
		} 
			
		else if(joystick_right()){
			LL_mDelay(100000); // Delay for switch bounce
				
			eeprom_write(packet); // Write packet to EEPROM
			
			put_string(0,0,"             "); // Report successful write
			put_string(0,0,"Written");
			put_string(0,15,"             ");
				
			LL_mDelay(500000);
			
			put_string(0,0,"             ");
			put_string(0,15,"             ");
			put_string(0,0, "Temp:");
			sprintf(outputString, "%f", packet.payload.sample*0.125f); // Print temperature to LCD
			put_string(0,15,outputString);
			current=4; // It is showing the payload sample so the index is set accordingly
		} 

		else if(joystick_left()){
			LL_mDelay(100000); // Delay for switch bounce
				
			packet = eeprom_read(); // Read packet from EEPROM
			
			put_string(0,0,"             "); // Report success read
			put_string(0,0,"Retrieved");
			put_string(0,15,"             ");
				
			LL_mDelay(500000);
			
			put_string(0,0,"             ");
			put_string(0,15,"             ");
			put_string(0,0, "Temp:");
			sprintf(outputString, "%f", packet.payload.sample*0.125f); // Print transferred payload sample to LCD 
			put_string(0,15,outputString);
			current=4; // It is showing the payload sample so the index is set accordingly
		}
		
		else if(joystick_down()){
			LL_mDelay(100000); // Delay for switch bounce
			 
			// current=1 means the current field is MAC dest 
			if (current==1){
                put_string(0,0,"             ");
                put_string(0,15,"             ");
                put_string(0,0,"MAC src:");
                for(int m=0; m<6; m++){
                    sprintf(outputString, "%x", packet.MAC_src[m]); // Print MAC src to LCD
                    put_string(22*m,15,outputString);
                }
		    	current++;  // Current field is now MAC src
            }
			 
			// current=2 means the current field is MAC src
			else if (current==2){
                put_string(0,0,"             ");
                put_string(0,15,"             ");
                put_string(0,0,"Length:");
                sprintf(outputString, "%x", packet.length ); // Print length to LCD
                put_string(0,15,outputString);
			    current++; // Current field is now Length
            } 
			 
			// current=3 means the current field is Length
			else if (current==3){
                put_string(0,0,"             ");
                put_string(0,15,"             ");
                put_string(0,0,"Temp:");
                sprintf(outputString, "%f", packet.payload.sample*0.125f); // Print payload sample to LCD 
                put_string(0,15,outputString);
			    current++; // Current field is now Payload
            } 
			 
			// current=4 means the current field is Payload
			else if (current==4){
                put_string(0,0,"             ");
                put_string(0,15,"             ");
                put_string(0,0,"FCS:");
                sprintf(outputString, "%x", packet.FCS); // Print FCS to LCD 
                put_string(0,15,outputString);
                current++; // Current field is now FCS
			}
			 
			// current=5 means the current field is FCS
			else if (current==5){
			    uint32_t variable=0; // Variable that holds the newly calculated CRC
                // Recalculate CRC
                variable =calculate_CRC(packet);
                    
                if(variable==packet.FCS){
                    put_string(0,0,"             ");
                    put_string(0,15,"             ");
                    put_string(0,0,"FCS check OK:");
                    sprintf(outputString, "%x", variable); // Print CRC field to LCD 
                    put_string(0,15,outputString);
                }
                else if (variable!=packet.FCS){
                    put_string(0,0,"             ");
                    put_string(0,15,"             ");
                    put_string(0,0,"FCS ERROR:");
                    sprintf(outputString, "%x", variable); // Print CRC field to LCD 
                    put_string(0,15,outputString);
                }
                current++;
		    }
		     
		    // current=6 means the CRC field and error are displayed
			else if (current==6){
                uint32_t variable=0; // Variable that holds the newly calculated CRC
                // Recalculate CRC
                variable =calculate_CRC(packet);
                    
                if(variable==packet.FCS){
                    put_string(0,0,"             ");
                    put_string(0,15,"             ");
                    put_string(0,0,"FCS check OK:");
                    sprintf(outputString, "%x", variable); // Print CRC field to LCD 
                    put_string(0,15,outputString);
                }
                else if (variable!=packet.FCS){
                    put_string(0,0,"             ");
                    put_string(0,15,"             ");
                    put_string(0,0,"FCS ERROR:");
                    sprintf(outputString, "%x", variable); // Print CRC field to LCD 
                    put_string(0,15,outputString);
                }
		}
				 
	}
		 
		else if(joystick_up()){
			LL_mDelay(100000); // Delay for switch bounce
			 
			// current=1 means current field is MAC dest 
			if (current==1){
                put_string(0,0,"             ");
                put_string(0,15,"             ");
                put_string(0,0,"MAC dest:");
                for(int m=0; m<6; m++){
                    sprintf(outputString, "%x", packet.MAC_dest[m]); // Print MAC dest to LCD
                    put_string(22*m,15,outputString);
                }
			}
			 
			// current=2 means current field is MAC src
			else if (current==2){
                put_string(0,0,"             ");
                put_string(0,15,"             ");
                put_string(0,0,"MAC dest:");
                for(int m=0; m<6; m++){
                    sprintf(outputString, "%x", packet.MAC_dest[m]); // Print MAC dest to LCD
                    put_string(22*m,15,outputString);
                    }
                current--; // Current is now MAC dest
            }
			 
			// current=3 means current field is Length
			else if (current==3){
                put_string(0,0,"             ");
                put_string(0,15,"             ");
                put_string(0,0,"MAC src:");
                for(int m=0; m<6; m++){
                    sprintf(outputString, "%x", packet.MAC_src[m]); // Print MAC src to LCD
                    put_string(22*m,15,outputString);
                }
                current--; // Current field is now MAC src
            }
			 
			// current=4 means current field is Payload
			else if (current==4){
                put_string(0,0,"             ");
                put_string(0,15,"             ");
                put_string(0,0, "Length:");
                sprintf(outputString, "%x", packet.length ); // Print length to LCD
                put_string(0,15,outputString);
                current--; // Current field is now Length
            }
			 
			// current=5 means current field is FCS
			else if (current==5){
			put_string(0,0,"             ");
			put_string(0,15,"             ");
			put_string(0,0, "Temp:");
			sprintf(outputString, "%f", packet.payload.sample*0.125f); // Print temperature to LCD
			put_string(0,15,outputString);
			current--; // Current field is now Payload
            } 
			 
			// current=6 means the CRC field and error are displayed
			else if (current==6){
			put_string(0,0,"             ");
			put_string(0,15,"             ");
			put_string(0,0, "FCS:");
			sprintf(outputString, "%x", packet.FCS ); // Print FCS to LCD
			put_string(0,15,outputString);
			current--; // Current field is now FCS
            }
			 
		}
	}
}

void configure_gpio(void){
	// Configures the GPIO pins by enabling the peripherial clocks on the ports used by the shield
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB); 
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC); 
	// Enabling the clock to the CRC peripheral
	LL_AHB1_GRP1_EnableClock (LL_AHB1_GRP1_PERIPH_CRC);

}	

void i2c_1_configure(void){
	// Enable I2C1 Clock
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
  
	// Configure SCL as: Alternate function, High Speed, Open Drain, Pull Up
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_8, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_8, LL_GPIO_AF_4);
    LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_8, LL_GPIO_OUTPUT_OPENDRAIN);
	
    // Configure SDA as: Alternate, High Speed, Open Drain, Pull Up
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_9, LL_GPIO_AF_4);
    LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_9, LL_GPIO_OUTPUT_OPENDRAIN);
  
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
  
    LL_I2C_Disable(I2C1);
    LL_I2C_SetMode(I2C1, LL_I2C_MODE_I2C);
    LL_I2C_ConfigSpeed(I2C1, 84000000, 400000, LL_I2C_DUTYCYCLE_2);
    LL_I2C_Enable(I2C1);
}	

void joystick_configure(void){
	//This function configures all the GPIO pins that are connected to the joystick on the mbed shield
	//(not all joystick pins are used)
	
	LL_GPIO_SetPinMode (GPIOA, LL_GPIO_PIN_4, LL_GPIO_MODE_INPUT); 		//set PA4 as Input
	LL_GPIO_SetPinPull (GPIOA, LL_GPIO_PIN_4, LL_GPIO_PULL_NO); 		//set PA4 as NO pull
	
	LL_GPIO_SetPinMode (GPIOB, LL_GPIO_PIN_0, LL_GPIO_MODE_INPUT); 		//set PB0 as Input
	LL_GPIO_SetPinPull (GPIOB, LL_GPIO_PIN_0, LL_GPIO_PULL_NO); 		//set PB0 as NO pull
	
	LL_GPIO_SetPinMode (GPIOC, LL_GPIO_PIN_1, LL_GPIO_MODE_INPUT); 		//set PC1 as Input
	LL_GPIO_SetPinPull (GPIOC, LL_GPIO_PIN_1, LL_GPIO_PULL_NO); 		//set PC1 as NO pull
	
	LL_GPIO_SetPinMode (GPIOC, LL_GPIO_PIN_0, LL_GPIO_MODE_INPUT); 		//set PC0 as Input
	LL_GPIO_SetPinPull (GPIOC, LL_GPIO_PIN_0, LL_GPIO_PULL_NO); 		//set PC0 as NO pull
	
	LL_GPIO_SetPinMode (GPIOB, LL_GPIO_PIN_5, LL_GPIO_MODE_INPUT); 		//set PB5 as Input
	LL_GPIO_SetPinPull (GPIOB, LL_GPIO_PIN_5, LL_GPIO_PULL_NO); 		//set PB5 as NO pull
}

uint32_t joystick_up(void) {
	//Returns 1 if the joystick is pressed up, 0 otherwise
	return (LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_4));
}

uint32_t joystick_down(void) {
	//Returns 1 if the joystick is pressed down, 0 otherwise
	return (LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_0));
}

uint32_t joystick_left(void) {
	//Returns 1 if the joystick is pressed left, 0 otherwise
	return (LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_1));
}

uint32_t joystick_right(void) {
	//Returns 1 if the joystick is pressed right, 0 otherwise
	return (LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_0));
}
uint32_t joystick_centre(void) {
	//Returns 1 if the joystick is pressed in the centre, 0 otherwise
	return (LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_5));
}

uint32_t calculate_CRC(struct Pack pkt) {
	//Calculate CRC value
	
	//Reset the CRC unit before use
	LL_CRC_ResetCRCCalculationUnit(CRC);
	
	//Packet is processed in 'units' of four bytes
	for(int i=0;i<4;i++){
	    LL_CRC_FeedData32(CRC, pkt.MAC_dest[i]);
	}
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	for(int i=4;i<6;i++){
	    LL_CRC_FeedData32(CRC, pkt.MAC_dest[i]);
	}
	for(int i=0;i<2;i++){
	    LL_CRC_FeedData32(CRC, pkt.MAC_src[i]);
	}
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	for(int i=2;i<6;i++){
	    LL_CRC_FeedData32(CRC, pkt.MAC_src[i]);
	}
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	LL_CRC_FeedData32(CRC, pkt.length);
	LL_CRC_FeedData32(CRC, pkt.payload.sample);
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	for(int i=0; i<4; i++){
	    LL_CRC_FeedData32(CRC, pkt.payload.pl[i]);
	}
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	for(int i=4; i<8; i++){
	    LL_CRC_FeedData32(CRC, pkt.payload.pl[i]);
	}
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	for(int i=8; i<12; i++){
	    LL_CRC_FeedData32(CRC, pkt.payload.pl[i]);
	}
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	for(int i=12; i<16; i++){
	    LL_CRC_FeedData32(CRC, pkt.payload.pl[i]);
	}
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	for(int i=16; i<20; i++){
	    LL_CRC_FeedData32(CRC, pkt.payload.pl[i]);
	}
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	for(int i=20; i<24; i++){
	    LL_CRC_FeedData32(CRC, pkt.payload.pl[i]);
	}
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	for(int i=24; i<28; i++){
	    LL_CRC_FeedData32(CRC, pkt.payload.pl[i]);
	}
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	for(int i=28; i<32; i++){
	    LL_CRC_FeedData32(CRC, pkt.payload.pl[i]);
	}
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	for(int i=32; i<36; i++){
	    LL_CRC_FeedData32(CRC, pkt.payload.pl[i]);
	}
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	for(int i=36; i<40; i++){
	    LL_CRC_FeedData32(CRC, pkt.payload.pl[i]);
	}
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	for(int i=40; i<44; i++){
	    LL_CRC_FeedData32(CRC, pkt.payload.pl[i]);
	}
	delay_us(100); // Short delay that allows the CRC to be computed
	
	
	return LL_CRC_ReadData32(CRC);
}
void eeprom_write(struct Pack data){
	//Writes packet to the EEPROM
	int i; // Index for loops
	
	LL_I2C_GenerateStartCondition(I2C1); //START
    while(!LL_I2C_IsActiveFlag_SB(I2C1));
	
    LL_I2C_TransmitData8(I2C1, EEPROMADR); //CONTROL BYTE (ADDRESS + WRITE)
    while(!LL_I2C_IsActiveFlag_ADDR(I2C1));
    LL_I2C_ClearFlag_ADDR(I2C1);

    LL_I2C_TransmitData8(I2C1, 0x00); //ADDRESS HIGH BYTE
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));

    LL_I2C_TransmitData8(I2C1, 0x00); //ADDRESS LOW BYTE
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));
				
	//Writing MAC Destination Address
	for(i=0; i<6; i++){
        LL_I2C_TransmitData8(I2C1, (unsigned char)(data.MAC_dest[i]));  
        while(!LL_I2C_IsActiveFlag_TXE(I2C1));
	}
				
	//Writing MAC Source Address
	for(i=0; i<6; i++){
        LL_I2C_TransmitData8(I2C1, (unsigned char)(data.MAC_src[i])); 
        while(!LL_I2C_IsActiveFlag_TXE(I2C1));
	}
				
	//Writing Length
    LL_I2C_TransmitData8(I2C1, (unsigned char)(data.length >> 8)); //LENGTH HIGH BYTE
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));

	LL_I2C_TransmitData8(I2C1, (unsigned char)(data.length & 0x00FF)); //LENGTH LOW BYTE
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));
				
	//Writing Payload sample
	LL_I2C_TransmitData8(I2C1, (unsigned char)(data.payload.sample >> 8)); //TEMPERATURE HIGH BYTE
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));

	LL_I2C_TransmitData8(I2C1, (unsigned char)(data.payload.sample & 0x00FF)); //TEMPERATURE LOW BYTE
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));
				
	//Writing pl of Payload until cache is full
	for(i=0; i<16; i++){
        LL_I2C_TransmitData8(I2C1, (unsigned char)(data.payload.pl[i])); 
        while(!LL_I2C_IsActiveFlag_TXE(I2C1));
	}
				
	LL_I2C_GenerateStopCondition(I2C1); //STOP
				
	//ACKNOWLEDGE POLLING
	do{
		LL_I2C_ClearFlag_AF (I2C1); //clear AF flag
		LL_I2C_GenerateStartCondition(I2C1); //START
        while(!LL_I2C_IsActiveFlag_SB(I2C1)); //wait until complete
	
        LL_I2C_TransmitData8(I2C1, EEPROMADR); //CONTROL BYTE (ADDRESS + WRITE)
					
		delay_us(5000); //wait for small delay for EEPROM to respond
	}
	while(!LL_I2C_IsActiveFlag_AF (I2C1));
				
	LL_I2C_GenerateStartCondition(I2C1); //START
    while(!LL_I2C_IsActiveFlag_SB(I2C1));
	
    LL_I2C_TransmitData8(I2C1, EEPROMADR); //CONTROL BYTE (ADDRESS + WRITE)
    while(!LL_I2C_IsActiveFlag_ADDR(I2C1));
    LL_I2C_ClearFlag_ADDR(I2C1);

    LL_I2C_TransmitData8(I2C1, 0x00); //ADDRESS HIGH BYTE
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));

    LL_I2C_TransmitData8(I2C1, 0x00+32); //ADDRESS LOW BYTE
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));
				
	//Writing rest of pl of Payload 
	for(int i=16; i<44; i++){
        LL_I2C_TransmitData8(I2C1, (unsigned char)(data.payload.pl[i])); 
        while(!LL_I2C_IsActiveFlag_TXE(I2C1));
	}
				
	LL_I2C_TransmitData8(I2C1, (unsigned char)(data.FCS >> 24)); //FIRST FCS BYTE
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));
				
	LL_I2C_TransmitData8(I2C1, (unsigned char)(data.FCS >> 16)); //SECOND FCS BYTE
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));
				
	LL_I2C_TransmitData8(I2C1, (unsigned char)(data.FCS>> 8)); //THIRD FCS BYTE
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));
			
	LL_I2C_TransmitData8(I2C1, (unsigned char)(data.FCS)); //FOURTH FCS BYTE
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));
				
    LL_I2C_GenerateStopCondition(I2C1); //STOP
}

uint16_t read_temperature(void){
	//Reads the 11 bit temperature value from the 2 byte temperature register
	
	uint16_t temperature = 0;
  
	LL_I2C_GenerateStartCondition(I2C1); //START
    while(!LL_I2C_IsActiveFlag_SB(I2C1));

	LL_I2C_TransmitData8(I2C1, TEMPADR); //CONTROL BYTE (ADDRESS + READ)
    while(!LL_I2C_IsActiveFlag_ADDR(I2C1));
    LL_I2C_ClearFlag_ADDR(I2C1);

    LL_I2C_TransmitData8(I2C1, 0x00); //Set pointer register to temperature register
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));

	LL_I2C_GenerateStartCondition(I2C1); //RE-START
    while(!LL_I2C_IsActiveFlag_SB(I2C1));

	LL_I2C_TransmitData8(I2C1, TEMPADR+1); //ADDRESS + READ
	while(!LL_I2C_IsActiveFlag_ADDR(I2C1));
    LL_I2C_ClearFlag_ADDR(I2C1);

    LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK); //ACK INCOMING DATA
    while(!LL_I2C_IsActiveFlag_RXNE(I2C1));
    temperature = LL_I2C_ReceiveData8(I2C1);  //TEMPERATURE HIGH BYTE
	temperature = temperature << 8;

	LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_NACK); //NACK INCOMING DATA
	while(!LL_I2C_IsActiveFlag_RXNE(I2C1));
    temperature += LL_I2C_ReceiveData8(I2C1);  //TEMPERATURE LOW BYTE

    LL_I2C_GenerateStopCondition(I2C1);       //STOP

	return temperature >> 5; //Bit shift temperature right, since it's stored in the upper part of the 16 bits, originally.
}

struct Pack eeprom_read(void){
	// Reads two bytes from the EEPROM
	struct Pack data; // Received packet
	int i; // Index for loops
	uint32_t var; // Variable used for FCS reading
	
	LL_I2C_GenerateStartCondition(I2C1); //START
    while(!LL_I2C_IsActiveFlag_SB(I2C1));

    LL_I2C_TransmitData8(I2C1, EEPROMADR); //CONTROL BYTE (ADDRESS + WRITE)
    while(!LL_I2C_IsActiveFlag_ADDR(I2C1));
    LL_I2C_ClearFlag_ADDR(I2C1);

    LL_I2C_TransmitData8(I2C1, 0x00); //ADDRESS HIGH BYTE
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));

	LL_I2C_TransmitData8(I2C1, 0x00); //ADDRESS LOW BYTE
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));

	LL_I2C_GenerateStartCondition(I2C1); //RE-START
    while(!LL_I2C_IsActiveFlag_SB(I2C1));

    LL_I2C_TransmitData8(I2C1, EEPROMADR+1); //ADDRESS + READ
    while(!LL_I2C_IsActiveFlag_ADDR(I2C1));
    LL_I2C_ClearFlag_ADDR(I2C1);
	LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK); //ACK INCOMING DATA
	
	// Reading MAC Destination Address
	for(int i=0; i<6; i++){
		while(!LL_I2C_IsActiveFlag_RXNE(I2C1));
        data.MAC_dest[i] = LL_I2C_ReceiveData8(I2C1); 
		LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK); //ACK INCOMING DATA
	}
	
	// Reading MAC Source Address
	for(int i=0; i<6; i++){
        while(!LL_I2C_IsActiveFlag_RXNE(I2C1));
        data.MAC_src[i] = LL_I2C_ReceiveData8(I2C1); 
		LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK); //ACK INCOMING DATA
	}
	
	// Reading Length
    while(!LL_I2C_IsActiveFlag_RXNE(I2C1));
	data.length = LL_I2C_ReceiveData8(I2C1); //LENGTH HIGH BYTE
	data.length += data.length << 8;
	LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK); //ACK INCOMING DATA

	while(!LL_I2C_IsActiveFlag_RXNE(I2C1));
	data.length += LL_I2C_ReceiveData8(I2C1);  //LENGTH LOW BYTE
	LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK); //ACK INCOMING DATA

	// Reading temperature sample
	while(!LL_I2C_IsActiveFlag_RXNE(I2C1));
    data.payload.sample = LL_I2C_ReceiveData8(I2C1); //TEMPERATURE HIGH BYTE
	data.payload.sample = data.payload.sample << 8;
	LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK); //ACK INCOMING DATA

	while(!LL_I2C_IsActiveFlag_RXNE(I2C1));
	data.payload.sample += LL_I2C_ReceiveData8(I2C1);  //TEMPERATURE LOW BYTE
	LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK); //ACK INCOMING DATA
	
	// Reading pl of the Payload (the zeros)
	for(i=0; i<44; i++){
        while(!LL_I2C_IsActiveFlag_RXNE(I2C1));
        data.payload.pl[i] = LL_I2C_ReceiveData8(I2C1); 
	    LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK); //ACK INCOMING DATA
	}
	// Reading FCS value
	while(!LL_I2C_IsActiveFlag_RXNE(I2C1));
	data.FCS = LL_I2C_ReceiveData8(I2C1); //FCS FIRST BYTE
	data.FCS = data.FCS<< 24;
	LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK); //ACK INCOMING DATA
	
	while(!LL_I2C_IsActiveFlag_RXNE(I2C1));
    var = LL_I2C_ReceiveData8(I2C1); //FCS SECOND BYTE
	data.FCS += var<< 16;
	LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK); //ACK INCOMING DATA
	
	while(!LL_I2C_IsActiveFlag_RXNE(I2C1));
    var = LL_I2C_ReceiveData8(I2C1); //FCS THIRD BYTE
	data.FCS += var<< 8;
	LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_NACK); //NACK INCOMING DATA
	
	while(!LL_I2C_IsActiveFlag_RXNE(I2C1));
	data.FCS += LL_I2C_ReceiveData8(I2C1);  //FCS FOURTH BYTE
	
	LL_I2C_GenerateStopCondition(I2C1); //STOP
	return data;
}
