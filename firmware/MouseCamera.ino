# include "Wire.h"

#define SCLK_H PORTB |=(1<<PB1)
#define SCLK_L PORTB &=(~(1<<PB1))

#define SDIO_H PORTD |=(1<<PD4)
#define SDIO_L PORTD &=(~(1<<PD4))

void setup() 
{
  Serial.begin(115200);
  Serial.println("test run...");
  _delay_ms(200);
  ADNS2610_setup();

  // strob pin
  DDRD |= (1<<PD2);
}
//-------------------------------------------------------
void loop() 
{
      uint8_t tmp[324] = {0,};
      //PORTD |=(1<<PD2);
      ADNS2610_readFrame(&tmp[0]);
      
      //PORTD |=(1<<PD2);
      for(int i=0;i<324;i++) {
        Serial.write(tmp[i]);
      }
      //PORTD &=(~(1<<PD2));
      _delay_ms(100);
}

void ADNS2610_setup () {
    // SCLK - 9 - PB1
    DDRB |=(1<<PB1);

    // SDIO - 4 - PD4
    ADNS2610_sdioDirect(0);

    // wake up sensor
    SCLK_H; 
    _delay_us(5); 
    SCLK_L; 
    _delay_us(1); 
    SCLK_H; 
    _delay_ms(125); 
    ADNS2610_writeReg (0x00, 0x01); 
    ADNS2610_writeReg (0x01, 0x01); 
    _delay_ms(1000); 
    
}

// напаравление линии SDA 0 - от микроконтроллера, 1 - к микроконтроллеру
void ADNS2610_sdioDirect (uint8_t direction) {
    if(direction != 0) {
      DDRD &=(~(1<<PD4));
      return;
    }
    DDRD |= (1<<PD4);
}

void ADNS2610_writeReg (uint8_t address, uint8_t data) {
  address |= 0x80;
  ADNS2610_sdioDirect(0); //pinMode (SDIO, OUTPUT);
  for (uint8_t i = 128; i > 0 ; i >>= 1) {
    SCLK_L;
    _delay_us(10);
    if((address & i) != 0) SDIO_H; else SDIO_L;
    _delay_us(10);
    SCLK_H;
  }
  _delay_us(120); //delayMicroseconds(120);

  for (uint8_t i = 128; i > 0 ; i >>= 1) {
    SCLK_L;
    _delay_us(10);
    if((data & i) != 0) SDIO_H; else SDIO_L;
    _delay_us(10);
    SCLK_H;
  }
  _delay_us(100);
}

uint8_t ADNS2610_readReg (uint8_t address) {
  PORTD |=(1<<PD2);
  uint8_t data = 0;
  ADNS2610_sdioDirect(0); 
  for (uint8_t i = 128; i > 0 ; i >>= 1) {
    SCLK_L;
    asm("nop");
    if((address & i) != 0) SDIO_H; else SDIO_L;
    asm("nop");
    SCLK_H;
  }
  _delay_us(1);

  ADNS2610_sdioDirect(1);
  for (uint8_t i=128; i >0 ; i >>= 1) 
  {
    SCLK_L;
    asm("nop"); 
    SCLK_H;
    asm("nop"); 
    if((PIND & (1 << PD4)) != 0) data |= i;
  }
   _delay_us(1);

  PORTD &=(~(1<<PD2));
  return data;
}

void ADNS2610_readFrame (uint8_t* buff) {
  uint16_t pixelCounter = 0;
  uint8_t pixelValue;
  ADNS2610_writeReg(0x08, 0x2A);

  while(1) {
    pixelValue = ADNS2610_readReg (0x08);
    *buff = pixelValue & 0x3F;
    _delay_ms(1);
    if( !(pixelValue & 0x40) ) continue;


    buff++;
    pixelCounter ++;

    if( pixelCounter == 324 ) break;
  }
}

