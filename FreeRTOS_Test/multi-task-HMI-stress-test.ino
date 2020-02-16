#include "Arduino.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <SPI.h>
#include "MFRC522.h"

#define RST_PIN 42
#define SS_PIN 53
#define BUZZ    48
MFRC522 mfrc522(SS_PIN, RST_PIN);
typedef struct
{
	uint8_t Data[16];
	int Len;
	bool Update;

}RFIDData;
RFIDData rfiddata;

#define PWM_2         2  //pwm motor control pulse pin
#define PWM_3         3  //pwm motor control pulse pin
#define PWM_6         6  //pwm motor control pulse pin
#define PWM_7         7  //pwm motor control pulse pin
#define PWM_11       11 //pwm motor control pulse pin
#define PWM_12       12 //pwm motor control pulse pin
#define PWM_44       44 //pwm motor control pulse pin
#define PWM_45       45 //pwm motor control pulse pin

static const uint8_t ledPin[] = {A0, A1, A2, A3, A4, A5, A6, A7};

SemaphoreHandle_t xSerialSemaphore;
void TasklReadBack( void *pvParameters );
void TasklLedFlash( void *pvParameters );
void TaskReadRfid( void *pvParameters );

bool ledStatus = true;
int incomingData = 0;

void setup()
{
	for(int i = 0 ; i <= 7 ; i++){
	  pinMode(ledPin[i], OUTPUT);
	}
	  Serial3.begin(115200);
	  Serial3.println("RFID reader is ready!");
	  SPI.begin();
	  noInterrupts(); // interrupt disable
	  Timer1Init();
	  Timer3Init();
	  Timer4Init();
	  Timer5Init();
	  interrupts(); 	/*interrupt enable*/
	  mfrc522.PCD_Init();   // 初始化MFRC522讀卡機模組

	 if ( xSerialSemaphore == NULL )  // Check to confirm that the Serial Semaphore has not already been created.
	  {
	    xSerialSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage the Serial Port
	    if ( ( xSerialSemaphore ) != NULL )
	      xSemaphoreGive( ( xSerialSemaphore ) );  // Make the Serial Port available for use, by "Giving" the Semaphore.
	  }

	xTaskCreate(
			TasklReadBack
	  ,  (const portCHAR *)"ReadBack"  // A name just for humans
	  ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
	  ,  NULL
	  ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
	  ,  NULL );

	xTaskCreate(
			TasklLedFlash
	  ,  (const portCHAR *)"LEDFlash"  // A name just for humans
	  ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
	  ,  NULL
	  ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
	  ,  NULL );

	xTaskCreate(
			TaskReadRfid
	  ,  (const portCHAR *)"ReadRfid"  // A name just for humans
	  ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
	  ,  NULL
	  ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
	  ,  NULL );
}

void loop()
{}
void Timer1Init(void)
{
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;
	OCR1A = 65535; /*compare match register 16MHz/65536/31.75KHz*/
	TCCR1B |= (1 << CS10)|(1 << CS11); /* 64 pre-scalar */
	TCCR1B |= (1 << WGM12); /* CTC mode*/
	TIMSK1 |= (1 << OCIE1A); /*Enable timer compare interrupt*/
}
void Timer3Init(void)
{
	TCCR3A = 0;
	TCCR3B = 0;
	TCNT3 = 0;
	OCR3A = 32768; /*compare match register 16MHz/65536/31.75KHz*/
	TCCR3B |= (1 << CS10)|(1 << CS11); /* 64 pre-scalar */
	TCCR3B |= (1 << WGM12); /* CTC mode*/
	TIMSK3 |= (1 << OCIE1A); /*Enable timer compare interrupt*/
}
void Timer4Init(void)
{
	TCCR4A = 0;
	TCCR4B = 0;
	TCNT4 = 0;
	OCR4A = 16384; /*compare match register 16MHz/65536/31.75KHz*/
	TCCR4B |= (1 << CS10)|(1 << CS11); /* 64 pre-scalar */
	TCCR4B |= (1 << WGM12); /* CTC mode*/
	TIMSK4 |= (1 << OCIE1A); /*Enable timer compare interrupt*/
}
void Timer5Init(void)
{
	TCCR5A = 0;
	TCCR5B = 0;
	TCNT5 = 0;
	OCR5A = 8192; /*compare match register 16MHz/65536/31.75KHz*/
	TCCR5B |= (1 << CS10)|(1 << CS11); /* 64 pre-scalar */
	TCCR5B |= (1 << WGM12); /* CTC mode*/
	TIMSK5 |= (1 << OCIE1A); /*Enable timer compare interrupt*/
}
	ISR(TIMER1_COMPA_vect)
{
	digitalWrite(PWM_2,digitalRead(PWM_2)^1);
	digitalWrite(PWM_3,digitalRead(PWM_3)^1);
	digitalWrite(ledPin[4],digitalRead(ledPin[4])^1);
}
ISR(TIMER3_COMPA_vect)
{
	digitalWrite(PWM_6,digitalRead(PWM_6)^1);
	digitalWrite(PWM_7,digitalRead(PWM_7)^1);
	digitalWrite(ledPin[5],digitalRead(ledPin[5])^1);
}
ISR(TIMER4_COMPA_vect)
{
	digitalWrite(PWM_11,digitalRead(PWM_11)^1);
	digitalWrite(PWM_12,digitalRead(PWM_12)^1);
	digitalWrite(ledPin[6],digitalRead(ledPin[6])^1);
}
ISR(TIMER5_COMPA_vect)
{
	digitalWrite(PWM_44,digitalRead(PWM_44)^1);
	digitalWrite(PWM_45,digitalRead(PWM_45)^1);
	digitalWrite(ledPin[7],digitalRead(ledPin[7])^1);
}

void buzzerPlay(int playMS)
{
  digitalWrite(BUZZ, HIGH);
  delay(playMS);
  digitalWrite(BUZZ, LOW);
}
void InitRFID(void)
{
  mfrc522.PCD_Init();
  rfiddata.Len = 0;
  rfiddata.Update =false;
}

void TasklReadBack( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  for (;;) // A Task shall never return or exit.
  {

    if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE  )
    {
    	if(Serial3.available() > 0){
    		incomingData = Serial3.read();
			Serial3.write(incomingData);
    	}
      xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
    }

    vTaskDelay(1);  // one tick delay (15ms) in between reads for stability
  }
}

void TasklLedFlash( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  for (;;) // A Task shall never return or exit.
  {
    	for(int i = 0 ; i <= 3 ; i += 2 ){
    		digitalWrite(ledPin[i],ledStatus);
    	}
    	ledStatus = !ledStatus;
    	for(int i = 1 ; i <= 3 ; i += 2){
    	digitalWrite(ledPin[i], ledStatus);
    	}
    vTaskDelay(10);  // one tick delay (15ms) in between reads for stability
  }
}
void TaskReadRfid( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  for (;;) // A Task shall never return or exit.
  {
    	if(mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()){
    		byte *id = mfrc522.uid.uidByte;   // 取得卡片的UID (第20行)
    		byte idSize = mfrc522.uid.size;   // 取得UID的長度
    		Serial3.print("PICC type: ");      // 顯示卡片類型
    		MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);   // 根據卡片回應的SAK值（mfrc522.uid.sak）判斷卡片類型
    		Serial3.println(mfrc522.PICC_GetTypeName(piccType));
    		Serial3.print("UID Size: ");       // 顯示卡片的UID長度值
    		Serial3.println(idSize);
    		buzzerPlay(10);
    		//mfrc522.PICC_HaltA();  // 讓卡片進入停止模式
    		for (byte i = 0; i < idSize; i++) {  // 逐一顯示UID碼
    		        Serial3.print("id[");
    		        Serial3.print(i);
    		        Serial3.print("]: ");
    		        Serial3.println(id[i], HEX);       // 以16進位顯示UID值
    		      }
    		  }
        vTaskDelay(10);  // one tick delay (150ms) in between reads for stability
    	}
  }







