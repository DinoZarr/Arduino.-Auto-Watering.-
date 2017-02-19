

// ----------------------------------------------------------------------
// PINS
//
// 38,39,40,41           display        Standard Arduino Mega/Due shield
// 6, 5, 4, 3, 2         touchscreen    Standard Arduino Mega/Due shield
//  8                    beep
//  9                    резерв
// 10                    DHT 11         желтый
// 11                    rele           оранжевый
// 8 analog              датчик почвы                        
// 13                    DS1820         белый     
// 20                    часы DS1307    SDA             
// 21                    часы DS1307    SCL  
// ----------------------------------------------------------------------


// Initialize  DHT 11
// ------------------
#include "DHT.h"
#define DHTPIN 10
#define DHTTYPE DHT11
DHT   dht(DHTPIN, DHTTYPE);
byte oldHumidity = 0;
byte oldTemperature = 0;


// Initialize  DS18B20
// ------------------
#include <OneWire.h>
OneWire  ds(13);
byte addr[8] =  {0x28, 0xFF, 0x3E, 0x4E, 0x15, 0x15, 0x02, 0x4B};    //      в метале градусник
float oldTemperatureOut = 0;


// Initialize  Clock DS1307
// ------------------------
#include "Wire.h"
#define DS1307_I2C_ADDRESS 0x68
byte   oldMinute     = 0;
byte   oldHour       = 0;
byte   oldDayOfWeek  = 0;
byte   oldDayOfMonth = 0;
byte   oldMonth      = 0;
byte   oldYear       = 0;


// Initialize display
// ------------------
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino Uno/2009 Shield            : <display model>,19,18,17,16
// Standard Arduino Mega/Due shield            : <display model>,38,39,40,41
// CTE TFT LCD/SD Shield for Arduino Due       : <display model>,25,26,27,28
// Teensy 3.x TFT Test Board                   : <display model>,23,22, 3, 4
// ElecHouse TFT LCD/SD Shield for Arduino Due : <display model>,22,23,31,33
//
// Remember to change the model parameter to suit your display module!
#include <UTFT.h>
UTFT    myGLCD(ITDB32S, 38, 39, 40, 41);


// Initialize touchscreen
// ----------------------
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino Uno/2009 Shield            : 15,10,14, 9, 8
// Standard Arduino Mega/Due shield            :  6, 5, 4, 3, 2
// CTE TFT LCD/SD Shield for Arduino Due       :  6, 5, 4, 3, 2
// Teensy 3.x TFT Test Board                   : 26,31,27,28,29
// ElecHouse TFT LCD/SD Shield for Arduino Due : 25,26,27,29,30
//
#include <UTouch.h>
UTouch  myTouch( 6, 5, 4, 3, 2);


// Initialize EEPROM
#include <EEPROM.h>


// Initialize time interrupt
//#include <FlexiTimer2.h>



// Declare which fonts we will be using
extern uint8_t BigRusFont[];
extern uint8_t SmallRusFont[];
extern uint8_t SevenSegRusNumFont[];


// Declare which pictures we will be using
extern unsigned int gradusnik[1350];
extern unsigned int vlag[0x325];
extern unsigned int lleika[0x9C4];


//-----------------------------
// Declare PIN to Sensor Ground
byte pinGround = 8;
byte oldGroundVal = 101;


// Declare PIN to Rele
byte pinRele   = 11;
//-----------------------------



// Declare PIN to Beep
byte pinBeep   = 8;
//-----------------------------



// Global
// --------------------------------------------------------------------------------------------------------
int     x, y;
boolean poliff           = false;       // включается на передней панели
boolean poliffTime       = false;       // включается в указанный промежуток времени
boolean poliffPochva     = false;       // включается от датчика влажности почвы          true - сухо
boolean modePoliv        = true;        // ежедневно/ через день true - ежедневно
boolean modePoliffFlag   = true;        // работает от modePoliv 
boolean poliffSensorTime = true;        // true - отключает полив по времяни, false - по датчику почвы
boolean noRain           = false;       // на начало полива почва мокрая значит дождь
boolean pinReleFlag      = false;       // реле ВКЛ / ВЫКЛ


// положение часов на экране
int clockY  =  96;

// остаток времени полива
int     oldLostTime = 1500; 
boolean firstTimeEnter = true;


// моргалка часов
//#if defined(ARDUINO) && ARDUINO >= 100
//   boolean blinkOfClock = true;
//#endif



byte startPolifHour =    EEPROM.read(0);
byte startPolifMinute =  EEPROM.read(1);
byte finishPolifHour =   EEPROM.read(2);
byte finishPolifMinute = EEPROM.read(3);




void setup() {

  dht.begin();                               // инициализация датчика DHT 11
  Wire.begin();                              // инициализация шины I2C для часов DS1307

  pinMode( pinRele , OUTPUT);                // реле включения питания
  digitalWrite( pinRele , HIGH );
  pinReleFlag  = false;

  pinMode( pinGround , INPUT);               // датчик влажности почвы

  myGLCD.InitLCD();
  myGLCD.setBackColor(VGA_TRANSPARENT);
  myGLCD.setBrightness(255);

  drawFirstScreen();

  myTouch.InitTouch();
  myTouch.setPrecision(PREC_HI);

  pinMode( pinBeep, OUTPUT );                // beep

}


void loop() {

  //  вывод температуру и влажность с внутреннего градусника
  drawTempHumidity();

  //  вывод инфу с часов
  drawTime(); 
  
  //  моргаем двоеточием
  myGLCD.setFont(SevenSegRusNumFont);
  myGLCD.print(":", 84, clockY, 0);

  //  вывод температуру с внешнего градусника
  drawTempOutside();

  //  подтираем двоеточие знак ";" - как пробел
  myGLCD.setColor(VGA_WHITE);
  myGLCD.print(";", 84, clockY, 0);
  myGLCD.setColor(100, 100, 100);

  // вывод датчика почвы
  drawSensorGround();


  //  ожидание нажатия клавиши
  if ( !poliffTime ) {
    if (myTouch.dataAvailable())
    {
      myTouch.read();
      x = myTouch.getX();
      y = myTouch.getY();

      if ( ( y <= 150 ) && ( y >= 98 ) )  {      // область часов
        drawBeep();
       // FlexiTimer2::stop();
        SetTimeScreen();
        oldHumidity = 0; oldTemperature = 0;
        oldTemperatureOut = 0;
        oldMinute = 255; oldHour = 255; oldDayOfWeek = 0; oldDayOfMonth = 0; oldMonth = 0; oldYear = 0; oldGroundVal = 101;
        drawFirstScreen();
      //  FlexiTimer2::start();
        x = 0 ;
        y = 0 ;
      }
      if ( y >= 160 )  {             // нижний ряд
        if ( x <= 140 )  {         //  лейка OFF/ON
          poliff = ! poliff ;
          drawBeep();
          myGLCD.setFont( BigRusFont);
          clearStr ("\xB2""\xB2""\xB2", 80, 202);
          if ( poliff ) {
            myGLCD.setColor(100, 100, 100);
  //          myGLCD.setFont ( SmallRusRus );
            myGLCD.print("on", 96, 202, 0);
 //           myGLCD.print("АБВГ", 96, 202, 0);
          }
          else  {
            myGLCD.setColor(220, 220, 220);
            myGLCD.print("off", 80, 202, 0);
  //         myGLCD.setFont ( SmallRusRus );
   //         myGLCD.print("ДЕЁЖ", 96, 202, 0);  
            myGLCD.setColor(100, 100, 100);
          }
        }  else  {
          if ( y > 200 ) {                     //   нажата Mode
            modePoliv = !modePoliv;
            drawBeep();
            myGLCD.setFont( SmallRusFont );
            clearStr ("\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0", 196, 205);
            myGLCD.setColor(100, 100, 100);
            drawModePoliff();
            modePoliffFlag = true;          
            drawModePoliffFlag();    
                    
          } else  {    // нажата - время полива
            drawBeep();
          //  FlexiTimer2::stop();
            SetModeScreen( );
            oldHumidity = 0; oldTemperature = 0;
            oldTemperatureOut = 0;
            oldMinute = 255; oldHour = 255; oldDayOfWeek = 0; oldDayOfMonth = 0; oldMonth = 0; oldYear = 0; oldGroundVal = 101;
            drawFirstScreen();
          //  FlexiTimer2::start();
          }
        }
      }              //  конец y >= 160 нижний рял
    }               //  конец myTouch.dataAvailable
  }  else {
    
 //-------------------------------------------------------------------------------
  //  poliff           = false;       // включается на передней панели
  //  poliffTime       = false;       // включается в указанный промежуток времени
  //  poliffPochva     = false;       // включается от датчика влажности почвы          true - сухо
  //  noRain           = false;       // в начале была почва мокрая значит дождь
  //  modePoliffFlag   = true;        // работает от modePoliv --  true - ежедневно
  //  poliffSensorTime = true;        // true - отключает полив по времени, false - по датчику почвы
  //  pinReleFlag      = false;       // реле ВКЛ / ВЫКЛ

    
    if ( poliff && noRain && modePoliffFlag ) {      
      int lostTime ;
      if ( (finishPolifHour * 60 + finishPolifMinute ) < (oldHour * 60 + oldMinute) ) 
        lostTime =  (finishPolifHour * 60 + finishPolifMinute ) - (oldHour * 60 + oldMinute)  + 1440 ;
       else 
        lostTime =  (finishPolifHour * 60 + finishPolifMinute ) - (oldHour * 60 + oldMinute) ;

      if ( oldLostTime != lostTime || firstTimeEnter ) {
          myGLCD.setColor(VGA_WHITE);
          myGLCD.setFont( SmallRusFont);
          myGLCD.print( "\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2", 160, 199, 0);
          myGLCD.setColor(100, 100, 100);
          String lostStr;
          if ( lostTime / 60 > 0 ) {
             int hourStr = lostTime / 60;
             int minStr  = lostTime % 60;
             lostStr = String ( hourStr ) + "\xA7""  " + String ( minStr ) + "\xA1""\x9D""\xA2";          
          } else             
             lostStr = String ( lostTime ) + " ""\xA1""\x9D""\xA2";          
          myGLCD.print( lostStr , 150 + (( 155 - lostStr.length() * 8 ) / 2), 199, 0);
          myGLCD.print( lostStr , 151 + (( 155 - lostStr.length() * 8 ) / 2), 199, 0);
          oldLostTime = lostTime;
          firstTimeEnter = false;
      }
      if  ( !poliffSensorTime ) {                              // полив по датчику полива
        if ( !poliffPochva  ) {                                // почва мокрая
           if ( pinReleFlag ) { 
             digitalWrite( pinRele , HIGH );                   // выключение воды
             pinReleFlag  = false;
           }
        } else {
         if ( !pinReleFlag ) { 
            digitalWrite( pinRele , LOW );                      // включение воды  если снова высохла
            pinReleFlag  = true;
          }  
        }
      }
    }      // end if ( poliff && noRain && modePoliffFlag )
    
    if (myTouch.dataAvailable())
    {
      myTouch.read();
      x = myTouch.getX();
      y = myTouch.getY();     
      if ( y >= 160 )  {                                // нижний ряд
        if ( x <= 130 ) { 
          poliff  = !poliff;                            //  лейка OFF/ON                                                           
          drawBeep();                                  
          poliffTime = false;                           //  отключение полива, возврат в основной режим
          if ( pinReleFlag ) { 
             digitalWrite( pinRele , HIGH );            // выключение воды
             pinReleFlag  = false;
          }
          myGLCD.setColor(VGA_WHITE);
          myGLCD.fillRoundRect (143, 170,  315, 220);
          myGLCD.setFont( BigRusFont);
          clearStr ("\xB2""\xB2""\xB2", 80, 202);
          drawDownFirstScreen();
        } 
      }
    }                //  конец if (myTouch.dataAvailable())
  }                  //  конец if ( !poliffTime )



  //    проверка времени и времени полива
  if ( ( oldHour == startPolifHour)  && ( oldMinute == startPolifMinute ) && !poliffTime) { 
      noRain = poliffPochva;  
      
      if (( fabs ( finishPolifHour - oldHour ) > 4 ) && poliffSensorTime ) noRain = true ;   //   если время полива больше 4 часов, и задан полив по сенсору
                                                                                             //   выключаем дождь в принципе
      if ( poliff && noRain && modePoliffFlag ) {  
        poliffTime = true;
        drawWaterScreen();
      }
  }

  if ( ( oldHour == finishPolifHour) && ( oldMinute == finishPolifMinute)  && poliffTime ) {
    poliffTime = false;
    if ( poliff ) {
       if ( pinReleFlag ) { 
           digitalWrite( pinRele , HIGH );             // выключение воды
           pinReleFlag  = false;
        }        
      myGLCD.setColor(VGA_WHITE);
      myGLCD.fillRoundRect (143, 170,  315, 220);
      drawDownFirstScreen();
    }
  }

}


// -----------------------------------------------------



void drawWaterScreen() {
  myGLCD.setColor(VGA_WHITE);
  myGLCD.fillRoundRect (143, 170,  315, 220);
  myGLCD.setColor(180, 180, 180);
  myGLCD.drawRoundRect (150, 170,  305, 220);
  myGLCD.setColor(100, 100, 100);
  myGLCD.setFont( BigRusFont);
  myGLCD.print("\xA3""o""\xA0""\x9D""\x97""a""\xAE", 170, 175, 0);
   if ( !pinReleFlag ) { 
      digitalWrite( pinRele , LOW );    // включение воды
      pinReleFlag  = true;
    }  
}


void DrawPressBottom ( int firstX, int firstY, int sizeX, String litera ) {

  myGLCD.setColor(VGA_YELLOW);
  myGLCD.fillRoundRect (firstX, firstY,  firstX + sizeX, firstY + 30 );
  myGLCD.setColor(180, 180, 180);
  myGLCD.drawRoundRect (firstX, firstY,  firstX + sizeX, firstY + 30  );
  myGLCD.setColor(VGA_BLACK);
  myGLCD.print(litera, firstX + (sizeX - litera.length() * 16 )/2, firstY + 7, 0);
  delay(200);
  myGLCD.setColor(100, 100, 100);
  myGLCD.fillRoundRect (firstX, firstY,  firstX + sizeX, firstY + 30  );
  myGLCD.setColor(0, 0, 0);
  myGLCD.drawRoundRect (firstX, firstY,  firstX + sizeX, firstY + 30  );
  myGLCD.setColor(VGA_WHITE);
  myGLCD.print(litera, firstX + (sizeX - litera.length() * 16 )/2, firstY + 7, 0);
 
}


void SetTimeScreen() {

  boolean enter = true;
  byte YYY = 160;          // координата вывода клавиатуры
  int  positX[6]  = { 50, 98, 50, 50, 102, 178 };
  int  positY[6]  = { 40, 40, 65, 90, 90, 90 };
  byte positV[6]  = { oldHour, oldMinute, oldDayOfWeek, oldDayOfMonth, oldMonth, oldYear };
  int  posit      = 0;    // указатель на все 3 массива
  int  volume     = oldHour;
  int  two        = 0;
  String daysOfWeekRus[7] = { "\x89""o""\xA2""e""\x99""e""\xA0""\xAC""\xA2""\x9D""\x9F",
                              "B""\xA4""op""\xA2""\x9D""\x9F",
                              "Cpe""\x99""a",
                              "\x8D""e""\xA4""\x97""ep""\x98",
                              "\x89""\xAF""\xA4""\xA2""\x9D""\xA6""a",
                              "Cy""\x96""\x96""o""\xA4""a",
                              "Boc""\x9F""pece""\xA2""\xAC""e"
                            };

  myGLCD.fillScr(VGA_WHITE);
  drawButtons( YYY );

  myGLCD.setFont(BigRusFont);
  myGLCD.setColor(VGA_BLACK);
  myGLCD.drawRoundRect (20, 30,  300, 120);

  // время
  myGLCD.setColor(100, 100, 100);
  if ( positV[0] < 10 ) {
    myGLCD.print( "0" + String ( positV[0] ), positX[0], positY[0], 0);
  } else {
    myGLCD.print( String ( positV[0] ), positX[0], positY[0], 0) ;
  }
  myGLCD.print(":", positX[0] + 32, positY[0], 0);
  if ( positV[1] < 10 ) {
    myGLCD.print( "0" + String ( positV[1] ), positX[1], positY[1], 0);
  } else {
    myGLCD.print( String ( positV[1] ), positX[1], positY[1], 0);
  }

  // день недели
  myGLCD.setColor(100, 100, 100);
  myGLCD.print(String ( positV[2] ), positX[2], positY[2], 0);
  myGLCD.setColor(200, 200, 200);
  myGLCD.print(daysOfWeekRus[positV[2] - 1], positX[2] + 40, positY[2], 0);

  // дата
  myGLCD.setColor(100, 100, 100);
  if ( positV[3] < 10 ) {
    myGLCD.print("0" + String ( positV[3] ), positX[3], positY[3], 0);
  } else {
    myGLCD.print( String ( positV[3] ), positX[3], positY[3], 0);
  }
  myGLCD.print("/  /20" , positX[3] + 32 , positY[3], 0);
  if ( positV[4] < 10 ) {
    myGLCD.print("0" + String ( positV[4] ) , positX[4], positY[4], 0);
  } else {
    myGLCD.print(String ( positV[4] ) , positX[4], positY[4], 0);
  }
  myGLCD.print(String ( positV[5] ), positX[5], positY[5], 0);
  delay (500);
  int timeEsc = 0;
  do {

    PositTimeDraw ( positX[posit], positY[posit], volume, posit );
    if (myTouch.dataAvailable())
    {
      myTouch.read();
      x = myTouch.getX();
      y = myTouch.getY();
      if (( y >= ( YYY - 1 ) ) && ( y <= ( YYY + 31)))  // Upper row
      {
        if  ( posit == 2 ) {
          two++ ;
          volume = 0;
        }
        two++ ;
        drawBeep();
        if ( two == 1 ) volume = 0;                   // если нажали кнопку число, то предустановленное время меняется на 0

        if ((x >= 4) && (x <= 46))                    // Button: 1
        {
          volume = volume * 10 + 1;
          DrawPressBottom ( 4, YYY, 42, "1" );   
        }

        if ((x >= 49) && (x <= 91))                   // Button: 2
        {
          volume = volume * 10 + 2;
          DrawPressBottom ( 49, YYY, 42, "2" ); 
        }

        if ((x >= 94) && (x <= 136))                  // Button: 3
        {
          volume = volume * 10 + 3;
          DrawPressBottom ( 94, YYY, 42, "3" ); 
        }

        if ((x >= 139) && (x <= 181))                 // Button: 4
        {
          volume = volume * 10 + 4;
          DrawPressBottom ( 139, YYY, 42, "4" );
        }

        if ((x >= 184) && (x <= 226))                 // Button: 5
        {
          volume = volume * 10 + 5;
          DrawPressBottom ( 184, YYY, 42, "5" );
        }

        if ((x >= 229) && (x <= 271))                 // Button: 6
        {
          volume = volume * 10 + 6;
          DrawPressBottom ( 229, YYY, 42, "6" ); 
        }

        if ((x >= 274) && (x <= 316))                 // Button: 7
        {
          volume = volume * 10 + 7;
          DrawPressBottom ( 274, YYY, 42, "7" ); 
        }
      }

      if ((( y >= 30 ) && ( y <= 120)) && (( x >= 20 ) && ( x <= 300)))    {
        enter = !enter;   // ESC
        drawBeep();
      }

      if (( y >= YYY + 32 ) && ( y <= YYY + 63))  //  down row
      {
        drawBeep();
        if ((x >= 4) && (x <= 46))                        // Button: 8
        {
          two++  ;
          if ( two == 1 ) volume = 0;
          if  ( posit == 2 ) {
            two++ ;
            volume = 0;
          }
          volume = volume * 10 + 8;
          DrawPressBottom ( 4, YYY + 33, 42, "8" ); 
        }

        if ((x >= 49) && (x <= 91))                       // Button: 9
        {
          two++  ;
          if  ( posit == 2 ) {
            two++ ;
            volume = 0;
          }
          if ( two == 1 ) volume = 0;
          volume = volume * 10 + 9;
          DrawPressBottom ( 49, YYY + 33, 42, "9" );
        }
        if ((x >= 94) && (x <= 136))                       // Button: 0
        {
          two++  ;
          if  ( posit == 2 ) {                             // день недели одна позиция
            two++ ;
            volume = 0;
          }
          if ( two == 1 ) volume = 0;
          volume = volume * 10;
          DrawPressBottom ( 94, YYY + 33, 42, "0" ); 
          if ( ( two == 2 ) &&  (volume == 0 ) ) { 
            if ( posit == 0 || posit == 1 )  {       
              PositTimeDraw ( positX[posit], positY[posit], volume, posit );
              positV[posit] = volume;
              posit++;
              if ( posit > 5 ) posit = 0;
              volume = positV[posit];
              two  = 0;
            }  
          }          
        }

        if ((x >= 139) && (x <= 181)) // Button: Left
        {
          DrawPressBottom ( 139, YYY + 33, 42, "\xB0" );
          switch (posit) {
            case 0:             // часов
              PositTimeDraw ( positX[posit], positY[posit], volume, posit );
              positV[posit] = volume;
              posit--;
              if ( posit < 0 ) posit = 5;
              volume = positV[posit];
              two  = 0;
              break;
            case 1:             // минут
              PositTimeDraw ( positX[posit], positY[posit], volume, posit );
              positV[posit] = volume;
              posit--;
              if ( posit < 0 ) posit = 5;
              volume = positV[posit];
              two  = 0;
              break;
            default:
              if ( volume != 0 ) {
                PositTimeDraw ( positX[posit], positY[posit], volume, posit );
                positV[posit] = volume;
                posit--;
                if ( posit < 0 ) posit = 5;
                volume = positV[posit];
                two  = 0;
              }
          }
        }

        if ((x >= 184) && (x <= 226)) // Button: Right
        {
          DrawPressBottom ( 184, YYY + 33, 42, "\xB1" ); 
          switch (posit) {
            case 0:             // часов
              PositTimeDraw ( positX[posit], positY[posit], volume, posit );
              positV[posit] = volume;
              posit++;
              if ( posit > 5 ) posit = 0;
              volume = positV[posit];
              two  = 0;
              break;
            case 1:             // минут
              PositTimeDraw ( positX[posit], positY[posit], volume, posit );
              positV[posit] = volume;
              posit++;
              if ( posit > 5 ) posit = 0;
              volume = positV[posit];
              two  = 0;
              break;
            default:
              if ( volume != 0 ) {
                PositTimeDraw ( positX[posit], positY[posit], volume, posit );
                positV[posit] = volume;
                posit++;
                if ( posit > 5 ) posit = 0;
                volume = positV[posit];
                two  = 0;
              }
          }
        }

        if ((x >= 229) && (x <= 316)) // Button: Enter
        {
          DrawPressBottom ( 229, YYY + 33, 87, "Enter" ); 
          if ( volume == 0 ) {
            AlarmStupid( YYY, "ERROR enter. Don't set 00 ! " );
          } else {
          //  setDateDs1307(0, positV[1], positV[0], positV[2], positV[3], positV[4], positV[5]);
            enter = !enter;
          }
        }
      }     //  down row

      if ( two == 2 ) {
        switch (posit) {
          case 0:
            if  ( volume  > 23  ) {
              AlarmStupid( YYY, "Bce""\x98""o 24 ""\xA7""aca ""\x97"" o""\x99""\xA2""o""\xA1"" ""\x99""\xA2""e!!" );
              volume = 0;
            }
            break;
          case 1:
            if  ( volume  > 59  ) {
              AlarmStupid( YYY, "Bce""\x98""o 60 ""\xA1""\x9D""\xA2""y""\xA4"" ""\x97"" o""\x99""\xA2""o""\xA1"" ""\xA7""ace!!" );
              volume = 0;
            }
            break;
          case 2:
            if  ( ( volume  > 7) || ( volume  < 1)) {
              AlarmStupid( YYY, "Bce""\x98""o 7 ""\x99""\xA2""e""\x9E"" ""\x97"" ""\xA2""e""\x99""e""\xA0""e!!!" );
              volume = oldDayOfWeek;
            }
            break;
          case 3:
            if  ( volume  > 31  ) {
              AlarmStupid( YYY, "Bce""\x98""o 31 ""\x99""e""\xA2""\xAC"" ""\x97"" ""\xA2""e""\x99""e""\xA0""e!!!" );
              volume = 0;
            }
            break;
          case 4:
            if ( volume  > 12 )  {
              AlarmStupid( YYY, "Bce""\x98""o 12 ""\xA2""e""\x99""\xAF""\xA0""e""\x97"" ""\x97"" ""\x98""o""\x99""y" );
              volume = 0;
            }
            break;
          case 5:
            if  ( volume <  oldYear  ) {
              AlarmStupid( YYY, "\x89""po""\x97""ep""\xAC"", ""\x98""o""\x99"" ""\xA0""a""\x9B""o""\x97""\xAB""\x9E""!!!" );
              volume = 0;
            }
            break;
        }
        two  = 0;
        if ( volume != 0 ) {
          PositTimeDraw ( positX[posit], positY[posit], volume, posit );
          positV[posit] = volume;
          posit++;
          if ( posit > 5 ) posit = 0;
          volume = positV[posit];
        }
      }

    }            //   myTouch.dataAvailable
    delay (300);
    ++timeEsc;
    if ( timeEsc == 400 && !myTouch.dataAvailable()) enter = !enter;
  } while (enter);
}


void PositTimeDraw ( int posX, int posY, int vol, int pos ) {

  String daysOfWeekRus[7] = { "\x89""o""\xA2""e""\x99""e""\xA0""\xAC""\xA2""\x9D""\x9F",
                              "B""\xA4""op""\xA2""\x9D""\x9F",
                              "Cpe""\x99""a",
                              "\x8D""e""\xA4""\x97""ep""\x98",
                              "\x89""\xAF""\xA4""\xA2""\x9D""\xA6""a",
                              "Cy""\x96""\x96""o""\xA4""a",
                              "Boc""\x9F""pece""\xA2""\xAC""e"
                            };

  myGLCD.setColor(VGA_WHITE);
  myGLCD.setFont( BigRusFont );

  if ( pos == 2 )  {
    myGLCD.print( "\xB2", posX , posY);
    myGLCD.setColor(VGA_WHITE);
    myGLCD.print( "\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2""\xB2", posX + 36, posY);
    myGLCD.setColor(200, 200, 200);
    myGLCD.print( daysOfWeekRus[vol - 1], posX + 40, posY, 0 );
    myGLCD.setColor(100, 100, 100);
    delay (400);
    myGLCD.print( String (vol), posX , posY);
  }
  else {
    myGLCD.print( "\xB2""\xB2", posX , posY);
    myGLCD.setColor(100, 100, 100);
    delay (400);
    if ( vol < 10 ) {
      myGLCD.print( "0" + String (vol), posX , posY);
    } else {
      myGLCD.print( String (vol), posX , posY);
    }
  }
  delay (400);
}



void PositDraw ( int posX, int posY, int vol ) {

  myGLCD.setColor(VGA_WHITE);
  myGLCD.setFont( BigRusFont );
  myGLCD.print( "\xB2""\xB2", posX , posY);
  myGLCD.setColor(VGA_BLACK);
  delay (400);
  if ( vol < 10 ) {
    myGLCD.print( "0" + String (vol), posX , posY);
  } else {
    myGLCD.print( String (vol), posX , posY);
  }
  delay (400);
}



void SetModeScreen() {

  boolean enter = true;    // кнопка Enter
  byte YYY = 165;          // координата вывода клавиатуры
  int  positX[4]  = { 205, 253, 205, 253 };
  int  positY[4]  = { 40, 40, 65, 65 };
  byte positV[4]  = { startPolifHour, startPolifMinute, finishPolifHour, finishPolifMinute };
  int  posit      = 0;    // указатель на все 3 массива
  int  volume     = startPolifHour;
  int  two        = 0;

  myGLCD.fillScr(VGA_WHITE);
  drawButtons( YYY );

  myGLCD.setColor(180, 180, 180);
  myGLCD.setFont( SmallRusFont );
  myGLCD.print("Ha""\xA7""a""\xA0""o  ""\xA3""o""\xA0""\x9D""\x97""a   ""\x97"":", 35 , positY[0] + 2);
  myGLCD.print("Ha""\xA7""a""\xA0""o  ""\xA3""o""\xA0""\x9D""\x97""a   ""\x97"":", 36 , positY[0] + 2);
  myGLCD.print("O""\x9F""o""\xA2""\xA7""a""\xA2""\x9D""e ""\xA3""o""\xA0""\x9D""\x97""a ""\x97"":", 35 , positY[2] + 4);
  myGLCD.print("O""\x9F""o""\xA2""\xA7""a""\xA2""\x9D""e ""\xA3""o""\xA0""\x9D""\x97""a ""\x97"":", 36 , positY[2] + 4);

  myGLCD.print("Pe""\x9B""\x9D""\xA1"" ""\xA3""o""\xA0""\x9D""\x97""a #MODE:", 35 , positY[2] + 43);
  myGLCD.print("Pe""\x9B""\x9D""\xA1"" ""\xA3""o""\xA0""\x9D""\x97""a      :", 36 , positY[2] + 43);

  myGLCD.setColor(VGA_BLACK);
  myGLCD.drawRoundRect (195, 97 , 295 , 128);
  myGLCD.setColor(180, 180, 180);
  myGLCD.fillRoundRect (195, 97 , 295 , 128);

  myGLCD.setFont( BigRusFont );
  myGLCD.setColor(VGA_WHITE);
  if ( poliffSensorTime ) {
    myGLCD.print("time", 213, 104, 0);
  }  else  {
    myGLCD.print("sens", 213, 104, 0);
  }


  myGLCD.setColor(VGA_BLACK);
  myGLCD.drawRoundRect (positX[0] - 5, positY[0] - 5,  positX[3] + 37, positY[3] + 21);

  if ( positV[0] < 10 ) {
    myGLCD.print("0" + String (positV[0]) + ":", positX[0] , positY[0]);
  } else {
    myGLCD.print( String (positV[0]) + ":", positX[0] , positY[0]);
  }

  if ( positV[1] < 10 ) {
    myGLCD.print("0" + String (positV[1]), positX[1] , positY[1]);
  } else {
    myGLCD.print( String (positV[1]), positX[1] , positY[1]);
  }

  if ( positV[2] < 10 ) {
    myGLCD.print( "0" + String (positV[2]) + ":" , positX[2] , positY[2]);
  } else {
    myGLCD.print( String (positV[2]) + ":" , positX[2] , positY[2]);
  }

  if ( positV[3] < 10 ) {
    myGLCD.print( "0" + String (positV[3]), positX[3] , positY[3]);
  } else {
    myGLCD.print( String (positV[3]), positX[3] , positY[3]);
  }
  delay (500);
  int timeEsc = 0;
  do {
    PositDraw ( positX[posit], positY[posit], volume );
    if (myTouch.dataAvailable())
    {
      myTouch.read();
      x = myTouch.getX();
      y = myTouch.getY();

      if (( y >= 97 ) && ( y <= 128 )) {               // Mode buttom
        if (( x >= 195 ) && ( x <= 295 )) {
          poliffSensorTime = !poliffSensorTime;
          myGLCD.setColor(180, 180, 180);
          drawBeep();
          myGLCD.print("\xB2""\xB2""\xB2""\xB2", 213, 104, 0);
          myGLCD.setColor(VGA_WHITE);
          if ( poliffSensorTime ) {
            myGLCD.print("time", 213, 104, 0);
          }  else  {
            myGLCD.print("sens", 213, 104, 0);
          }
        }
      }

      if (( y >= ( YYY - 1 ) ) && ( y <= ( YYY + 31)))  // Upper row
      {
        two++ ;
        drawBeep();
        if ( two == 1 ) volume = 0;                   // если нажали кнопку число, то предустановленное время меняется на 0

        if ((x >= 4) && (x <= 46))                    // Button: 1
        {
          volume = volume * 10 + 1;
          DrawPressBottom ( 4, YYY, 42, "1" ); 
        }

        if ((x >= 49) && (x <= 91))                   // Button: 2
        {
          volume = volume * 10 + 2;
          DrawPressBottom ( 49, YYY, 42, "2" ); 
        }

        if ((x >= 94) && (x <= 136))                  // Button: 3
        {
          volume = volume * 10 + 3;
          DrawPressBottom ( 94, YYY, 42, "3" ); 
        }

        if ((x >= 139) && (x <= 181))                 // Button: 4
        {
          volume = volume * 10 + 4;
          DrawPressBottom ( 139, YYY, 42, "4" );
        }

        if ((x >= 184) && (x <= 226))                 // Button: 5
        {
          volume = volume * 10 + 5;
          DrawPressBottom ( 184, YYY, 42, "5" );
        }

        if ((x >= 229) && (x <= 271))                 // Button: 6
        {
          volume = volume * 10 + 6;
          DrawPressBottom ( 229, YYY, 42, "6" ); 
        }

        if ((x >= 274) && (x <= 316))                 // Button: 7
        {
          volume = volume * 10 + 7;
          DrawPressBottom ( 274, YYY, 42, "7" ); 
        }
      }

      if ((( y >= positY[0] - 5 ) && ( y <= positY[3] + 21)) && (( x >= positX[0] - 5 ) && ( x <= positX[3] + 37)))  {
        enter = !enter;   // ESC
        drawBeep();
      }



      if (( y >= YYY + 32 ) && ( y <= YYY + 63))  //  down row
      {
        drawBeep();
        if ((x >= 4) && (x <= 46))                        // Button: 8
        {
          two++  ;
          if ( two == 1 ) volume = 0;
          volume = volume * 10 + 8;
          DrawPressBottom ( 4, YYY + 33, 42, "8" ); 
        }

        if ((x >= 49) && (x <= 91))                       // Button: 9
        {
          two++  ;
          if ( two == 1 ) volume = 0;
          volume = volume * 10 + 9;
          DrawPressBottom ( 49, YYY + 33, 42, "9" ); 
        }
        if ((x >= 94) && (x <= 136))                       // Button: 0
        {
          two++  ;
          if ( two == 1 ) volume = 0;
          volume = volume * 10;
          DrawPressBottom ( 94, YYY + 33, 42, "0" ); 
        }

        if ((x >= 139) && (x <= 181)) // Button: Left
        {
          DrawPressBottom ( 139, YYY + 33, 42, "\xB0" ); 
          PositDraw ( positX[posit], positY[posit], volume );
          positV[posit] = volume;
          posit--;
          if ( posit < 0 ) posit = 3;
          volume = positV[posit];
          two  = 0;
        }

        if ((x >= 184) && (x <= 226)) // Button: Right
        {
          DrawPressBottom ( 184, YYY + 33, 42, "\xB1" ); 
          PositDraw ( positX[posit], positY[posit], volume );
          positV[posit] = volume;
          posit++;
          if ( posit > 3 ) posit = 0;
          volume = positV[posit];
          two  = 0;
        }

        if ((x >= 229) && (x <= 316)) // Button: Enter
        {
          DrawPressBottom ( 229, YYY + 33, 87, "Enter" ); 
          positV[posit] = volume;
          startPolifHour    = positV[0];
          startPolifMinute  = positV[1];
          finishPolifHour   = positV[2];
          finishPolifMinute = positV[3];

          if ((positV[0] == positV[2]) && (positV[1] == positV[3])) AlarmStupid ( YYY, "\x89""po""\x97""ep""\xAC"", ""\x97""pe""\xA1""\xAF"" ""\xA0""a""\x9B""o""\x97""oe!!!" );
          delay (500);

          EEPROM.write(0, positV[0]);
          EEPROM.write(1, positV[1]);
          EEPROM.write(2, positV[2]);
          EEPROM.write(3, positV[3]);

          enter = !enter;

        }
      }

      if ( two == 2 ) {
        volume = ChoisePos ( YYY, posit, volume );
        two  = 0;
        PositDraw ( positX[posit], positY[posit], volume );
        positV[posit] = volume;
        posit++;
        if ( posit > 3 ) posit = 0;
        volume = positV[posit];
      }

    }            //   myTouch.dataAvailable
    delay (300);
    ++timeEsc;
    if ( timeEsc == 400 && !myTouch.dataAvailable()) enter = !enter;
  } while (enter);

}





void AlarmStupid ( int Y, String str ) {

  myGLCD.setFont( SmallRusFont );
  myGLCD.setColor(VGA_RED);
  myGLCD.print( str, (320 - str.length() * 8) / 2 , Y - 30);
  myGLCD.print( str, (320 - str.length() * 8) / 2 + 1 , Y - 30);
  delay (2000) ;
  myGLCD.setColor(VGA_WHITE);
  for ( int i = 0; i <= str.length(); i++ ) {
    myGLCD.print( "\xB0", (320 - str.length() * 8) / 2 + i * 8 , Y - 30);
  }
}



void drawButtons( int YY ) {

  myGLCD.setFont(BigRusFont);
  int XX = 4;
  int sizeX = 42;
  int sizeY = 30;
  int otstup = 3;


  for (x = 0; x < 7; x++)
  {
    myGLCD.setColor(100, 100, 100);
    myGLCD.fillRoundRect (XX + (x * sizeX) + (otstup * x), YY, XX + (sizeX * ( x + 1 )) + otstup *  x, YY + sizeY);
    myGLCD.setColor(0, 0, 0);
    myGLCD.drawRoundRect (XX + (x * sizeX) + (otstup * x), YY, XX + (sizeX * ( x + 1 )) + otstup *  x, YY + sizeY);
    myGLCD.setColor(VGA_WHITE);
    myGLCD.printNumI(x + 1, XX + sizeX / 2 - 8 + (sizeX + otstup) * x, YY + sizeY / 2 - 8);
  }
  for (x = 0; x < 5; x++)
  {
    myGLCD.setColor(100, 100, 100);
    myGLCD.fillRoundRect (XX + (x * sizeX) + (otstup * x), YY + sizeY + otstup,  XX + (sizeX * ( x + 1 )) + otstup * x, YY + sizeY * 2 + otstup);
    myGLCD.setColor(0, 0, 0);
    myGLCD.drawRoundRect (XX + (x * sizeX) + (otstup * x), YY + sizeY + otstup,  XX + (sizeX * ( x + 1 )) + otstup * x, YY + sizeY * 2 + otstup);
    myGLCD.setColor(VGA_WHITE);

    switch (x) {
      case 2:
        myGLCD.print("0", XX + sizeX / 2 - 8 + (x * sizeX) + (otstup * x), YY + sizeY + otstup + sizeY / 2 - 8);         // 0
        break;
      case 3:
        myGLCD.print("\xB0", XX + sizeX / 2 - 8 + (x * sizeX) + (otstup * x), YY + sizeY + otstup + sizeY / 2 - 8);      // стрелка влево
        break;
      case 4:
        myGLCD.print("\xB1", XX + sizeX / 2 - 8 + (x * sizeX) + (otstup * x), YY + sizeY + otstup + sizeY / 2 - 8);      // стрелка вправо
        break;
      default:
        myGLCD.printNumI( x + 8, XX + sizeX / 2 - 8 + (sizeX + otstup) * x, YY + sizeY + otstup + sizeY / 2 - 8);
    }
  }

  // Enter
  myGLCD.setColor(100, 100, 100);
  myGLCD.fillRoundRect (XX + 5 * (otstup + sizeX), YY + sizeY + otstup,  XX + (6 * otstup) + (sizeX * 7),  YY + sizeY * 2 + otstup);
  myGLCD.setColor(0, 0, 0);
  myGLCD.drawRoundRect (XX + 5 * (otstup + sizeX), YY + sizeY + otstup,  XX + (6 * otstup) + (sizeX * 7),  YY + sizeY * 2 + otstup);
  myGLCD.setColor(VGA_WHITE);
  myGLCD.print("Enter", XX + 5 * (otstup + sizeX) + sizeX - 39, YY + sizeY + otstup + 8);
}


void drawDownFirstScreen() {

  myGLCD.setFont( BigRusFont);
  if ( poliff ) {
    myGLCD.setColor(100, 100, 100);
    myGLCD.print("on", 96, 202, 0);
  }  else  {
    myGLCD.setColor(220, 220, 220);
    myGLCD.print("off", 80, 202, 0);
  }

  myGLCD.setColor(100, 100, 100);
  myGLCD.setFont( SmallRusFont );
  myGLCD.print("\x89""o""\xA0""\x9D""\x97"":", 143, 172, 0);

  if ( startPolifHour < 10 )  {
    myGLCD.print( "0" + String( startPolifHour ) + ":", 194, 172, 0);
    myGLCD.print( "0" + String( startPolifHour ) + ":", 195, 172, 0);
  } else {
    myGLCD.print(String( startPolifHour ) + ":", 194, 172, 0);
    myGLCD.print(String( startPolifHour ) + ":", 195, 172, 0);
  }

  if ( startPolifMinute < 10 )  {
    myGLCD.print( "0" + String( startPolifMinute ) + "-", 219, 172, 0);
    myGLCD.print( "0" + String( startPolifMinute ) + "-", 220, 172, 0);
  } else {
    myGLCD.print(String( startPolifMinute ) + "-", 219, 172, 0);
    myGLCD.print(String( startPolifMinute ) + "-", 220, 172, 0);
  }


  if ( finishPolifHour < 10 )  {
    myGLCD.print( "0" + String( finishPolifHour ) + ":", 243, 172, 0);
    myGLCD.print( "0" + String( finishPolifHour ) + ":", 244, 172, 0);
  } else {
    myGLCD.print(String( finishPolifHour ) + ":", 243, 172, 0);
    myGLCD.print(String( finishPolifHour ) + ":", 244, 172, 0);
  }

  if ( finishPolifMinute < 10 )  {
    myGLCD.print(  "0" + String( finishPolifMinute ), 267, 172, 0);
    myGLCD.print(  "0" + String( finishPolifMinute ), 268, 172, 0);
  } else {
    myGLCD.print(String( finishPolifMinute ), 267, 172, 0);
    myGLCD.print(String( finishPolifMinute ), 268, 172, 0);
  }


// картинка, режим:  S - T
  
  myGLCD.drawRoundRect (289, 170 , 302 , 185);
  if ( poliffSensorTime ) {
    myGLCD.print("T", 292, 172, 0);
    myGLCD.print("T", 293, 172, 0);
  }  else  {
    myGLCD.print("S", 292, 172, 0);
    myGLCD.print("S", 293, 172, 0);
  }


// картинка, режим:  ежедневно - через день

  drawModePoliffFlag();



  myGLCD.setColor(100, 100, 100);
  myGLCD.print("Pe""\x9B""\x9D""\xA1"":", 143, 205, 0);
  drawModePoliff();

}


void drawModePoliff() {
 if ( modePoliv ) {
    myGLCD.print("E""\x9B""e""\x99""\xA2""e""\x97""\xA2""o", 196, 205, 0);
    myGLCD.print("E""\x9B""e""\x99""\xA2""e""\x97""\xA2""o", 197, 205, 0);
  }  else  {
    myGLCD.print("\x8D""epe""\x9C"" ""\x99""e""\xA2""\xAC", 196, 205, 0);
    myGLCD.print("\x8D""epe""\x9C"" ""\x99""e""\xA2""\xAC", 197, 205, 0);
  }  
}



void drawModePoliffFlag() {

  if ( modePoliffFlag ) {
    myGLCD.setColor(0, 200, 0);
    myGLCD.fillRoundRect (289, 202 , 302 , 217);
    myGLCD.setColor(100, 100, 100);   
    myGLCD.drawRoundRect (289, 202 , 302 , 217);
    myGLCD.setColor(VGA_WHITE); 
    myGLCD.print("C", 292, 204, 0);
    myGLCD.print("C", 293, 204, 0);   
  }  else  {
    myGLCD.setColor(200, 200, 200);
    myGLCD.fillRoundRect (289, 202 , 302 , 217);
    myGLCD.setColor(170, 170, 170);
    myGLCD.drawRoundRect (289, 202 , 302 , 217);
    myGLCD.setColor(VGA_WHITE); 
    myGLCD.print("\x85", 292, 204, 0);
    myGLCD.print("\x85", 293, 204, 0);    
  }
  
}



void drawFirstScreen() {
  myGLCD.clrScr();

  myGLCD.fillScr(VGA_WHITE);
  myGLCD.setColor(100, 100, 100);

  myGLCD.drawBitmap (15, 20, 27, 50, gradusnik, 1);
  myGLCD.drawBitmap (105, 20, 27, 50, gradusnik, 1);
  myGLCD.drawBitmap (230, 30, 23, 35, vlag, 1);
  myGLCD.drawBitmap (15, 170, 75, 50, lleika, 1);

  myGLCD.setColor(100, 100, 100);
  myGLCD.setFont( SmallRusFont );
  myGLCD.print("\x97"".""\xA3""o""\xA7""\x97""\xAB"":", 65, 172, 0);
  myGLCD.print("IN ""\x7F""C", 48, 26, 0);
  myGLCD.print("OUT ""\x7F""C", 148, 26, 0);
  myGLCD.print("IN %", 264, 26, 0);

  if (!ds.search(addr)) { myGLCD.setColor(200, 200, 200); myGLCD.setFont( BigRusFont);  myGLCD.print( "n/u" , 148, 45, 0); }

  drawDownFirstScreen();
}



int ChoisePos (int YYY, int pos, int vol ) {
  switch (pos) {
    case 0:
      if  ( vol  > 23  ) {
        AlarmStupid( YYY, "Bce""\x98""o 24 ""\xA7""aca ""\x97"" o""\x99""\xA2""o""\xA1"" ""\x99""\xA2""e!!" );
        vol = 0;
      }
      break;
    case 1:
      if  ( vol  > 59  ) {
        AlarmStupid( YYY, "Bce""\x98""o 60 ""\xA1""\x9D""\xA2""y""\xA4""\x97"" o""\x99""\xA2""o""\xA1"" ""\xA7""ace!!" );
        vol = 0;
      }
      break;
    case 2:
      if  ( vol  > 23  ) {
        AlarmStupid( YYY, "Bce""\x98""o 24 ""\xA7""aca ""\x97"" o""\x99""\xA2""o""\xA1"" ""\x99""\xA2""e!!" );
        vol = 0;
      }
      break;
    case 3:
      if  ( vol  > 59  ) {
        AlarmStupid( YYY, "Bce""\x98""o 60 ""\xA1""\x9D""\xA2""y""\xA4""\x97"" o""\x99""\xA2""o""\xA1"" ""\xA7""ace!!" );
        vol = 0;
      }
      break;
  }
  return vol;
}


void drawTempOutside() {
  byte data[12];
  ds.reset();        // обнулить устройство
  ds.select(addr);   // выбор устройства по адресу
  ds.write(0x44);    // команда проверить температуру
  delay(1000);

  ds.reset();
  ds.select(addr);
  ds.write(0xBE);    // команда отправить температуру

  for (byte i = 0; i < 9; i++) data[i] = ds.read();  // читаем с устройства значение побайтно первые 8 байт

  float raw = (data[1] << 8) | data[0];

  float temperatureOut =  raw / 16 ;                 // перевод в 10-ю систему

  if ( oldTemperatureOut != temperatureOut )  {
    myGLCD.setFont( BigRusFont);
    clearStr ("\xB2""\xB2""\xB2""\xB2", 148, 45);
    myGLCD.setColor(100, 100, 100);
    if ( temperatureOut > 0 ) {
      myGLCD.print( "+" , 132, 45, 0);
      myGLCD.printNumF( temperatureOut, 1, 148, 45);
    } else {
      myGLCD.printNumF( temperatureOut, 1, 132, 45);
    }
    oldTemperatureOut = temperatureOut;
  } 
}


void drawTempHumidity()
{
  byte humidity = dht.readHumidity();
  delay (500);
  byte temperature = dht.readTemperature() - 2;       // корректирую нагрев в корпусе
  if ( temperature >= 0 && temperature <= 50 ) {
      if ( oldTemperature != temperature )  {
        myGLCD.setFont( BigRusFont);
        myGLCD.setColor(VGA_WHITE);
        clearStr ("\xB2""\xB2", 58, 45);
        myGLCD.setColor(100, 100, 100);
        myGLCD.print( "+" + String ( temperature ), 42, 45, 0);
        oldTemperature = temperature;
      }
      if ( (oldHumidity != humidity) && (humidity != 0 )) {
        myGLCD.setFont( BigRusFont);
        myGLCD.setColor(VGA_WHITE);
        myGLCD.print("\xB2""\xB2", 264, 45, 0);
        myGLCD.setColor(100, 100, 100);
        myGLCD.print( String ( humidity ), 264, 45, 0);
        oldHumidity = humidity;
      }
  } 
}


byte decToBcd(byte val)
{
  return ( (val / 10 * 16) + (val % 10) );
}


byte bcdToDec(byte val)
{
  return ( (val / 16 * 10) + (val % 16) );
}


void setDateDs1307(byte second,        // 0-59
                   byte minute,        // 0-59
                   byte hour,          // 1-23
                   byte dayOfWeek,     // 1-7
                   byte dayOfMonth,    // 1-28/29/30/31
                   byte month,         // 1-12
                   byte year)          // 0-99
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}


void getDateDs1307(byte *second,
                   byte *minute,
                   byte *hour,
                   byte *dayOfWeek,
                   byte *dayOfMonth,
                   byte *month,
                   byte *year)
{

  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);

  *second     = bcdToDec(Wire.read() & 0x7f);
  *minute     = bcdToDec(Wire.read());
  *hour       = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek  = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month      = bcdToDec(Wire.read());
  *year       = bcdToDec(Wire.read());
}



void clearStr( String str, byte  XX, byte YY )
{
  myGLCD.setColor(VGA_WHITE);
  myGLCD.print(str, XX, YY, 0);
}


void drawTime()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  String daysOfWeek[7] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
  String daysOfWeekRus[7] = { "\x89""o""\xA2""e""\x99""e""\xA0""\xAC""\xA2""\x9D""\x9F",
                              "B""\xA4""op""\xA2""\x9D""\x9F",
                              "Cpe""\x99""a",
                              "\x8D""e""\xA4""\x97""ep""\x98",
                              "\x89""\xAF""\xA4""\xA2""\x9D""\xA6""a",
                              "Cy""\x96""\x96""o""\xA4""a",
                              "Boc""\x9F""pece""\xA2""\xAC""e"
                            };

  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

  if ( oldHour != hour )  {
    myGLCD.setFont(SevenSegRusNumFont);
    clearStr (";;", 20, 98);
    myGLCD.setColor(100, 100, 100);
    if ( hour < 10 ) {
      myGLCD.print( "0" + String ( hour ), 20, 98, 0);
    } else {
      myGLCD.print( String ( hour ), 20, 98, 0);
    }
    oldHour = hour;
  }
  if ( oldMinute != minute )  {
    myGLCD.setFont(SevenSegRusNumFont);
    clearStr (";;", 20 + 32 * 3, 98);
    myGLCD.setColor(100, 100, 100);
    if ( minute < 10 ) {
      myGLCD.print( "0" + String ( minute ), 116, 98, 0);
    } else {
      myGLCD.print( String ( minute ), 116, 98, 0);
    }
    oldMinute = minute;
  }
  if ( oldDayOfWeek != dayOfWeek )  {
    myGLCD.setFont(SmallRusFont);
    clearStr ("\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0", 217, 98);
    myGLCD.setColor(100, 100, 100);
    myGLCD.print(daysOfWeek[dayOfWeek - 1], 217, 98, 0);
    clearStr ("\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0", 217, 115);
    myGLCD.setColor(100, 100, 100);
    myGLCD.print(daysOfWeekRus[dayOfWeek - 1], 217 , 115, 0);
    oldDayOfWeek = dayOfWeek;
  }
  if ( oldDayOfMonth != dayOfMonth )  {
    oldMonth = month ;
    oldYear = year;
    myGLCD.setFont(SmallRusFont);
    clearStr ("\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0""\xB0", 217, 132);
    myGLCD.setColor(100, 100, 100);
    myGLCD.print(String ( dayOfMonth ) + "/" + String ( month ) + "/20" + String ( year ), 217 , 132, 0);
    oldDayOfMonth = dayOfMonth;
    if ( modePoliv ) {
      modePoliffFlag = true ;
    } else {
      modePoliffFlag = !modePoliffFlag ;
    };                                            // ежедневно/ через день  -----   true - ежедневно
 
    drawModePoliffFlag();
    
  }

}


void drawBeep() {

tone(pinBeep, 100, 20);
tone(pinBeep, 200, 10);
tone(pinBeep, 1000, 80);
noTone;

}


void drawSensorGround() {

  int inpData = analogRead(pinGround);
  //  extrMax =  1008;  // - сухо
  //  extrMin =  150 ;  // - 100% влажно

  // 80 %  примерно 350
  float percento = 100 - (inpData - 150) / 8.6;
  if ( percento < 0 ) percento = 0;
  if ( percento >= 99 ) percento = 99;
  if ( percento >= 80 )  poliffPochva = false;   //  мокро
  if ( percento <= 70 )  poliffPochva = true;    //  сухо

   if ( byte (percento) != oldGroundVal ) {
      myGLCD.setFont( SmallRusFont );
      myGLCD.setColor(VGA_WHITE);
      myGLCD.print("\xB0""\xB0""\xB0""\xB0""\xB0", 100, 186, 0);
      if ( poliffPochva ) {
           myGLCD.setColor(100, 100, 100);  // сухо 
      } else {
           myGLCD.setColor(220, 220, 220);  // мокро
      }
      myGLCD.print(String (byte (percento))+ "%", 100, 186, 0);  
      myGLCD.print(String (byte (percento))+ "%", 101, 186, 0);
      myGLCD.setColor(100, 100, 100);
      oldGroundVal = byte (percento);
  }
}

