#include "WORLD_IR_CODES.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <Adafruit_NeoPixel.h>

void xmitCodeElement(uint16_t ontime, uint16_t offtime, uint8_t PWM_code );
void quickflashLEDx( uint8_t x );
void delay_ten_us(uint16_t us);
void quickflashLED( void );
uint8_t read_bits(uint8_t count);
uint16_t rawData[300];


// Which pin on the Arduino is connected to the NeoPixels?
#define PIN 35 
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 1 

#define putstring_nl(s) Serial.println(s)
#define putstring(s) Serial.print(s)
#define putnum_ud(n) Serial.print(n, DEC)
#define putnum_uh(n) Serial.print(n, HEX)

#define MAX_WAIT_TIME 65535 //tens of us (ie: 655.350ms)

IRsend irsend(IRLED);  // Set the GPIO to be used to sending the message.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

extern const IrCode* const NApowerCodes[];
extern const IrCode* const EUpowerCodes[];
extern uint8_t num_NAcodes, num_EUcodes;


uint8_t bitsleft_r = 0;
uint8_t bits_r=0;
uint8_t code_ptr;
volatile const IrCode * powerCode;

// we cant read more than 8 bits at a time so dont try!
uint8_t read_bits(uint8_t count)
{
  uint8_t i;
  uint8_t tmp=0;

  // we need to read back count bytes
  for (i=0; i<count; i++) {
    // check if the 8-bit buffer we have has run out
    if (bitsleft_r == 0) {
      // in which case we read a new byte in
      bits_r = powerCode->codes[code_ptr++];
      DEBUGP(putstring("\n\rGet byte: ");
      putnum_uh(bits_r);
      );
      // and reset the buffer size (8 bites in a byte)
      bitsleft_r = 8;
    }
    // remove one bit
    bitsleft_r--;
    // and shift it off of the end of 'bits_r'
    tmp |= (((bits_r >> (bitsleft_r)) & 1) << (count-1-i));
  }
  // return the selected bits in the LSB part of tmp
  return tmp;
}


#define BUTTON_PRESSED LOW 
#define BUTTON_RELEASED HIGH

uint16_t ontime, offtime;
uint8_t i,num_codes;
uint8_t region;

void setup()   
{
  Serial.begin(115200);

  irsend.begin();
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

  pinMode(REGIONSWITCH, INPUT_PULLUP);
  pinMode(TRIGGER, INPUT_PULLUP);

  delay_ten_us(5000); //50ms (5000x10 us) delay: let everything settle for a bit

  // determine region
  if (digitalRead(REGIONSWITCH)) {
    region = NA;
    DEBUGP(putstring_nl("NA"));
  }
  else {
    region = EU;
    DEBUGP(putstring_nl("EU"));
  }

  // Debug output: indicate how big our database is
  DEBUGP(putstring("\n\rNA Codesize: ");
  putnum_ud(num_NAcodes);
  );
  DEBUGP(putstring("\n\rEU Codesize: ");
  putnum_ud(num_EUcodes);
  );

  // Tell the user what region we're in  - 3 flashes is NA, 6 is EU
  if (region == NA)
    quickflashLEDx(3);
  else //region == EU
    quickflashLEDx(6);
}

void sendAllCodes() 
{
  bool endingEarly = false; //will be set to true if the user presses the button during code-sending 
      
  // determine region from REGIONSWITCH: 1 = NA, 0 = EU (defined in main.h)
  if (digitalRead(REGIONSWITCH)) {
    region = NA;
    num_codes = num_NAcodes;
  }
  else {
    region = EU;
    num_codes = num_EUcodes;
  }

  // for every POWER code in our collection
  for (i=0 ; i<num_codes; i++) 
  {

    // print out the code # we are about to transmit
    DEBUGP(putstring("\n\r\n\rCode #: ");
    putnum_ud(i));

    // point to next POWER code, from the right database
    if (region == NA) {
      powerCode = NApowerCodes[i];
    }
    else {
      powerCode = EUpowerCodes[i];
    }
    
    // Read the carrier frequency from the first byte of code structure
    const uint8_t freq = powerCode->timer_val;
    // set OCR for Timer1 to output this POWER code's carrier frequency

    // Print out the frequency of the carrier and the PWM settings
    DEBUGP(putstring("\n\rFrequency: ");
    putnum_ud(freq);
    );
    
    DEBUGP(uint16_t x = (freq+1) * 2;
    putstring("\n\rFreq: ");
    putnum_ud(F_CPU/x);
    );

    // Get the number of pairs, the second byte from the code struct
    const uint8_t numpairs = powerCode->numpairs;
    DEBUGP(putstring("\n\rOn/off pairs: ");
    putnum_ud(numpairs));

    // Get the number of bits we use to index into the timer table
    // This is the third byte of the structure
    const uint8_t bitcompression = powerCode->bitcompression;
    DEBUGP(putstring("\n\rCompression: ");
    putnum_ud(bitcompression);
    putstring("\n\r"));

    // For EACH pair in this code....
    code_ptr = 0;
    for (uint8_t k=0; k<numpairs; k++) {
      uint16_t ti;

      // Read the next 'n' bits as indicated by the compression variable
      // The multiply by 4 because there are 2 timing numbers per pair
      // and each timing number is one word long, so 4 bytes total!
      ti = (read_bits(bitcompression)) * 2;

      // read the onTime and offTime from the program memory
      ontime = powerCode->times[ti];  // read word 1 - ontime
      offtime = powerCode->times[ti+1];  // read word 2 - offtime

      DEBUGP(putstring("\n\rti = ");
      putnum_ud(ti>>1);
      putstring("\tPair = ");
      putnum_ud(ontime));
      DEBUGP(putstring("\t");
      putnum_ud(offtime));      

      rawData[k*2] = ontime * 10;
      rawData[(k*2)+1] = offtime * 10;
      yield();
    }

    // Send Code with library
    irsend.sendRaw(rawData, (numpairs*2) , freq);
    Serial.print("\n");
    yield();
    //Flush remaining bits, so that next code starts
    //with a fresh set of 8 bits.
    bitsleft_r=0;

    // visible indication that a code has been output.
    quickflashLED();
    
    // delay 205 milliseconds before transmitting next POWER code
    delay_ten_us(20500);

    // if user is pushing (holding down) TRIGGER button, stop transmission early 
    if (digitalRead(TRIGGER) == BUTTON_PRESSED) 
    {
      while (digitalRead(TRIGGER) == BUTTON_PRESSED){
        yield();
      }
      endingEarly = true;
      delay_ten_us(50000); //500ms delay 
      quickflashLEDx(4);
      //pause for ~1.3 sec to give the user time to release the button so that the code sequence won't immediately start again.
      delay_ten_us(MAX_WAIT_TIME); // wait 655.350ms
      delay_ten_us(MAX_WAIT_TIME); // wait 655.350ms
      break; //exit the POWER code "for" loop
    }
    
  } //end of POWER code for loop

  if (endingEarly==false)
  {
    //pause for ~1.3 sec, then flash the visible LED 8 times to indicate that we're done
    delay_ten_us(MAX_WAIT_TIME); // wait 655.350ms
    delay_ten_us(MAX_WAIT_TIME); // wait 655.350ms
    quickflashLEDx(8);
  }

} //end of sendAllCodes

void loop() 
{
  //Super "ghetto" (but decent enough for this application) button debouncing:
  //-if the user pushes the Trigger button, then wait a while to let the button stop bouncing, then start transmission of all POWER codes
  if (digitalRead(TRIGGER) == BUTTON_PRESSED) 
  {
    delay_ten_us(40000);
    while (digitalRead(TRIGGER) == BUTTON_PRESSED){
      delay_ten_us(500);
      yield();
    }
    sendAllCodes();
  }
  yield();
}


void delay_ten_us(uint16_t us) {
  uint8_t timer;
  while (us != 0) {
    // for 8MHz we want to delay 80 cycles per 10 microseconds
    // this code is tweaked to give about that amount.
    for (timer=0; timer <= DELAY_CNT; timer++) {
      NOP;
      NOP;
    }
    NOP;
    us--;
  }
}


// This function quickly pulses the visible LED 
// This will indicate to the user that a code is being transmitted
void quickflashLED( void ) {
  pixels.setPixelColor(0, pixels.Color(0, 150, 0));
  pixels.show();
  delay_ten_us(3000);   // 30 ms ON-time delay
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();
}

// This function just flashes the visible LED a couple times, used to
// tell the user what region is selected
void quickflashLEDx( uint8_t x ) {
  quickflashLED();
  while(--x) {
    delay_ten_us(25000);     // 250 ms OFF-time delay between flashes
    quickflashLED();
  }
}

