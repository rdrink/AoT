// BOF preprocessor bug prevent - insert me on top of your arduino-code
// From: http://www.a-control.de/arduino-fehler/?lang=en
// From: http://subethasoftware.com/2013/04/09/arduino-compiler-problem-with-ifdefs-solved/
// From: http://forum.arduino.cc/index.php?PHPSESSID=8395001def82bee5b565614dc4f2d89d&topic=158885.0

#define nop() __asm volatile ("nop")
#if 1
nop();
#endif
/*
* EOF preprocessor bug prevent
*/

/*
 Analog Input with prescale change
 Reading a 1 kHz sine wave, 0 to 5 volts
 Using analog 0
 Results stored in memory for highest speed
 using code from:
 http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1208715493/11
 with special thanks to jmknapp
 */

#define FASTADC 1
// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))  
#endif

#include <OneWire.h> 
#include <dht.h>
#include <Wire.h> 
//I2C library
#include <HIH61XX.h>
#include <LibTempTMP421.h>
#include <DallasTemperature.h>
// #include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>

#define debug_serial 1



#define SHT75_ADD 1
#define BMP180_ADD 2
#define RHT03_ADD 3
#define TMP102_ADD 4
#define SHT15_ADD_1 6
#define SHT15_ADD_2 66
#define MMA8452 5
#define HIH6130_ADD 7
#define DS18B20_ADD 8
#define MLX90614_ADD 9
#define TMP421_ADD 13
// #define IR_D6T_44L_06_ADD 14 

#define GA1A1S201WP_ADD 10
#define THERMIS_100K_ADD 11
#define MAX4466_ADD 12
#define HIH4030_ADD 15


// #define PhoRes_ADD 16








#ifdef IR_D6T_44L_06_ADD
#include <i2cmaster.h>
#define I2C_ADD 1
int rbuf[35];
float tdata[16];
float t_PTAT;
float tPEC;

bool D6T_get_data(void) {
  int k;
  i2c_start_wait(0x14);
  i2c_write(0x4C);
  
  i2c_rep_start(0x15);
  for (k = 0; k < 35; k++) {
    if (k == 34) {
      rbuf[k] = i2c_readNak();
    }
    else {
      rbuf[k] = i2c_readAck();
    }
  }
  i2c_stop();
   
  if (!D6T_checkPEC(rbuf, 34)) {
    return false;
  }
  else {
    t_PTAT = (rbuf[0]+(rbuf[1]<<8))*0.1;
    for (k = 0; k < 16; k++) {
      tdata[k]=(rbuf[(k*2+2)]+(rbuf[(k*2+3)]<<8))*0.1;
    }
    tPEC = rbuf[34];
    return true;
  }
}

void output_csv() {
  Serial.print(t_PTAT);
  Serial.print(",");
  for (int i = 0; i < 16; i++) {
    Serial.print(tdata[i]);
    if (i < 15) {
      Serial.print(",");
    }
  }
}

int D6T_checkPEC(int* buf, int pPEC) {
  byte crc;
  
  crc = calc_crc(0x14);
  crc = calc_crc(0x4C ^ crc);
  crc = calc_crc(0x15 ^ crc);
  for (int i=0; i<pPEC; i++) {
    crc = calc_crc(buf[i] ^ crc);
  }
  return (crc == buf[pPEC]);
}

byte calc_crc(byte data) {
  for (int index=0; index<8; index++) {
    byte temp = data;
    data = data<<1;
    if (temp & 0x80) data = data ^ 0x07;
  }
  return data;
}
#endif

#ifdef MMA8452
#ifndef I2C_ADD
#include <i2cmaster.h>
#define I2C_ADD 1
#endif //I2C_ADD
#define MMA8452_ADDRESS_WRITE 0x3A
#define MMA8452_ADDRESS_READ 0x3B
#define OUT_X_MSB 0x01
#define XYZ_DATA_CFG 0x0E
#define WHO_AM_I 0x0D
#define CTRL_REG1 0x2A
#define GSCALE 2
#define accel_samples 10

float xmean, ymean, zmean, rt_mean_sq;

void MMA8452_get_means(float *xmean, float *ymean, float *zmean, float *rt_mean_sq)
{
  int n = 0;
  float xvals[accel_samples], yvals[accel_samples], zvals[accel_samples];
  float xtotal = 0, ytotal = 0, ztotal = 0, xvar = 0, yvar = 0, zvar = 0, mean_square_var;

  // Serial.println("Entering while loop"); // DEBUG

  while (n < accel_samples){
    delay(10);
    int accelCount[3]; // Stores the 12-bit signed value

    // Serial.println("About to try reading data..."); // DEBUG
    readAccelData(accelCount); // Read the x/y/z adc values

    float accelG[3]; // Stores the real accel value in g's
    for (int i = 0 ; i < 3 ; i++)
    {
      accelG[i] = (float) accelCount[i] / ((1<<12)/(2*GSCALE)); // get actual g value, this depends on scale being set
    }

    // Serial.println("Acquired data"); // DEBUG

    xvals[n] = accelG[0];
    xtotal += accelG[0];
    yvals[n] = accelG[1];
    ytotal += accelG[1];
    zvals[n] = accelG[2];
    ztotal += accelG[2];
    n++;
  }

  // Serial.println("Means being calculated..."); // DEBUG

  *xmean = xtotal/accel_samples;
  *ymean = ytotal/accel_samples;
  *zmean = ztotal/accel_samples;

  for (int j = 0; j < accel_samples; j++)
  {
    xvar += sq(xvals[j] - *xmean);
    yvar += sq(yvals[j] - *ymean);
    zvar += sq(zvals[j] - *zmean);
  }

  mean_square_var = (sq(xvar) + sq(yvar) + sq(zvar))/3;
  *rt_mean_sq = sqrt(mean_square_var);
}

void readAccelData(int *destination)
{
  byte rawData[6]; // x/y/z accel register data stored here

  // Serial.println("Problem potential ahead"); // DEBUG
  readRegisters(OUT_X_MSB, 6, rawData); // Read the six raw data registers into data array

  // Loop to calculate 12-bit ADC and g value for each axis
  for(int i = 0; i < 3 ; i++)
  {
    int gCount = (rawData[i*2] << 8) | rawData[(i*2)+1]; //Combine the two 8 bit registers into one 12-bit number
    gCount >>= 4; //The registers are left align, here we right align the 12-bit integer
    
    // If the number is negative, we have to make it so manually (no 12-bit data type)
    if (rawData[i*2] > 0x7F)
    {
      gCount = ~gCount + 1;
      gCount *= -1; // Transform into negative 2's complement #
    }

    destination[i] = gCount; //Record this gCount into the 3 int array
  }
}

void initMMA8452()
{
  byte c = readRegister(WHO_AM_I); // Read WHO_AM_I register
  if (c == 0x2A) ; // WHO_AM_I should always be 0x2A
  else
    Serial.print("Error with MMA8452");

  MMA8452Standby(); // Must be in standby to change registers

  byte fsr = GSCALE;
  if(fsr > 8) fsr = 8; //Easy error check
  fsr >>= 2;
  writeRegister(XYZ_DATA_CFG, fsr);

  MMA8452Active(); // Set to active to start reading
}

void MMA8452Standby()
{
  byte c = readRegister(CTRL_REG1);
  writeRegister(CTRL_REG1, c & ~(0x01)); //Clear the active bit to go into standby
}

void MMA8452Active()
{
  byte c = readRegister(CTRL_REG1);
  writeRegister(CTRL_REG1, c | 0x01); //Set the active bit to begin detection
}

void readRegisters(byte addressToRead, int bytesToRead, byte * dest)
{
  i2c_start(MMA8452_ADDRESS_WRITE);
  i2c_write(addressToRead);
  i2c_rep_start(MMA8452_ADDRESS_READ);

  for(int x = 0 ; x < bytesToRead ; x++)
    if (x == bytesToRead - 1)
      dest[x] = i2c_readNak();
    else
      dest[x] = i2c_readAck();
}

byte readRegister(byte addressToRead)
{
  i2c_start(MMA8452_ADDRESS_WRITE); // Write address
  i2c_write(addressToRead);
  i2c_rep_start(MMA8452_ADDRESS_READ); // Read address

  return i2c_readNak();
}

void writeRegister(byte addressToWrite, byte dataToWrite)
{
  i2c_start(MMA8452_ADDRESS_WRITE);
  i2c_write(addressToWrite);
  i2c_write(dataToWrite);
  i2c_stop();
}
#endif

#ifdef HIH4030_ADD
#define HIH4030_PIN A3
#endif

#ifdef MLX90614_ADD
#ifndef I2C_ADD
#include <i2cmaster.h>
#endif //I2C_ADD
#endif //MLX90614_ADD

#ifdef PhoRes_ADD
#define PhoRes_PIN 12
#endif //PhoRes_ADD

#ifdef SHT15_ADD_1
#include <Sensirion.h>
#define Sensirion_ADD 1
#define SHT15_1_dataPin  2     // SHT15_1 serial data
#define SHT15_1_sclkPin  3     // SHT15_1 serial clock
float SHT15_1_temperature;
float SHT15_1_humidity;
Sensirion SHT15_1_tempSensor = Sensirion(SHT15_1_dataPin, SHT15_1_sclkPin);
#endif 

#ifdef SHT15_ADD_2
#include <Sensirion.h>
#define Sensirion_ADD 1
#define SHT15_2_dataPin  9     // SHT15_2 serial data
#define SHT15_2_sclkPin  8     // SHT15_2 serial clock
float SHT15_2_temperature;
float SHT15_2_humidity;
Sensirion SHT15_2_tempSensor = Sensirion(SHT15_2_dataPin, SHT15_2_sclkPin);
#endif

#ifdef SHT75_ADD
#ifndef Sensirion_ADD
#include <Sensirion.h>
#endif //Sensirion_ADD
#define SHT75_1_dataPin 6  
#define SHT75_1_sclkPin 7   
float SHT75_1_temperature;
float SHT75_1_humidity;
Sensirion SHT75_1_tempSensor = Sensirion(SHT75_1_dataPin, SHT75_1_sclkPin);
#endif //SHT75_ADD

#ifdef GA1A1S201WP_ADD
#define AMBI_1_Pin A0    // Ambient sensor
#endif //GA1A1S201WP_ADD

#ifdef MAX4466_ADD
#define MAX4466_PIN A2
const int MAX4466_Buff_Len = 50;
int MAX4466_Val[MAX4466_Buff_Len];   // variable to store the value coming from the sensor
#endif //MAX4466_ADD

#ifdef THERMIS_100K_ADD
#define THERMIS_100K_Pin A1    // NTC sensor
#endif

#ifdef DS18B20_ADD
#define DS18B20_1_Pin 4
OneWire DS18B20_1_ds(DS18B20_1_Pin);  // Temperature chip i/o on digital pin 2

float DS18B20_1_getTemp()
{
  //returns the temperature from one DS18B20 in DEG Celsius
  
  byte data[12];
  byte addr[8];
  
  if (!DS18B20_1_ds.search(addr)) {
    //no more sensors on chain, reset search
    DS18B20_1_ds.reset_search();
    return -1000;
  }
  
  if ( OneWire::crc8( addr, 7) != addr[7]) {
    //Serial.println("CRC is not valid!");
    return -1000;
  }
  
  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    //Serial.print("Device is not recognized");
    return -1000;
  }
  
  DS18B20_1_ds.reset();
  DS18B20_1_ds.select(addr);
  DS18B20_1_ds.write(0x44,1); // start conversion, with parasite power on at the end
  
  byte present = DS18B20_1_ds.reset();
  DS18B20_1_ds.select(addr);    
  DS18B20_1_ds.write(0xBE); // Read Scratchpad
      
  for (int i = 0; i < 9; i++) { // we need 9 bytes
  data[i] = DS18B20_1_ds.read();
  }
  
  DS18B20_1_ds.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];
  
  //float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum;
  TemperatureSum = tempRead/16.0;
  return TemperatureSum;
}
#endif //DS18B20_ADD

#ifdef RHT03_ADD
#define RHT03_1_PIN 5 // RHT03 Sensor pin
dht RHT03_1;
#endif //RHT03_ADD

#ifdef TMP102_ADD
const int tmp102_1_Address = 0x48;
#endif //TMP421_ADD

#ifdef HIH6130_ADD
//  Create an HIH61XX with I2C address 0x27, powered by pin 22, need to modify this
HIH61XX HIH_6130_1_hih(0x27,22);
#endif //HIH6130_ADD

#ifdef TMP421_ADD
LibTempTMP421 TMP421_1 = LibTempTMP421(0);
#endif //TMP421_ADD

#ifdef BMP180_ADD
Adafruit_BMP085_Unified BMP_180_1 = Adafruit_BMP085_Unified(10085);
#endif //BMP180_ADD

void setup()
{
  Serial.begin(115200);
  #ifdef debug_serial
  Serial.print("Starting Up.");
  #endif
  
  Wire.begin();

  #ifdef TMP421_ADD
  /************ The LibTemp421 library turns on Arduino pins A2, A3 (aka 16, 17) to power the sensor.
  *  This is necessary due to the fact that Wire.begin() is called in the constructor and needs to be 
  *  talk to the chip, which needs to be powered. If you are using the sensor in a differnt location
  *  and powering it from dedicated GND and +5V lines then include the lines below to reset the 
  *  A2 & A3 pins for use as analog inputs. */
  
  //  Uncomment the three lines below to reset the analog pins A2 & A3
  pinMode(A2, INPUT);        // GND pin      
  pinMode(A3, INPUT);        // VCC pin
  digitalWrite(A3, LOW);     // turn off pullups   
  #endif //TMP421_ADD
  
  #ifdef BMP180_ADD
  BMP_180_1.begin();
  #endif
  // set the resolution to 10 bit (good enough?)
      
  delay(3000); // waiting for the sensors to settle
  
  #ifdef debug_serial
  Serial.println("...entering Sensor Loop...");
  #endif //debug_serial  
  
  #ifdef MLX90614_ADD
  i2c_init(); //Initialise the i2c bus
  #define I2C_INIT_ADD 1
  //PORTC = (1 << PORTC4) | (1 << PORTC5);//enable pullups
  #endif //MLX90614_ADD
  
  #ifdef IR_D6T_44L_06_ADD
  #ifndef I2C_INIT_ADD
  i2c_init();
  #define I2C_INIT_ADD 1
  #endif //I2C_INIT_ADD
  pinMode(17,OUTPUT);    // Power (VCC) for setting D17 (A3) pin 
  digitalWrite(17,HIGH);
  pinMode(16,OUTPUT);    // Power supply (GND) for setting D16 (A2) pin 
  digitalWrite(16,LOW);
  pinMode(A4,INPUT);
  digitalWrite(A4,HIGH); // Pull-up resistor for SDA
  pinMode(A5,INPUT);
  digitalWrite(A5,HIGH); // Pull-up resistor for SCL
  #endif //IR_D6T_44L_06_ADD

  #ifdef MMA8452
  #ifndef I2C_INIT_ADD
  i2c_init();
  #endif //I2C_INIT_ADD
  initMMA8452();
  #endif

  #if FASTADC
  // set prescale to 16
  sbi(ADCSRA,ADPS2) ;
  cbi(ADCSRA,ADPS1) ;
  cbi(ADCSRA,ADPS0) ;
  #endif
}

void loop()
{
  #ifdef IR_D6T_44L_06_ADD
  D6T_get_data();
  Serial.print("D6T_44L_06_1_T_C:");
  output_csv();
  Serial.print("; ");
  #endif

  #ifdef MMA8452
  // Serial.println("Trying to get MMA values"); // DEBUG
  MMA8452_get_means(&xmean, &ymean, &zmean, &rt_mean_sq);
  // Serial.println("Got MMA Values!"); // DEBUG
  Serial.print("MMA8452_1_A_X_Units:");
  Serial.print(xmean);
  Serial.print("; ");
  Serial.print("MMA8452_1_A_Y_Units:");
  Serial.print(ymean);
  Serial.print("; ");
  Serial.print("MMA8452_1_A_Z_Units:");
  Serial.print(zmean);
  Serial.print("; ");
  Serial.print("MMA8452_1_A_RMS_Units:");
  Serial.print(rt_mean_sq);
  Serial.print("; ");
  #endif

  #ifdef SHT15_ADD_1
  uint16_t SHT15_1_rawData;
  uint8_t SHT15_1_error; 
  SHT15_1_error = SHT15_1_tempSensor.measTemp(&SHT15_1_rawData);
  SHT15_1_temperature = SHT15_1_tempSensor.calcTemp(SHT15_1_rawData);
  SHT15_1_error = SHT15_1_tempSensor.measHumi(&SHT15_1_rawData);
  SHT15_1_humidity = SHT15_1_tempSensor.calcHumi(SHT15_1_rawData, SHT15_1_temperature);
  Serial.print("SHT15_1_T_C:");
  Serial.print(SHT15_1_temperature,2);
  Serial.print("; ");
  Serial.print("SHT15_1_H_%:");
  Serial.print(SHT15_1_humidity,2);
  Serial.print("; ");
  #endif //SHT15_ADD
  
  #ifdef SHT15_ADD_2
  uint16_t SHT15_2_rawData;
  uint8_t SHT15_2_error; 
  SHT15_2_error = SHT15_2_tempSensor.measTemp(&SHT15_2_rawData);
  SHT15_2_temperature = SHT15_2_tempSensor.calcTemp(SHT15_2_rawData);
  SHT15_2_error = SHT15_2_tempSensor.measHumi(&SHT15_2_rawData);
  SHT15_2_humidity = SHT15_2_tempSensor.calcHumi(SHT15_2_rawData, SHT15_2_temperature);
  Serial.print("SHT15_2_T_C:");
  Serial.print(SHT15_2_temperature,2);
  Serial.print("; ");
  Serial.print("SHT15_2_H_%:");
  Serial.print(SHT15_2_humidity,2);
  Serial.print("; ");
  #endif //SHT15_ADD
  
  #ifdef SHT75_ADD
  uint16_t SHT75_1_rawData;
  uint8_t SHT75_1_error;
  SHT75_1_error = SHT75_1_tempSensor.measTemp(&SHT75_1_rawData);
  SHT75_1_temperature = SHT75_1_tempSensor.calcTemp(SHT75_1_rawData);
  SHT75_1_error = SHT75_1_tempSensor.measHumi(&SHT75_1_rawData);
  SHT75_1_humidity = SHT75_1_tempSensor.calcHumi(SHT75_1_rawData, SHT75_1_temperature);
  
  Serial.print("SHT75_1_T_C:");
  Serial.print(SHT75_1_temperature,2);
  Serial.print("; ");
  Serial.print("SHT75_1_H_%:");
  Serial.print(SHT75_1_humidity,2);
  Serial.print("; ");
  #endif //SHT75_ADD
  
  #ifdef MAX4466_ADD
  int i=0;
  int MAX4466_Peak;
  
  for (i=0;i<MAX4466_Buff_Len;i++)
  {
    MAX4466_Val[i]= analogRead(MAX4466_PIN);
  } 

  // we have read sample_size values, let us now find the peak of the 100 values. 
  MAX4466_Peak = 0;
  
  for (i=0;i<MAX4466_Buff_Len;i++)
  {
    MAX4466_Peak = max(MAX4466_Peak , abs(512-MAX4466_Val[i]));
  }
  Serial.print("MAX4466_1_Units:");
  Serial.print(MAX4466_Peak);
  Serial.print("; ");
  #endif //MAX4466_ADD
  
  #ifdef GA1A1S201WP_ADD
  int AMBI_1_Value;
  AMBI_1_Value = analogRead(AMBI_1_Pin); 
  Serial.print("AMBI_1_Units:");
  Serial.print(AMBI_1_Value);
  Serial.print("; ");
  #endif //GA1A1S201WP_ADD
  
  #ifdef PhoRes_ADD
  int PhoRes_1_Value;
  PhoRes_1_Value = analogRead(PhoRes_PIN); 
  Serial.print("PhoRes_10K4.7K_Units:");
  Serial.print(PhoRes_1_Value);
  Serial.print("; ");
  #endif //PhoRes_ADD
  
  #ifdef HIH4030_ADD
  int HIH4030_1_Value;
  HIH4030_1_Value = analogRead(HIH4030_PIN); 
  Serial.print("HIH4030_Units:");
  Serial.print(HIH4030_1_Value);
  Serial.print("; ");
  #endif //HIH4030_ADD
  
  #ifdef THERMIS_100K_ADD
  int THERMIS_1_Value;
  THERMIS_1_Value = analogRead(THERMIS_100K_Pin); 
  Serial.print("THERMIS_100K_Units:");
  Serial.print(THERMIS_1_Value);
  Serial.print("; ");
  #endif //THERMIS_100K_ADD
  
  #ifdef DS18B20_ADD
  float DS18B20_1_temperature;
  DS18B20_1_temperature = DS18B20_1_getTemp();
  Serial.print("DS18B20_1_T_C:");
  Serial.print(DS18B20_1_temperature, 2);
  Serial.print("; ");
  #endif
      
  #ifdef TMP421_ADD
  float TMP421_1_temperature;
  TMP421_1_temperature = TMP421_1.GetTemperature(); // Some error, check this out! 
  Serial.print("TMP421_1_T_C:");
  Serial.print(TMP421_1_temperature, 2);
  Serial.print("; ");
  #endif
      
  #ifdef RHT03_ADD
  int chk_RHT03_1;
  chk_RHT03_1 = RHT03_1.read22(RHT03_1_PIN);
  if (chk_RHT03_1 == DHTLIB_OK)
  { 
    Serial.print("RHT03_1_T_C:");
    Serial.print(RHT03_1.temperature, 2);
    Serial.print("; ");
    Serial.print("RHT03_1_H_%:");
    Serial.print(RHT03_1.humidity, 2);
    Serial.print("; ");
  }
  #endif
  
  #ifdef BMP180_ADD
  float BMP_180_1_temperature, BMP_180_1_pressure;
  BMP_180_1.getPressure(&BMP_180_1_pressure);  
  BMP_180_1.getTemperature(&BMP_180_1_temperature);
  Serial.print("BMP_180_1_T_C:");
  Serial.print(BMP_180_1_temperature, 2);
  Serial.print("; ");
  Serial.print("BMP_180_1_P_PA:");
  Serial.print(BMP_180_1_pressure, 2);
  Serial.print("; ");
  #endif
  
  #ifdef TMP102_ADD
  float tmp102_1_temperature;
  Wire.requestFrom(tmp102_1_Address,2);
  byte tmp102_1_MSB = Wire.read();
  byte tmp102_1_LSB = Wire.read();
  tmp102_1_temperature = (((tmp102_1_MSB << 8) | tmp102_1_LSB) >> 4) * 0.11250 + 32; //it's a 12bit int, using two's compliment for negative
  Serial.print("TMP102_1_T_F:");
  Serial.print(tmp102_1_temperature,2);
  Serial.print("; ");
  #endif

  #ifdef  HIH6130_ADD
  HIH_6130_1_hih.start();
  HIH_6130_1_hih.update();
  Serial.print("HIH_6130_1_T_C:");
  Serial.print(HIH_6130_1_hih.temperature(), 2);
  Serial.print("; ");
  Serial.print("HIH_6130_1_H_%:");
  Serial.print(HIH_6130_1_hih.humidity(), 2);
  Serial.print("; ");
  #endif
  
  #ifdef MLX90614_ADD
  int dev = 0x5A<<1;
  int data_low = 0;
  int data_high = 0;
  int pec = 0;
  
  i2c_start_wait(dev+I2C_WRITE);
  i2c_write(0x07);
  
  // read
  i2c_rep_start(dev+I2C_READ);
  data_low = i2c_readAck(); //Read 1 byte and then send ack
  data_high = i2c_readAck(); //Read 1 byte and then send ack
  pec = i2c_readNak();
  i2c_stop();
  
  //This converts high and low bytes together and processes temperature, MSB is a error bit and is ignored for temps
  double tempFactor = 0.02; // 0.02 degrees per LSB (measurement resolution of the MLX90614)
  double tempData = 0x0000; // zero out the data
  int frac; // data past the decimal point
  
  // This masks off the error bit of the high byte, then moves it left 8 bits and adds the low byte.
  tempData = (double)(((data_high & 0x007F) << 8) + data_low);
  tempData = (tempData * tempFactor)-0.01;
  
  float celcius = tempData - 273.15;
  float fahrenheit = (celcius*1.8) + 32;
  
  Serial.print("MLX90614_T_F:");
  Serial.print(fahrenheit);
  Serial.print("; ");
  #endif
  
  Serial.print('\n');
  delay(1000);
}