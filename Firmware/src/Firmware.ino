#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <Nextion.h>
#include <dht.h>

// Defines * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#define CHECK_DOOR

#define DHT11_PIN 7
#define REED_DOOR_PIN 6
#define REED_WINDOW_1_PIN 5
#define REED_WINDOW_2_PIN 4
#define REED_WINDOW_3_PIN 3
#define WINDOW_OPEN_PIN 8
#define WINDOW_CLOSE_PIN 9

#define RELAY_OPEN HIGH
#define RELAY_CLOSE LOW

#define WINDOW_OPEN HIGH
#define WINDOW_CLOSE LOW

#define PAGE_OVERVIEW 0
#define PAGE_DOORS_WINDOWS 1
#define PAGE_MESSAGE_OK 2
#define PAGE_MESSAGE_ERROR 3

#define REFRESH_DHT_CYCLE 5000
#define REFRESH_STATE_CYCLE 500
#define RELAY_CLOSE_CYCLE 1000
#define DELAY_OK_MESSAGE 3000

// Variables * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

char textBuffer[10] = {0};
byte currentPage = PAGE_OVERVIEW;

long refreshTime = 0;

int currentTemperature = 0;
int currentHumidity = 0;

dht DHT11;

// Interface Components * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

// Page 00
NexPage pageOverview = NexPage( 0 , 0 , "page0" );

NexText temperatureText = NexText( 0 , 3 , "txtTemperature" );
NexText humidityText = NexText( 0 , 5 , "txtHumidity" );

NexPicture temperaturePicture = NexPicture( 0 , 2 , "pThermometer" );
NexPicture btnDoorsWindowsPage0 = NexPicture( 0 , 9 , "btnDoors" );

// Page 01
NexPage pageDoorsWindows = NexPage( 1 , 0 , "page1" );

NexPicture picDoor = NexPicture( 1 , 4 , "picDoor" );
NexPicture picWindow01 = NexPicture( 1 , 5 , "picWin01" );
NexPicture picWindow02 = NexPicture( 1 , 6 , "picWin02" );
NexPicture picWindow03 = NexPicture( 1 , 7 , "picWin03" );
NexPicture btnOverviewPage1 = NexPicture( 1 , 1 , "btnOverview" );

NexPicture btnWindowOpen = NexPicture( 1 , 8 , "btnOpen" );
NexPicture btnWindowClose = NexPicture( 1 , 9 , "btnClose" );
NexPicture btnHouseLeave = NexPicture( 1 , 10 , "btnLeave" );

// Page 02 (OK Message)
NexPage pageMessageOk = NexPage( 2 , 0 , "page2" );

// Page 03 (Error Message)
NexPage pageMessageError = NexPage( 3 , 0 , "page3" );
NexHotspot btnMessageOk = NexHotspot( 3 , 11 , "btnMsgOk" );
NexHotspot btnMessageClose = NexHotspot( 3 , 12 , "btnMsgClose" );
NexPicture picDoorP3 = NexPicture( 3 , 4 , "picDoor" );
NexPicture picWindow01P3 = NexPicture( 3 , 5 , "picWin01" );
NexPicture picWindow02P3 = NexPicture( 3 , 6 , "picWin02" );
NexPicture picWindow03P3 = NexPicture( 3 , 7 , "picWin03" );

// Listener Array * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

NexTouch *nex_listen_list[] = {
  &btnOverviewPage1,
  &btnDoorsWindowsPage0,
  &btnWindowOpen,
  &btnWindowClose,
  &btnHouseLeave,
  &btnMessageOk,
  &btnMessageClose,
  NULL
};

// Callback Functions * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

void btnDoorsPushCallback( void *ptr ) {
  pageDoorsWindows.show();
  currentPage = PAGE_DOORS_WINDOWS;
  displayWindowDoorState();
}

void btnOverviewPushCallback( void *ptr ) {
  pageOverview.show();
  currentPage = PAGE_OVERVIEW;
  displayTempHumidity( currentTemperature , currentHumidity );
}

void btnWindowOpenPushCallback( void *ptr ) {
  btnWindowOpen.setPic( 14 );
  digitalWrite( WINDOW_OPEN_PIN , RELAY_CLOSE );
  delay( RELAY_CLOSE_CYCLE );
  digitalWrite( WINDOW_OPEN_PIN , RELAY_OPEN );
  btnWindowOpen.setPic( 13 );
}

void btnWindowClosePushCallback( void *ptr ) {
  btnWindowClose.setPic( 12 );
  digitalWrite( WINDOW_CLOSE_PIN , RELAY_CLOSE );
  delay( RELAY_CLOSE_CYCLE );
  digitalWrite( WINDOW_CLOSE_PIN , RELAY_OPEN );
  btnWindowClose.setPic( 11 );
}

void btnHouseLeavePushCallback( void *ptr ) {
  if( checkWindowDoorState() == true ) {
    pageMessageOk.show();
    delay( DELAY_OK_MESSAGE );
    pageDoorsWindows.show();
    displayWindowDoorState();
  } else {
    currentPage = PAGE_MESSAGE_ERROR;
    pageMessageError.show();
    #ifdef CHECK_DOOR
      if( digitalRead( REED_DOOR_PIN ) == WINDOW_OPEN ) picDoorP3.setPic( 28 );
    #endif
    if( digitalRead( REED_WINDOW_1_PIN ) == WINDOW_OPEN ) picWindow01P3.setPic( 25 );
    if( digitalRead( REED_WINDOW_2_PIN ) == WINDOW_OPEN ) picWindow02P3.setPic( 26 );
    if( digitalRead( REED_WINDOW_3_PIN ) == WINDOW_OPEN ) picWindow03P3.setPic( 27 );
  }
}

void btnMessageOkCallback( void *ptr ) {
  currentPage = PAGE_DOORS_WINDOWS;
  pageDoorsWindows.show();
  displayWindowDoorState();
}

void btnMessageCloseCallback( void *ptr ) {
  digitalWrite( WINDOW_CLOSE_PIN , RELAY_CLOSE );
  delay( RELAY_CLOSE_CYCLE );
  digitalWrite( WINDOW_CLOSE_PIN , RELAY_OPEN );
  currentPage = PAGE_DOORS_WINDOWS;
  pageDoorsWindows.show();
  displayWindowDoorState();
}

// Setup Function * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

void setup() {

  pinMode( WINDOW_OPEN_PIN , OUTPUT );
  digitalWrite( WINDOW_OPEN_PIN , RELAY_OPEN );

  pinMode( WINDOW_CLOSE_PIN , OUTPUT );
  digitalWrite( WINDOW_CLOSE_PIN , RELAY_OPEN );

  pinMode( REED_DOOR_PIN , INPUT_PULLUP );
  pinMode( REED_WINDOW_1_PIN , INPUT_PULLUP );
  pinMode( REED_WINDOW_2_PIN , INPUT_PULLUP );
  pinMode( REED_WINDOW_3_PIN , INPUT_PULLUP );

  nexInit();

  btnDoorsWindowsPage0.attachPush( btnDoorsPushCallback );
  btnOverviewPage1.attachPush( btnOverviewPushCallback );

  btnWindowOpen.attachPush( btnWindowOpenPushCallback );
  btnWindowClose.attachPush( btnWindowClosePushCallback );
  btnHouseLeave.attachPush( btnHouseLeavePushCallback );

  btnMessageOk.attachPush( btnMessageOkCallback );
  btnMessageClose.attachPush( btnMessageCloseCallback );

  int dhtResult = DHT11.read11(DHT11_PIN);
  currentTemperature = DHT11.temperature;
  currentHumidity = DHT11.humidity;

}

void loop() {

  nexLoop( nex_listen_list );

  if( currentPage == PAGE_OVERVIEW ) {
    if( ( millis() - refreshTime ) > REFRESH_DHT_CYCLE ) {

      nexLoop( nex_listen_list );
      int dhtResult = DHT11.read11(DHT11_PIN);
      nexLoop( nex_listen_list );
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
    if( ( millis() - refreshTime ) > REFRESH_STATE_CYCLE ) {
      displayWindowDoorState();
      refreshTime = millis();
    }
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

void displayWindowDoorState() {
  #ifdef CHECK_DOOR
    if( digitalRead( REED_DOOR_PIN ) == WINDOW_CLOSE ) picDoor.setPic( 22 );
    else picDoor.setPic( 23 );
  #endif
  if( digitalRead( REED_WINDOW_1_PIN ) == WINDOW_CLOSE ) picWindow01.setPic( 15 );
  else picWindow01.setPic( 16 );
  if( digitalRead( REED_WINDOW_2_PIN ) == WINDOW_CLOSE ) picWindow02.setPic( 17 );
  else picWindow02.setPic( 18 );
  if( digitalRead( REED_WINDOW_3_PIN ) == WINDOW_CLOSE ) picWindow03.setPic( 19 );
  else picWindow03.setPic( 20 );
}

bool checkWindowDoorState() {
  #ifdef CHECK_DOOR
    if( digitalRead( REED_DOOR_PIN ) == WINDOW_OPEN ) return false;
  #endif
  if( digitalRead( REED_WINDOW_1_PIN ) == WINDOW_OPEN ) return false;
  if( digitalRead( REED_WINDOW_2_PIN ) == WINDOW_OPEN ) return false;
  if( digitalRead( REED_WINDOW_3_PIN ) == WINDOW_OPEN ) return false;
  return true;
}
