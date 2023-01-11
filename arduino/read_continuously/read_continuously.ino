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

// Function checks the UART for received characters and interprets them
void serialInput( );

// Arduino setup function is only called once at startup. Do all the HW initialisation stuff here.
void setup( );

// Arduino main loop function, is executed cyclic
void loop( );


// ---------------------------------------------- functions -----------------------------------------


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

  //Enable device (same as recieving 'e' on serial input in example script)
  tmf882xEnable( TMF882X_A );
  delay_in_microseconds( ENABLE_TIME_MS * 1000 );
  tmf882xClkCorrection( TMF882X_A, clkCorrectionOn ); 
  tmf882xSetLogLevel( TMF882X_A, MY_LOG_LEVEL );
  tmf882xWakeup( TMF882X_A );
  if ( tmf882xIsCpuReady( TMF882X_A, CPU_READY_TIME_MS ) )
  {
    if ( tmf882xDownloadFirmware( TMF882X_A ) == BL_SUCCESS_OK ) 
    {
      stateTmf882x = TMF882X_STATE_STOPPED;                           // prints on UART usage and waits for user input on serial
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

  //Perform factory calibration (might as well do this every time at startup - same as receiving 'f' on serial input in example script)
  tmf882xConfigure( TMF882X_A, 1, 4000, configSpadId[configNr], 0 );    // no histogram dumping in factory calibration allowed, 4M iterations for factory calibration recommended
  tmf882xFactoryCalibration( TMF882X_A );

  //Configure the period, number of iterations (KiloIter), and spad map (SpadId)
  tmf882xConfigure( TMF882X_A, 100, 900, TMF882X_COM_SPAD_MAP_ID__spad_map_id__map_no_1, 1 );   

  //Begin measuring (same as recieving an 'm' on serial input in example script)
  tmf882xClrAndEnableInterrupts( TMF882X_A, TMF882X_APP_I2C_RESULT_IRQ_MASK | TMF882X_APP_I2C_RAW_HISTOGRAM_IRQ_MASK );
  tmf882xStartMeasurement( TMF882X_A );
  stateTmf882x = TMF882X_STATE_MEASURE;

}

// Arduino main loop function, is executed cyclic
void loop ( )
{
  int8_t res = APP_SUCCESS_OK;
  uint8_t intStatus = 0;

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