#include <Bounce2.h>

// Pin declarations
#define PIN_OUTPUT 2
#define PIN_TRIGGER 0
#define PIN_SPEED_A 1
#define PIN_SPEED_B 3

// Solenoid pulse generation times
#define PULSE_ON_TIME 35
#define PULSE_RETRACT_TIME 80

// Other stuff
#define BURST_SIZE 3              // How many shots per burst
#define TIME_BETWEEN_SHOTS 50    // How long to wait from one shot to the next
#define MODE_SINGLE 0
#define MODE_BURST 1
#define MODE_AUTO 2
#define MODE_AUTO_LASTSHOT 3
#define MODE_IDLE 4

// Current Mode
int CurrentMode = MODE_IDLE;

// 

// Software debouncing
Bounce BtnTrigger = Bounce();

void setup() {
  // put your setup code here, to run once:

  pinMode( PIN_OUTPUT, OUTPUT );
  digitalWrite( PIN_OUTPUT, LOW );

  //digitalWrite( PIN_TRIGGER, HIGH );
  pinMode( PIN_TRIGGER, INPUT_PULLUP );
  //pinMode( PIN_SPEED_A, INPUT_PULLUP );
  //pinMode( PIN_SPEED_B, INPUT_PULLUP );

  BtnTrigger.attach(PIN_TRIGGER);
  BtnTrigger.interval(5); // interval in ms

  //Serial.begin( 57600 );
}

void loop() {
  // put your main code here, to run repeatedly:

  static unsigned long LastShot = 0;
  static int ShotsToFire = 0;
  #define Cycle_Idle 0
  #define Cycle_Pulse 1
  #define Cycle_Retract 2
  #define Cycle_Cooldown 3
  static int CurrentCyclePosition = Cycle_Idle;
  static unsigned long LastCycleStarted = 0;
  
  bool ButtonChange = ProcessButtons();

  if( ButtonChange )
  {
    if( (millis() - LastShot) < TIME_BETWEEN_SHOTS )
    {
      //Serial.println( "Too soon to fire again...");
      CurrentMode = MODE_IDLE;
    }
    switch( CurrentMode )
    {
      case MODE_SINGLE:
        ShotsToFire = 1;
        LastCycleStarted = millis();
        break;
      case MODE_BURST:
        ShotsToFire = BURST_SIZE;
        LastCycleStarted = millis();
        break;
      case MODE_AUTO:
        ShotsToFire = 9999;
        LastCycleStarted = millis();
        break;
      case MODE_AUTO_LASTSHOT:
        if( CurrentCyclePosition == Cycle_Pulse )
        {
          ShotsToFire = 1;
          LastCycleStarted = millis();
        }
        else
        {
          ShotsToFire = 0;
          LastCycleStarted = millis();
        }
        break;  
    }
  }
  else
  {
    if( CurrentMode == MODE_IDLE )
    {
      delay( 10 );
      return; // DO nothing anymore
    }

    if( ShotsToFire == 0 && CurrentMode != MODE_IDLE )
    {
      CurrentMode = MODE_IDLE;
      CurrentCyclePosition = Cycle_Idle;
      LastShot = millis();
      //Serial.println( "Finished shooting" );
      //Serial.println( CurrentMode );
      delay( 10 );
      return;
    }

    if( (millis() - LastCycleStarted) < PULSE_ON_TIME )
    {
      if( CurrentCyclePosition != Cycle_Pulse )
      {
        //Serial.println( "Start Pulse" );
      }
      CurrentCyclePosition = Cycle_Pulse;
      digitalWrite( PIN_OUTPUT, HIGH );

      return;
    }

    if( (millis() - LastCycleStarted) < (PULSE_ON_TIME + PULSE_RETRACT_TIME) )
    {
      if( CurrentCyclePosition != Cycle_Retract )
      {
        //Serial.println( "End Pulse" );
      }
      CurrentCyclePosition = Cycle_Retract;
      digitalWrite( PIN_OUTPUT, LOW );

      return;      
    }

    if( (millis() - LastCycleStarted) < (PULSE_ON_TIME + PULSE_RETRACT_TIME + TIME_BETWEEN_SHOTS) )
    {
      if( CurrentCyclePosition != Cycle_Cooldown )
      {
        //Serial.println( "Cooling Down" );
      }
      CurrentCyclePosition = Cycle_Cooldown;
      digitalWrite( PIN_OUTPUT, LOW );

      return;      
    }

    // Otherwise time to decrease shot count
    CurrentCyclePosition = Cycle_Idle;
    ShotsToFire -= 1;
    LastCycleStarted = millis();
    //Serial.println( "Bang!!" );
    
    return;
  }


}

bool ProcessButtons()
{
  BtnTrigger.update();

  if( CurrentMode == MODE_IDLE )
  {
    if( BtnTrigger.fell() )
    {
      CurrentMode = MODE_AUTO;  // You can change between modes here is desired 
      //Serial.println( "Fire Please" );
      return true;
    }
  }

  if( CurrentMode == MODE_AUTO )
  {
    if( BtnTrigger.rose() )
    {
      CurrentMode = MODE_AUTO_LASTSHOT;
      //Serial.println( "AUTO - Last Shot" );
      return true;
    }
  }
  
  return false;
}
