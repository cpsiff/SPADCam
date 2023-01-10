/*
 *****************************************************************************
 * Copyright by ams OSRAM AG                                                 *
 * All rights are reserved.                                                  *
 *                                                                           *
 * IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
 * THE SOFTWARE.                                                             *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS         *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
 *****************************************************************************
 */
//
// tmf8820/1 arduino uno driver example program
//

// ---------------------------------------------- includes ----------------------------------------

#include <Wire.h>
#include "registers_i2c.h"
#include "tmf882x.h"
#include "tmf882x_calib.h"


// ---------------------------------------------- defines -----------------------------------------

#define UART_BAUD_RATE              115200
#define VERSION                     17

// arduino uno can only do 400 KHz I2C
#define I2C_CLK_SPEED               400000


// how much logging we want - info prints the FW number
#define MY_LOG_LEVEL                LOG_LEVEL_INFO

// tmf states
#define TMF882X_STATE_DISABLED      0
#define TMF882X_STATE_STANDBY       1     
#define TMF882X_STATE_STOPPED       2
#define TMF882X_STATE_MEASURE       3
#define TMF882X_STATE_ERROR         4

// convendience macro to have a pointer to the driver structure
#define TMF882X_A                   ( &tmf882x )

// ---------------------------------------------- constants -----------------------------------------

// for each configuration specifiy a period in milli-seconds
const uint16_t configPeriod[3] = {33, 500, 100};

// for each configuration specify the number of Kilo Iterations (Kilo = 1024)
const uint16_t configKiloIter[3] = {537, 550, 900};

// for each configuration select a SPAD map through the id
const uint8_t  configSpadId[3] = {TMF882X_COM_SPAD_MAP_ID__spad_map_id__map_no_1, TMF882X_COM_SPAD_MAP_ID__spad_map_id__map_no_2, TMF882X_COM_SPAD_MAP_ID__spad_map_id__map_no_7};


// ---------------------------------------------- variables -----------------------------------------

tmf882xDriver tmf882x;            // instance of tmf882x
extern uint8_t logLevel;          // for i2c logging in shim 
int8_t stateTmf882x;              // current state of the device 
int8_t configNr;                  // this sample application has only a few configurations it will loop through, the variable keeps track of that
int8_t clkCorrectionOn;           // if non-zero clock correction is on
int8_t dumpHistogramOn;           // if non-zero, dump all histograms
// ---------------------------------------------- function declaration ------------------------------

// Switch I2C address.
void changeI2CAddress( );

// Select one of the available configurations and configure the device accordingly.
void configure( );

// Function to print the content of these registers, len should be a multiple of 8
void printRegisters( uint8_t regAddr, uint16_t len, char seperator );

// Print the current state (stateTmf882x) in a readable format
void printState( );

// Function prints a help screen
void printHelp( );

// Function checks the UART for received characters and interprets them
void serialInput( );

// Arduino setup function is only called once at startup. Do all the HW initialisation stuff here.
void setup( );

// Arduino main loop function, is executed cyclic
void loop( );


// ---------------------------------------------- functions -----------------------------------------

// Switch I2C address.
void changeI2CAddress ( )
{
  uint8_t newAddr = TMF882X_A->i2cSlaveAddress;
  if ( newAddr == TMF882X_SLAVE_ADDR )
  {
    newAddr = TMF882X_SLAVE_ADDR + 1;      // use next i2c slave address
  }
  else
  {
    newAddr = TMF882X_SLAVE_ADDR;         // back to original
  }
  if ( tmf882xChangeI2CAddress( TMF882X_A, newAddr ) != APP_SUCCESS_OK )
  {
    PRINT_STR( "ERR " );
  }
  PRINT_STR( "I2C Addr=" );
  PRINT_INT( TMF882X_A->i2cSlaveAddress );
  PRINT_LN( );
}

// wrap through the available configurations and configure the device accordingly.
void configure ( )
{
  configNr = configNr + 1;
  if ( configNr > 2 )
  {
    configNr = 0;     // wrap around
  }

  if ( tmf882xConfigure( TMF882X_A, configPeriod[configNr], configKiloIter[configNr], configSpadId[configNr], dumpHistogramOn ) == APP_SUCCESS_OK )
  {
    PRINT_STR( "Conf Period=" );
    PRINT_INT( configPeriod[configNr] );
    PRINT_STR( "ms KIter=" );
    PRINT_INT( configKiloIter[configNr] );
    PRINT_STR( " SPAD=" );
    PRINT_INT( configSpadId[configNr] );
  }
  else
  {
    PRINT_STR( "Error Config" );
  }
  PRINT_LN( );
}

// function will return the factory calibration matching the selected SPAD config
static const uint8_t * getPrecollectedFactoryCalibration ( )
{
  const uint8_t * factory_calib = tmf882x_calib_0;
  if ( configNr == 1) 
  {
    factory_calib = tmf882x_calib_1;
  }
  else if ( configNr == 2 )
  {
    factory_calib = tmf882x_calib_2;
  }
  return factory_calib;
}


// Print the current state (stateTmf882x) in a readable format
void printState ( )
{
  PRINT_STR( "state=" );
  switch ( stateTmf882x )
  {
    case TMF882X_STATE_DISABLED: PRINT_STR( "disabled" ); break;
    case TMF882X_STATE_STANDBY: PRINT_STR( "standby" ); break;
    case TMF882X_STATE_STOPPED: PRINT_STR( "stopped" ); break;
    case TMF882X_STATE_MEASURE: PRINT_STR( "measure" ); break;
    case TMF882X_STATE_ERROR: PRINT_STR( "error" ); break;   
    default: PRINT_STR( "???" ); break;
  }
  PRINT_LN( );
}

// print registers either as c-struct or plain
void printRegisters ( uint8_t regAddr, uint16_t len, char seperator )
{
  uint8_t buf[8];
  uint16_t i;
  if ( seperator == ',' )
  {
    PRINT_STR( "const PROGMEM uint8_t tmf8828_calib_" );
    PRINT_INT( configNr );
    PRINT_STR( "[] = {" );
    PRINT_LN( );
  }
  for ( i = 0; i < len; i += 8 )            // if len is not a multiple of 8, we will print a bit more registers ....
  {
    uint8_t * ptr = buf;    
    i2c_rx( TMF882X_A->i2cSlaveAddress, regAddr, buf, 8 );
    if ( seperator == ' ' )
    {
      PRINT_STR( "0x" );
      PRINT_INT_HEX( regAddr );
      PRINT_STR( ": " );
    }
    PRINT_STR( " 0x" ); PRINT_INT_HEX( *ptr++ ); PRINT_CHAR( seperator ); 
    PRINT_STR( " 0x" ); PRINT_INT_HEX( *ptr++ ); PRINT_CHAR( seperator ); 
    PRINT_STR( " 0x" ); PRINT_INT_HEX( *ptr++ ); PRINT_CHAR( seperator ); 
    PRINT_STR( " 0x" ); PRINT_INT_HEX( *ptr++ ); PRINT_CHAR( seperator ); 
    PRINT_STR( " 0x" ); PRINT_INT_HEX( *ptr++ ); PRINT_CHAR( seperator ); 
    PRINT_STR( " 0x" ); PRINT_INT_HEX( *ptr++ ); PRINT_CHAR( seperator ); 
    PRINT_STR( " 0x" ); PRINT_INT_HEX( *ptr++ ); PRINT_CHAR( seperator ); 
    PRINT_STR( " 0x" ); PRINT_INT_HEX( *ptr++ ); PRINT_CHAR( seperator ); 
    PRINT_LN( );
    regAddr = regAddr + 8;
  }
  if ( seperator == ',' )
  {
    PRINT_STR( "};" );
    PRINT_LN( );
  }
}



// -------------------------------------------------------------------------------------------------------------

// Function prints a help screen
void printHelp ( )
{
  PRINT_STR( "TMF882x Arduino Version=" );
  PRINT_INT( VERSION ); 
  PRINT_LN( ); PRINT_STR( "UART commands" );
  PRINT_LN( ); PRINT_STR( "h ... print help " );
  PRINT_LN( ); PRINT_STR( "e ... enable device and download FW" );
  PRINT_LN( ); PRINT_STR( "d ... disable device" );
  PRINT_LN( ); PRINT_STR( "w ... wakeup" );
  PRINT_LN( ); PRINT_STR( "p ... power down" );
  PRINT_LN( ); PRINT_STR( "m ... start measure" );
  PRINT_LN( ); PRINT_STR( "s ... stop measure" );
  PRINT_LN( ); PRINT_STR( "c ... use next conf." );
  PRINT_LN( ); PRINT_STR( "f ... do fact. calib" );
  PRINT_LN( ); PRINT_STR( "l ... load fact. calib. page" );
  PRINT_LN( ); PRINT_STR( "r ... restore fact. calib. from file" );  
  PRINT_LN( ); PRINT_STR( "x ... clock corr. on/off" );
  PRINT_LN( ); PRINT_STR( "a ... dump registers" );
  PRINT_LN( ); PRINT_STR( "z ... histogram" );
  PRINT_LN( ); PRINT_STR( "i ... i2c addr. change" );
  PRINT_LN( ); 
}

// Function checks the UART for received characters and interprets them
void serialInput ( )
{
  while ( Serial.available() )
  {
    char rx = Serial.read();
    if ( rx < 33 || rx >=126 ) // skip all control characters and DEL  
    {
      continue; // nothing to do here
    }
    else
    { 
      if ( rx == 'h' )
      {
          printHelp(); 
      }
      else if ( rx == 'c' ) // show and use next configuration
      {
        if ( stateTmf882x == TMF882X_STATE_STOPPED )
        {
          configure( );
        }
      }
      else if ( rx == 'e' ) // enable
      {  
        if ( stateTmf882x == TMF882X_STATE_DISABLED )
        {
          tmf882xEnable( TMF882X_A );
          delay_in_microseconds( ENABLE_TIME_MS * 1000 );
          tmf882xClkCorrection( TMF882X_A, clkCorrectionOn ); 
          tmf882xSetLogLevel( TMF882X_A, MY_LOG_LEVEL );
          tmf882xWakeup( TMF882X_A );
          if ( tmf882xIsCpuReady( TMF882X_A, CPU_READY_TIME_MS ) )
          {
            if ( tmf882xDownloadFirmware( TMF882X_A ) == BL_SUCCESS_OK ) 
            {
              stateTmf882x = TMF882X_STATE_STOPPED;
              printHelp();                              // prints on UART usage and waits for user input on serial
            }
            else
            {
              stateTmf882x = TMF882X_STATE_ERROR;
            }
          }
          else
          {
            stateTmf882x = TMF882X_STATE_ERROR;
          }
        } // else device is already enabled
      }
      else if ( rx == 'd' )       // disable
      {  
        tmf882xDisable( TMF882X_A );
        stateTmf882x = TMF882X_STATE_DISABLED;
      }
      else if ( rx == 'w' )       // wakeup
      {
        if ( stateTmf882x == TMF882X_STATE_STANDBY )
        {
          tmf882xWakeup( TMF882X_A );
          if ( tmf882xIsCpuReady( TMF882X_A, CPU_READY_TIME_MS ) )
          {
            stateTmf882x = TMF882X_STATE_STOPPED;
          }
          else
          {
            stateTmf882x = TMF882X_STATE_ERROR;
          }
        }
      }
      else if ( rx == 'p' )       // power down
      {
        if ( stateTmf882x == TMF882X_STATE_MEASURE )      // stop a measurement first
        {
          tmf882xStopMeasurement( TMF882X_A );
          tmf882xDisableInterrupts( TMF882X_A, 0xFF );               // just disable all
          stateTmf882x = TMF882X_STATE_STOPPED;
        }
        if ( stateTmf882x == TMF882X_STATE_STOPPED )
        {
          tmf882xStandby( TMF882X_A );
          stateTmf882x = TMF882X_STATE_STANDBY;
        }
      }
      else if ( rx == 'm' )
      {  
        if ( stateTmf882x == TMF882X_STATE_STOPPED )
        {
          tmf882xClrAndEnableInterrupts( TMF882X_A, TMF882X_APP_I2C_RESULT_IRQ_MASK | TMF882X_APP_I2C_RAW_HISTOGRAM_IRQ_MASK );
          tmf882xStartMeasurement( TMF882X_A );
          stateTmf882x = TMF882X_STATE_MEASURE;
        }
      }
      else if ( rx == 's' )
      {
        if ( stateTmf882x == TMF882X_STATE_MEASURE || stateTmf882x == TMF882X_STATE_STOPPED )
        {
          tmf882xStopMeasurement( TMF882X_A );
          tmf882xDisableInterrupts( TMF882X_A, 0xFF );               // just disable all
          stateTmf882x = TMF882X_STATE_STOPPED;
        }
      }
      else if ( rx == 'f' )
      {  
        if ( stateTmf882x == TMF882X_STATE_STOPPED )
        {
          PRINT_STR( "Fact Calib" );
          PRINT_LN( );
          tmf882xConfigure( TMF882X_A, 1, 4000, configSpadId[configNr], 0 );    // no histogram dumping in factory calibration allowed, 4M iterations for factory calibration recommended
          tmf882xFactoryCalibration( TMF882X_A );
          tmf882xConfigure( TMF882X_A, configPeriod[configNr], configKiloIter[configNr], configSpadId[configNr], dumpHistogramOn );
        }
      }
      else if ( rx == 'l' )
      {  
        if ( stateTmf882x == TMF882X_STATE_STOPPED )
        {
          tmf882xLoadConfigPageFactoryCalib( TMF882X_A );
          printRegisters( 0x20, 0xE0-0x20, ',' );  
        }
      }
      else if ( rx == 'r' )
      {
        if ( stateTmf882x == TMF882X_STATE_STOPPED )
        {
          if ( APP_SUCCESS_OK == tmf882xSetStoredFactoryCalibration( TMF882X_A, getPrecollectedFactoryCalibration( ) ) )
          {   
            PRINT_STR( "Set fact calib" );
          }
          else
          {
            PRINT_STR( "#Err" );
            PRINT_CHAR( SEPERATOR );
            PRINT_STR( "loadCalib" );  
          }
          PRINT_LN( );
        }
      }
      else if ( rx == 'z' )
      {
        if ( stateTmf882x == TMF882X_STATE_STOPPED )
        {
          dumpHistogramOn = dumpHistogramOn + 1;       // select histogram dump on/off, and type of histogram dumping
          if ( dumpHistogramOn > (TMF882X_COM_HIST_DUMP__histogram__electrical_calibration_24_bit_histogram + TMF882X_COM_HIST_DUMP__histogram__raw_24_bit_histogram) )
          {
            dumpHistogramOn = 0; // is off again
          }
          tmf882xConfigure( TMF882X_A, configPeriod[configNr], configKiloIter[configNr], configSpadId[configNr], dumpHistogramOn );   
          PRINT_STR( "Histogram is " );
          PRINT_INT( dumpHistogramOn );
          PRINT_LN( );
        }
      }
      else if ( rx == 'a' )
      {  
        if ( stateTmf882x != TMF882X_STATE_DISABLED )
        {
          printRegisters( 0x00, 256, ' ' );  
        }
      }
      else if ( rx == 'x' )
      {
        clkCorrectionOn = !clkCorrectionOn;       // toggle clock correction on/off  
        tmf882xClkCorrection( TMF882X_A, clkCorrectionOn );
        PRINT_STR( "Clk corr is " );
        PRINT_INT( clkCorrectionOn );
        PRINT_LN( );
      }
      else if ( rx == 'i' )
      {
        if ( stateTmf882x == TMF882X_STATE_STOPPED )
        {
          changeI2CAddress( );
        }
      }
      else 
      {
        PRINT_STR( "#Err" );
        PRINT_CHAR( SEPERATOR );
        PRINT_STR( "Cmd" );
        PRINT_CHAR( rx );
        PRINT_LN( );
      }
    }
    printState();
  }
}

// -------------------------------------------------------------------------------------------------------------

// Arduino setup function is only called once at startup. Do all the HW initialisation stuff here.
void setup ( )
{
  stateTmf882x = TMF882X_STATE_DISABLED;
  configNr = 0;                             // rotate through the given configurations
  clkCorrectionOn = 1;
  dumpHistogramOn = 0;                      // default is off
  tmf882xInitialise( TMF882X_A, ENABLE_PIN, INTERRUPT_PIN );
  logLevel = LOG_LEVEL_INFO;                                             //i2c logging only

  // configure ENABLE pin and interupt pin
  PIN_OUTPUT( ENABLE_PIN );
  PIN_INPUT( INTERRUPT_PIN );

  // start serial
  Serial.end( );                                      // this clears any old pending data 
  Serial.begin( UART_BAUD_RATE );

  // start i2c
  Wire.begin();
  Wire.setClock( I2C_CLK_SPEED );

  tmf882xDisable( TMF882X_A );                                     // this resets the I2C address in the device
  delay_in_microseconds(CAP_DISCHARGE_TIME_MS * 1000); // wait for a proper discharge of the cap
  printHelp();
}

// Arduino main loop function, is executed cyclic
void loop ( )
{
  int8_t res = APP_SUCCESS_OK;
  uint8_t intStatus = 0;
  serialInput();                                                            // handle any keystrokes from UART

  if ( stateTmf882x == TMF882X_STATE_STOPPED || stateTmf882x == TMF882X_STATE_MEASURE )
  { 
    intStatus = tmf882xGetAndClrInterrupts( TMF882X_A, TMF882X_APP_I2C_RESULT_IRQ_MASK | TMF882X_APP_I2C_ANY_IRQ_MASK | TMF882X_APP_I2C_RAW_HISTOGRAM_IRQ_MASK );   // always clear also the ANY interrupt
    if ( intStatus & TMF882X_APP_I2C_RESULT_IRQ_MASK )                      // check if a result is available (ignore here the any interrupt)
    {
      res = tmf882xReadResults( TMF882X_A );
    }
    if ( intStatus & TMF882X_APP_I2C_RAW_HISTOGRAM_IRQ_MASK )
    {
      res = tmf882xReadHistogram( TMF882X_A );                                              // read a (partial) raw histogram
    }
  }

  if ( res != APP_SUCCESS_OK )                         // in case that fails there is some error in programming or on the device, this should not happen
  {
    tmf882xStopMeasurement( TMF882X_A );
    tmf882xDisableInterrupts( TMF882X_A, 0xFF );
    stateTmf882x = TMF882X_STATE_STOPPED;
    PRINT_STR( "#Err" );
    PRINT_CHAR( SEPERATOR );
    PRINT_STR( "inter" );
    PRINT_CHAR( SEPERATOR );
    PRINT_INT( intStatus );
    PRINT_CHAR( SEPERATOR );
    PRINT_STR( "but no data" );
    PRINT_LN( );
  }
}
