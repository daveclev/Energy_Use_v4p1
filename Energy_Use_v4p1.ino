/*
Energy Use
v1.0 25ms timer2 routine w/ 500ms loop in main, .5second heartbeat, analog read using internal reference.
v2.0 math to do take an analog reading every 100msec and average them.  Reporting the average every 500msec
v3.0 math and serial out for instantaneous current draw.
v4.0 Adds 2.2" TFT display with Title and Instantaneous Current Draw: XX.XXX
Next Step: Build a running average of the analog input.  Build a elapsed time coutner to be used to calculate kW hours
 
 */
 
// Libraries
#include <MsTimer2.h>                                       //Easy access to Timer2 ISR from: http://arduino.cc/playground/Main/MsTimer2
#include <Adafruit_GFX.h>                                   //Adafruit Generic Graphics library
#include <Adafruit_HX8340B.h>                               //Adafruit 2.2" TFT LCD (Needs Adafruit_GFX.h)
#include <SPI.h>

// Pin definition
int sensorPin = A0;                                         // select the input pin for the potentiometer
#define HB 13                                               //Digital pin for Heart Beat
#define TFT_MOSI  11		// SDI                      //SPI
#define TFT_CLK   13		// SCL                      //SPI
#define TFT_CS    10		// CS                       //SPI chip select for 2.2" TFT LCD
#define TFT_RESET  9		// RESET                    //2.2" TFT LCD onboard driver chip reset used by Adafruit_H8340B.h library

// Color definitions for 2.2" TFT LCD
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

//Define objects
Adafruit_HX8340B display(TFT_RESET, TFT_CS);                //Confirm terminology: Object 'display' defined as Class 'Adafruit_HX8340B'  ????

// Global variables
int sensorValue = 0;                                        // variable to store the value coming from the sensor
unsigned char half_second_mscounter = 0;                    //Holder for a flag that will increment every time the millisecond_holder gets reset
unsigned char tenth_second_mscounter = 0;                  
bool half_second_flag = 0;
bool tenth_second_flag = 0;
bool heart_beat = 0;                                        //Heartbeat flag
uint8_t Heading_Count = 20;

float  sum_current = 0;                                     //Needs to be float or it result of calculation will be rounded to int
float  average_current = 0;                                 //Needs to be float or the resulting calcution is off by %80 at least
float  average_ADC = 0;                                     //
float  average_Watts = 0;
uint8_t  Voltage = 120;
uint8_t  average_counter = 0;


/*Timer2 Overlflow*******************************************
Any code placed in timer2_overflow() will be executed every at the frequency set in the the setup portion of the code*/
void timer2_overflow() {
  //Counters-------------------------------------------------
  half_second_mscounter++;                                   //increments the variable for the .5+ second loop                
  if (half_second_mscounter > 19){                           //Loop is true every .5 seconds
    half_second_mscounter = 0;
    half_second_flag = 1;
  }
  tenth_second_mscounter++;
    if (tenth_second_mscounter > 3){                           //Loop is true every 100 milli seconds
    tenth_second_mscounter = 0;
    tenth_second_flag = 1;
  }
  //Any other code can be placed here.

}
//***********************************************************


/***********************************************************/
/*                       SETUP                             */
/***********************************************************/
void setup() {
  Serial.begin(9600);
  Serial.println("\n\rEnergy Use v4.0");                    //\n moves down to the next line, \r moves to the begining of the line
  display.begin();                                          //Setup Display
  display.fillScreen(WHITE);
  display.setCursor(20, 5);                                 //x, y in pixels
  display.setTextColor(BLACK);  
  display.setTextSize(2);
  display.println("Energy v4.0");
  
  
  MsTimer2::set(25, timer2_overflow);                       //(30ms period, function call) function is located above void setup()
  MsTimer2::start();                                        //Starts Interrupt service routine
  
  analogReference(INTERNAL);  
  //Internal uses the 1.1VRef.  
  //External allows a voltage to be aplied to the AREF as 
  //the reference.  If this line isn't used or if default
 // is specified the 5V will  be used as a reference.
 
 //Pin setups
   pinMode(HB, OUTPUT);   
   

 
}

/***********************************************************/
/*                    Main Loop                            */
/***********************************************************/
void loop() {
  if (half_second_flag){                                    //Loop is true every .5 seconds, determined in timer2 overflow routine
    half_second_flag = 0;
    //Place any functions in here
    
    //Heart Beat
    if(heart_beat) digitalWrite(HB, HIGH); 
    else digitalWrite(HB, LOW);
    heart_beat = !heart_beat;

    Do_Math();
    
    Serial_Printing();   
    LCD_Printing();
  }
  if (tenth_second_flag){                                    //Loop is true every .1 seconds, determined in timer2 overflow routine
    tenth_second_flag = 0;
    sum_current += sensorValue;
    average_counter++;    
  }

  sensorValue = analogRead(sensorPin);                       // read the value from the sensor:
  if(sensorValue < 3) sensorValue = 0;                       // get rid of noise

 // Serial.println(sensorValue, DEC);
            
}

//---------------------------------------------------------//
/* Do Math                                                 */
//---------------------------------------------------------//
//Updates data
void Do_Math(){
  average_ADC = sum_current / average_counter;             //creates average of   
  average_counter = 0;                                     //Reset to zero
  sum_current = 0;                                         //Reset to zero
  
  average_current = 0.0458 * average_ADC + 0.0059;         //EXACT Current calculation from spreadsheet
  average_current = average_current - 0.0059;              //removes the offset from the ECXACT current calculation
  
  average_Watts = average_current * Voltage;
 
}

//---------------------------------------------------------//
/* LCD Printing                                            */
//---------------------------------------------------------//
//Takes care of the main LCD Display
void LCD_Printing(){
  uint8_t l_amps = 25;
  display.setCursor(0, l_amps);                                //x, y in pixels
  display.setTextColor(BLACK);  
  display.setTextSize(2);
  display.print("Amps: ");  
  
  
  display.fillRect(60, 25, 116, 14 , WHITE);                // x position, y position, x relative length right, y relative height down (14 exact coverage of size 2 numbers)
  display.setTextColor(RED);  
  display.setTextSize(2);
  display.println(average_current, 3);  
  
  display.fillRect(60, 50, 116, 14 , WHITE);                // x position, y position, x relative length right, y relative height down (14 exact coverage of size 2 numbers)
  display.setTextColor(RED);  
  display.setTextSize(2);
  display.print(average_Watts, 3);    
  
}
  
//---------------------------------------------------------//
/* Serial Printing                                         */
//---------------------------------------------------------//
//Takes care of the main serial print stream
void Serial_Printing(){
  bool Headings = 1;  
  bool HB_Debug = 1;
  bool Raw_AI   = 1;
  bool CS       = 1; 
  bool I_inst   = 1;
  
  if(HB_Debug){
    Serial.print(heart_beat);
    Serial.print(", ");
  }
  if(Raw_AI){    
    Serial.print(sensorValue, DEC); 
    Serial.print(", ");
  }
  if(CS){    
    Serial.print(average_ADC, 2); 
    Serial.print(", ");
  }
  if(I_inst){
    Serial.print(average_current, 3); 
    Serial.print(", ");
  }
  
  Serial.print("\n\r");
  
  if(Headings && (Heading_Count > 17)){ 
    Heading_Count = 0;
    Serial.print("\n\r");    
    if(HB_Debug){
      Serial.print("H");
      Serial.print(", ");
    }
    if(Raw_AI){    
      Serial.print("AI"); 
      Serial.print(", ");
    }
    if(CS){    
      Serial.print("Av AI"); 
      Serial.print(", ");
    }
    if(I_inst){
      Serial.print("I (A)"); 
      Serial.print(", ");
    }     
    
    Serial.print("\n\r");
  }
  Heading_Count++;


}
