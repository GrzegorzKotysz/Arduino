// test code for
// CNC Single Axis 4A TB6600 Stepper Motor Driver Controller
// use Serial Monitor to control 115200 baud

// enable storage of calibration data
// http://playground.arduino.cc/Code/EEPROMWriteAnything

#include <EEPROM.h>
template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          EEPROM.write(ee++, *p++);
    return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value)
{
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          *p++ = EEPROM.read(ee++);
    return i;
}

// struct for storing calibration data
struct calib_struct
{
  long xi;
  float x;
  float c;
} calib_data;

// long to enable greater range of movement
long  xi=0; // stores current position in steps
long nxi; // stores new position in steps
long dxi; // stores delta position in steps
float x=0;   // stores current position in mm
float nx;  // stores new position in mm
float dx;  // stores delta position in mm
float c=1;   // stores calibration data (c = dx/dxi)
int moveSpeed = 600; //step in Microseconds
String     inputString = "help\n"; // a string to hold incoming data
boolean stringComplete = true;     // whether the string is completed
boolean        comData = false;    // whether com data is on when motors are moving will slow them down
boolean   calibrationMode = false; // checks whether program is run in calibration mode
boolean   isOn = false; // checks whether engine is on
boolean   autoModeOn = false; // whether program is in auto mode
float step = 0;
 
// functions for accessing and altering calib_data
void getCalib_data()
{
  xi = calib_data.xi;
  x = calib_data.x;
  c = calib_data.c;
}

void setCalib_data()
{
  calib_data.xi = xi;
  calib_data.x = x;
  calib_data.c = c;
}

# define X_ENgnd  2 //ENA-(ENA) stepper motor enable , active low     Gray 
# define X_EN_5v  3 //ENA+(+5V) stepper motor enable , active low     Orange
# define X_DIRgnd 4 //DIR-(DIR) axis stepper motor direction control  Blue
# define X_DIR_5v 5 //DIR+(+5v) axis stepper motor direction control  Brown
# define X_STPgnd 6 //PUL-(PUL) axis stepper motor step control       Black
# define X_STP_5v 7 //PUL+(+5v) axis stepper motor step control       RED


void setup()  // **********************************************************    setup
{
  pinMode (X_ENgnd , OUTPUT); //ENA-(ENA)
  pinMode (X_EN_5v , OUTPUT); //ENA+(+5V)
  pinMode (X_DIRgnd, OUTPUT); //DIR-(DIR)
  pinMode (X_DIR_5v, OUTPUT); //DIR+(+5v)
  pinMode (X_STPgnd, OUTPUT); //PUL-(PUL)
  pinMode (X_STP_5v, OUTPUT); //PUL+(+5v)
  pinMode (13, OUTPUT);
  digitalWrite (X_ENgnd,  LOW); //ENA-(ENA)
  digitalWrite (X_EN_5v, HIGH); //ENA+(+5V) low=enabled
  digitalWrite (X_DIRgnd, LOW); //DIR-(DIR)
  digitalWrite (X_DIR_5v, LOW); //DIR+(+5v)
  digitalWrite (X_STPgnd, LOW); //PUL-(PUL)
  digitalWrite (X_STP_5v, LOW); //PUL+(+5v)

  // getting calibration data from ROM
  EEPROM_readAnything(0, calib_data);
  getCalib_data();
  
  Serial.begin(115200);
}

void serialEvent()  // ****************************************************    serialEvent
{ while (Serial.available())  // automatically exits loop when buffer was read
  {
    char inChar = (char)Serial.read();            // get the new byte:
    if (inChar > 0)     {
      inputString += inChar; // add it to the inputString:
    }
    if (inChar == '\n') {
      stringComplete = true; // if the incoming character is a newline, set a flag so the main loop can do something about it:
    }
  }
}

// cleaningService() sets inputString to empty string and resets stringComplete to false
void cleaningService()  // ************************************************    cleaningService()
{
  inputString = ""; stringComplete = false;
}

// template for printing floats
template <typename number>
void printFloat(number decimal, int decimalPlaces = 4)  // ****************    printFloat
{
  long h; //helper
  h = (long)decimal; // type conversion (holds integral part)
  Serial.print(h);
  Serial.print(".");
  h = abs(h); // needed for proper display of negative numbers
  decimal = abs(decimal); 
  decimal -= h; // decimal = 0. ... (holds fractional part)
  for(int i=0; i < decimalPlaces; ++i)
  {
    decimal *= 10; // shift numbers to the left (base10)
    h = (long)decimal; // take integral part
    Serial.print(h); // print integral part
    decimal -= h; // remove integral part
  }
}

// prints c as float but with given precision
void customPrintC()  // ***************************************************    customPrintC
{
  inputString = inputString.substring(6); // removes 'printc' from inputString
  Serial.print("c = ");
  printFloat(c, inputString.toInt()); // prints c to given decimal place
  Serial.println(); // ends line
  inputString = "";
}

void printNewPosition()  // ***********************************************    printNewPosition
{
  Serial.print("nx = ");
  printFloat(nx, 2);
  Serial.print(" mm,  ");
  Serial.print("nxi = ");
  Serial.print(nxi);
  Serial.println();
}

void printPosition()  // **************************************************    printPosition
{
  Serial.print("x = ");
  printFloat(x, 2);
  Serial.print(" mm,  ");
  Serial.print("xi = ");
  Serial.print(xi);
  Serial.println();
}

void help()  // ***********************************************************    help
{
  Serial.println("Commands step by step guide");
  Serial.println("hello      - sends TB6600 Tester Ready ");
  Serial.println("pos        - prints actual position");
  Serial.println("newPos     - prints desired position");
  Serial.println("printc     - prints calibration factor");
  Serial.println("printc+n   - prints calibration factor with given precision");
  Serial.println("on         - turns TB6600 on");
  Serial.println("x+n        - sets next move position [mm]");
  Serial.println("xi+n       - sets next move position [steps]");
  Serial.println("dx+n       - sets delta position [mm]");
  Serial.println("dxi+n      - sets delta position [steps]");
  Serial.println("m          - motor moves to the next position");
  Serial.println("setc+n     - sets new calibration factor");
  Serial.println("setx+n     - sets new position [mm]");
  Serial.println("setxi+n    - sets new position [steps]");
  Serial.println("cdon       - turns on postion data when moving (will increase time of step)");
  Serial.println("s+n        - sets Microseconds between steps");
  Serial.println("off        - turns TB6600 off");
  Serial.println("cdoff      - turns off postion data when moving");
  Serial.println("calibrate  - starts calibration procedure");
  Serial.println("--------------------------------------------------------------------------------");
  Serial.println("AUTO MODE:");
  Serial.println("--------------------------------------------------------------------------------");
  Serial.println("autoOn     - starts auto mode");
  Serial.println("autoOff    - stops auto mode");
  Serial.println("step+n     - AM: sets step in [mm] for next move");
  Serial.println("w / s      - AM: increase distance / decrease distance");

  inputString = ""; // resets inputString
}

void hello()  // **********************************************************    hello
{
  Serial.println("TB6600 Tester Ready");
  inputString = "";
}

void ENAXON()  // *********************************************************    ENAXON
{
  isOn = true;
  Serial.println("ENAXON");
  digitalWrite (X_EN_5v, LOW);//ENA+(+5V) low=enabled
  inputString = "";
}

void ENAXOFF()  // ********************************************************    ENAXOFF
{
  isOn = false;
  Serial.println("ENAXOFF");
  digitalWrite (X_EN_5v, HIGH);//ENA+(+5V) low=enabled
  inputString = "";
}

void MSpeed()  // *********************************************************    MSpeed
{
  inputString = inputString.substring(1); // removes 's' from inputString
  moveSpeed = inputString.toInt(); // changes inputString to integer
  Serial.print("Speed=");
  Serial.println(moveSpeed);
  inputString = "";
}

void comDataON()  // ******************************************************    comDataON
{
  comData = true;
  Serial.println("comDataOn");
  inputString = "";
}

void comDataOFF()  // *****************************************************    comDataOFF
{
  comData = false;
  Serial.println("comDataOFF");
  inputString = "";
}

void nextXi()  // *********************************************************    nextXi
{
  inputString = inputString.substring(2);
  nxi = inputString.toInt();
  dxi = nxi - xi;
  dx = c * dxi;
  nx = x + dx;
  printNewPosition();
  inputString = "";
}

void nextX()  // **********************************************************    nextX
{
  inputString = inputString.substring(1);
  nx = inputString.toFloat();
  dx = nx - x;
  dxi = dx/c;
  nxi = xi + dxi;
  printNewPosition();
  inputString = "";
}

void nextDxi()  // ********************************************************    nextDxi
{
  inputString = inputString.substring(3);
  dxi = inputString.toInt();
  dx = c * dxi;
  nxi = xi + dxi;
  nx = x + dx;
  printNewPosition();
  inputString = "";
}

void nextDx()  // *********************************************************    nextDx
{
  inputString = inputString.substring(2);
  dx = inputString.toFloat();
  dxi = dx/c;
  nxi = xi + dxi;
  nx = x + dx;
  printNewPosition();
  inputString = "";
}

void moveDef()  // ********************************************************    moveDef
{
  if (isOn == true)
  {
  int dir; // identifies direction of movement
  if (nxi > xi)
  {
    dir = nxi - xi;  // rotates left if LOW
    digitalWrite (X_DIR_5v, LOW);
    dir = 1;
  }
  else
  {
    dir = nxi - xi;
    digitalWrite (X_DIR_5v, HIGH);
    dir = -1;
  }
  if (comData == true)
  { for (; xi != nxi; xi = xi + dir)
    { digitalWrite (X_STP_5v, HIGH);
      Serial.print("xi=");
      delayMicroseconds (moveSpeed);
      digitalWrite (X_STP_5v, LOW);
      delayMicroseconds (moveSpeed);
      Serial.println(xi);
    }
  }
  else
  { for (; xi != nxi; xi = xi + dir)
    { digitalWrite (X_STP_5v, HIGH);
      delayMicroseconds (moveSpeed);
      digitalWrite (X_STP_5v, LOW);
      delayMicroseconds (moveSpeed);
    }
  }
  if (calibrationMode == false)
  {
    x = nx;
    printPosition();
    setCalib_data();
    EEPROM_writeAnything(0, calib_data);
  }
  inputString = "";
  } else {
    Serial.println("Engine is OFF!");
    inputString = "";
  }
}

void setXi()  // **********************************************************    setXi
{
  inputString = inputString.substring(5);
  xi = inputString.toInt();
  printPosition();
  setCalib_data();
  EEPROM_writeAnything(0, calib_data);
  inputString = "";
}

void setX()  // ***********************************************************    setX
{
  inputString = inputString.substring(4);
  x = inputString.toFloat();
  printPosition();
  setCalib_data();
  EEPROM_writeAnything(0, calib_data);
  inputString = "";
}

void setC()  // ***********************************************************    setC
{
  inputString = inputString.substring(4);
  c = inputString.toFloat();
  Serial.print("c = ");
  printFloat(c);
  Serial.println();
  setCalib_data();
  EEPROM_writeAnything(0, calib_data);
  inputString = "";
}

void calibration()  // ****************************************************    calibration
{
  calibrationMode = true;
  cleaningService();
  Serial.println("specify number of steps for the calibration process");
  do
  {
      serialEvent();
  } while(stringComplete == false);
  dxi = inputString.toInt();
  Serial.print("dxi = ");
  Serial.println(dxi);
  nxi = xi + dxi;
  cleaningService();
  Serial.println("specify actual position [mm] of the reference point");
  do
  {
      serialEvent();
  } while(stringComplete == false);
  x = inputString.toFloat();
  cleaningService();
  Serial.println("Caution! Engine will start in 5 seconds...");
  if (isOn == false) // check whether engine is turned on
    {isOn = true;
    ENAXON();}
  delay(5000);
  moveDef();
  Serial.println("specify actual position [mm] of the reference point");
  do
  {
      serialEvent();
  } while(stringComplete == false);
  nx = inputString.toFloat();
  dx = nx - x;
  x = nx;
  printPosition();
  cleaningService();
  c = dx/dxi;
  Serial.print("calibration completed: c = ");
  printFloat(c, 5);
  Serial.println();
  setCalib_data();
  EEPROM_writeAnything(0, calib_data);
  calibrationMode = false;
}

// set of functions for auto-mode
void printStep()
{
  Serial.print("step = ");
  printFloat(step, 2);
  Serial.print(" mm");
  Serial.println();
}

void autoMode(boolean autoSwitch)  // *************************************    autoMode
{
    if (autoSwitch == true & autoModeOn == false)
    {
        Serial.println("Starting Auto Mode");
        ENAXON();
        if (step == 0)
        {
            Serial.println("Specify desired auto step in [mm]:");
            do
            {
                serialEvent();
            } while(stringComplete == false);
            step = inputString.toFloat();
            cleaningService();
        }
        printStep();
        autoModeOn = true;
        Serial.println("Ready");
    } else if (autoSwitch == true & autoModeOn == true)
    {
        Serial.println("Already in Auto Mode");
    } else if (autoSwitch == false & autoModeOn == true)
    {
        Serial.println("Disabling Auto Mode");
        ENAXOFF();
        autoModeOn = false;
        Serial.println("Ready");
    } else if (autoSwitch == false & autoMode == false)
    {
        Serial.println("Auto Mode inactive");
    }
}

void setStep() // *********************************************************    setStep
{
  inputString = inputString.substring(4);
  step = inputString.toFloat();
  printStep();
  inputString = "";
}

void autoMove(float s, int dir)  // ***************************************    autoMove
{
  switch (dir) {
      case 0:
          dx = -s; break;
      case 1:
          dx = s; break;
  }
  dxi = dx/c;
  nxi = xi + dxi;
  nx = x + dx;
  moveDef();
}


void loop()  // ***********************************************************    loop
{
  serialEvent();
  if (stringComplete)
  {
    if (inputString == "w\n")            {autoMove(step, 1);}
    if (inputString == "s\n")            {autoMove(step, 0);}
    if (inputString == "autoOn\n")       {autoMode(true);}
    if (inputString == "autoOff\n")      {autoMode(false);}
    if (inputString == "help\n")         {help();}
    if (inputString == "hello\n")        {hello();}
    if (inputString == "on\n")           {ENAXON();}
    if (inputString == "off\n")          {ENAXOFF();}
    if (inputString == "cdon\n")         {comDataON();}
    if (inputString == "cdoff\n")        {comDataOFF();}
    if (inputString == "m\n")            {moveDef();}
    if (inputString == "calibrate\n")    {calibration();}
    if (inputString == "printc\n")       {Serial.print("c="); printFloat(c); Serial.println(); inputString="";}
    if (inputString == "pos\n")          {printPosition(); inputString = "";}
    if (inputString == "newPos\n")       {printNewPosition(); inputString = "";}
    if (inputString.startsWith("step"))  {setStep();}
    if (inputString.startsWith("printc")){customPrintC();}
    if (inputString.startsWith("setxi")) {setXi();}
    if (inputString.startsWith("setx"))  {setX();}
    if (inputString.startsWith("setc"))  {setC();}
    if (inputString.startsWith("dxi"))   {nextDxi();}
    if (inputString.startsWith("dx"))    {nextDx();}
    if (inputString.startsWith("xi"))    {nextXi();}
    if (inputString.charAt(0) == 's')    {MSpeed();}
    if (inputString.charAt(0) == 'x')    {nextX();}

    if (inputString != "") 
    { Serial.println("BAD COMMAND=" + inputString);    }
    cleaningService(); // clear the string and reset stringComplete
  }
}
