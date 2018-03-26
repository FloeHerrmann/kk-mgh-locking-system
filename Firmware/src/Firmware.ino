#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <Nextion.h>
#include <dht.h>

#define DHT11_PIN 7
#define REED_DOOR_PIN 6
#define REED_WINDOW_1_PIN 5
#define REED_WINDOW_2_PIN 4
#define REED_WINDOW_3_PIN 3
#define WINDOW_OPEN_PIN 8
#define WINDOW_CLOSE_PIN 9

#define PAGE_OVERVIEW 0
#define PAGE_DOORS_WINDOWS 1

char textBuffer[10] = {0};
byte currentPage = PAGE_OVERVIEW;

long refreshTime = 0;
long refreshCycle = 1000;

int currentTemperature = 0;
int currentHumidity = 0;

dht DHT11;

NexPage pageOverview = NexPage( 0 , 0 , "page0" );
NexText temperatureText = NexText( 0 , 3 , "txtTemperature" );
NexText humidityText = NexText( 0 , 5 , "txtHumidity" );
NexPicture temperaturePicture = NexPicture( 0 , 2 , "pThermometer" );

NexPicture btnDoorsWindowsPage0 = NexPicture( 0 , 9 , "btnDoors" );
NexPicture btnOverviewPage1 = NexPicture( 1 , 1 , "btnOverview" );

NexPage pageDoorsWindows = NexPage( 1 , 0 , "page1" );


NexTouch *nex_listen_list[] = {
  &btnOverviewPage1,
  &btnDoorsWindowsPage0,
  NULL
};

void btnDoorsPushCallback( void *ptr ) {
  pageDoorsWindows.show();
}

void btnOverviewPushCallback( void *ptr ) {
  pageOverview.show();
  displayTempHumidity( currentTemperature , currentHumidity );
}

void setup() {
  Serial.begin( 9600 );

  pinMode( WINDOW_OPEN_PIN , OUTPUT );
  digitalWrite( WINDOW_OPEN_PIN , HIGH );

  pinMode( WINDOW_CLOSE_PIN , OUTPUT );
  digitalWrite( WINDOW_CLOSE_PIN , LOW );

  pinMode( REED_DOOR_PIN , INPUT_PULLUP );
  pinMode( REED_WINDOW_1_PIN , INPUT_PULLUP );
  pinMode( REED_WINDOW_2_PIN , INPUT_PULLUP );
  pinMode( REED_WINDOW_3_PIN , INPUT_PULLUP );

  nexInit();

  btnDoorsWindowsPage0.attachPush( btnDoorsPushCallback );
  btnOverviewPage1.attachPush( btnOverviewPushCallback );

  //pageDoorsWindows.show();
}

void loop() {

  /*
  Serial.print( digitalRead( REED_DOOR_PIN ) );
  delay( 500 );
  */

  nexLoop( nex_listen_list );

  if( currentPage == PAGE_OVERVIEW ) {
    if( ( millis() - refreshTime ) > refreshCycle ) {

      int dhtResult = DHT11.read11(DHT11_PIN);
      switch( dhtResult ) {
        case DHTLIB_OK:
    		  break;
        case DHTLIB_ERROR_CHECKSUM:
    		  break;
        case DHTLIB_ERROR_TIMEOUT:
          break;
        case DHTLIB_ERROR_CONNECT:
            break;
        case DHTLIB_ERROR_ACK_L:
            break;
        case DHTLIB_ERROR_ACK_H:
          break;
        default:
      		break;
      }

      currentTemperature = DHT11.temperature;
      currentHumidity = DHT11.humidity;
      displayTempHumidity( currentTemperature , currentHumidity );

      refreshTime = millis();
    }
  } else if( currentPage == PAGE_DOORS_WINDOWS ) {

  }

}

void displayTempHumidity( int temperature , int humidity ) {
  memset( textBuffer , 0 , sizeof( textBuffer ) );
  itoa( temperature , textBuffer , 10 );
  temperatureText.setText( textBuffer );
  if( temperature <= 18 ) {
    temperatureText.Set_font_color_pco( 13660 );
    temperaturePicture.setPic( 1 );
  }
  if( temperature > 18 && temperature < 30 ) {
    temperatureText.Set_font_color_pco( 62592 );
    temperaturePicture.setPic( 2 );
  }
  if( temperature >= 30 ) {
    temperatureText.Set_font_color_pco( 47235 );
    temperaturePicture.setPic( 3 );
  }

  memset( textBuffer , 0 , sizeof( textBuffer ) );
  itoa( humidity , textBuffer , 10 );
  humidityText.setText( textBuffer );
  humidityText.Set_font_color_pco( 13660 );
}
