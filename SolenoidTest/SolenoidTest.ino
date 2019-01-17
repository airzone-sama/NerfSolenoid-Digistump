#include <Bounce2.h>

// ---------------------------------
// CONFIGURATION HERE
// ---------------------------------

// This is the solenoid pulse time. Make this bigger to give it more power - 
// but don't allow it to get so high that it sticks on
#define PULSE_ON_TIME 65

// This is the solenoid retract time. Make this bigger to give the solenoid more
// time to reset. A bigger spring will make this faster
#define PULSE_RETRACT_TIME 105

// This is your desired DPS. Make it 99 to go as fast as possible, otherwise any number
// higher than 1 is OK
#define TargetDPS 99

// This is the burst fire size. Change this to any number more than one to increase the number of
// burst shots... DOn't be silly though.
#define BURST_SIZE 3


// ---------------------------------
// STOP TOUCHING HERE
// ---------------------------------


// Pin declarations
#define PIN_MOSFET 5
#define PIN_TRIGGER 4
#define PIN_MODE_A 2
#define PIN_MODE_B 3

// Other stuff
int TimeBetweenShots = 0;    // How long to wait from one shot to the next
#define MODE_SINGLE 0
#define MODE_BURST 1
#define MODE_AUTO 2
#define MODE_AUTO_LASTSHOT 3
#define MODE_IDLE 4

// Current Mode
int CurrentMode = MODE_IDLE;
int RunningMode = MODE_SINGLE;

// Software debouncing
Bounce BtnTrigger = Bounce();

void setup() {
  // put your setup code here, to run once:

  pinMode( PIN_MOSFET, OUTPUT );
  digitalWrite( PIN_MOSFET, LOW );

  pinMode( PIN_TRIGGER, INPUT_PULLUP );
  pinMode( PIN_MODE_A, INPUT_PULLUP );
  pinMode( PIN_MODE_B, INPUT_PULLUP );

  BtnTrigger.attach(PIN_TRIGGER);
  BtnTrigger.interval(5); // interval in ms

  if( TargetDPS == 99 ) // Full rate
  {
    TimeBetweenShots = 0;
  }
  else
  {
    int PulseOverhead = PULSE_ON_TIME + PULSE_RETRACT_TIME;
    int TotalPulseOverhead = PulseOverhead * TargetDPS;
    int FreeMS = 1000 - TotalPulseOverhead;
    if( FreeMS <= 0 )
    {
      TimeBetweenShots = 0; // Pusher won't achieve this rate
    }
    else
    {
      TimeBetweenShots = FreeMS / TargetDPS;
    }
  }
  

  Serial.begin( 57600 );
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
    if( (millis() - LastShot) < TimeBetweenShots )
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
      digitalWrite( PIN_MOSFET, HIGH );

      return;
    }

    if( (millis() - LastCycleStarted) < (PULSE_ON_TIME + PULSE_RETRACT_TIME) )
    {
      if( CurrentCyclePosition != Cycle_Retract )
      {
        //Serial.println( "End Pulse" );
      }
      CurrentCyclePosition = Cycle_Retract;
      digitalWrite( PIN_MOSFET, LOW );

      return;      
    }

    if( (millis() - LastCycleStarted) < (PULSE_ON_TIME + PULSE_RETRACT_TIME + TimeBetweenShots) )
    {
      if( CurrentCyclePosition != Cycle_Cooldown )
      {
        //Serial.println( "Cooling Down" );
      }
      CurrentCyclePosition = Cycle_Cooldown;
      digitalWrite( PIN_MOSFET, LOW );

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

  if( digitalRead( PIN_MODE_A ) == LOW )
  {
    RunningMode = MODE_SINGLE;
  }
  else if( digitalRead( PIN_MODE_B ) == LOW )
  {
    RunningMode = MODE_AUTO;
  }
  else
  {
    RunningMode = MODE_BURST;
  }


  BtnTrigger.update();

  if( CurrentMode == MODE_IDLE )
  {
    if( BtnTrigger.fell() )
    {
      CurrentMode = RunningMode;  // You can change between modes here is desired 
      Serial.println( "Fire Please" );
      return true;
    }
  }
  
  if( CurrentMode == MODE_AUTO )
  {
    if( BtnTrigger.rose() )
    {
      CurrentMode = MODE_AUTO_LASTSHOT;
      Serial.println( "AUTO - Last Shot" );
      return true;
    }
  }
  
  return false;
}
