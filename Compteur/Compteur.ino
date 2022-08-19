#include <SPI.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
const byte tachy_pin = 2;
const byte TM_pin = 12;
volatile uint32_t rev;
unsigned long measureTime = 0;

int RX = 4, TX = 5;
int TM_active = 0;
TinyGPSPlus gps;
SoftwareSerial gpssoft(RX, TX);

const int nblectures = 2;
int lectures[nblectures]; 
unsigned long moyenne = 0;
int index = 0;
unsigned long total;
unsigned long rpm = 0;
unsigned long kmh = 0;
unsigned long Dmils_compute = 0;
unsigned long Dmils_display = 0;
char kmh_buff[3];
char gps_buff[3];

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO/NANO:       A4(SDA), A5(SCL)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Callback fo tachy_pin interrupt
void count_turn(){
  rev++;
}

void setup(){
  // put your setup code here, to run once:
  Serial.begin(9600);
  gpssoft.begin(9600);
  
  pinMode(tachy_pin, INPUT_PULLUP);
  pinMode(TM_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(tachy_pin), count_turn, FALLING);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(1000); // Pause

  // Clear the buffer
  display.clearDisplay();

  if (!digitalRead(TM_pin))
  {
    display.setTextSize(2);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 0);     // Start at top-left corner
    display.print(F("TEST_MODE"));
    display.display();
    delay(2000); // Pause
    display.clearDisplay();
    TM_active = 1;
  }
  
}

void loop() {

  //Compute rpm and speed every 1000 ms
  if ( (millis() - Dmils_compute) >= 1000 ) {
    
     noInterrupts();
     total = 0;
     lectures[index] = rev * 60;

     for (int x=0; x<=nblectures - 1; x++){
        total = total + lectures[x];
     }

     moyenne = total / nblectures;
     rpm = moyenne;
     kmh = rpm * PI * 0.33 * 60 / 1000;
     rev = 0;
     
     index++;
     if (index >= nblectures){
        index = 0;
     }
     interrupts();

     //Serial.println(rpm);
     
     Dmils_compute = millis();
     
   }

   //Refresh display every 200 ms
   if ( (millis() - Dmils_display) >= 200 ) {
      if (millis() > 1000){
           //Effacer l ecran
          display.clearDisplay();


          display.setTextSize(3);      // Normal 1:1 pixel scale
          display.setTextColor(SSD1306_WHITE); // Draw white text
          display.setCursor(0, 10);     // Start at top-left corner
          display.cp437(true);         // Use full 256 char 'Code Page 437' font

          if (TM_active)
          {
             // Tr/min for test mode
            display.print(rpm);
            display.setTextSize(1); 
            display.println(F(" tr/min"));
          }
          else
          {
            //Jauge de vitesse
            display.fillRect(0,0 ,(kmh/2),3 ,SSD1306_WHITE);

            // affichage vitesse km/h
            sprintf(kmh_buff,"%03d",kmh);
            display.print(kmh_buff);
            display.setTextSize(1);      // Normal 1:1 pixel scale
            display.println(F(" Km/h"));
            displayspeed();
          }  
          
          display.display();
     }

     Dmils_display = millis();
   }

  //Read GPS module NMEA frames every loop if available
  if (gpssoft.available() > 0)
   {
      gps.encode(gpssoft.read());
   }

   
   
}

//Display GPS speed only if valid
void displayspeed()
{
  if (gps.speed.isValid())
  {
    display.setTextSize(2);
    display.setCursor(92, 15);
    sprintf(gps_buff,"%03d",round(gps.speed.kmph()));
    display.print(gps_buff);
    display.display();
    
  }
  else
  {
    display.setTextSize(2);
    display.setCursor(92, 15);
    display.print("---");
    display.display();
  }
}
