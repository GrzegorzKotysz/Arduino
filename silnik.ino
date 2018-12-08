// test code for
// CNC Single Axis 4A TB6600 Stepper Motor Driver Controller
// use Serial Monitor to control 115200 baud

#include "DHT.h" // biblioteka DHT (Adafruit_Sensor.h)

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
unsigned int calib_data_size = sizeof(calib_data); // stores size of the calibration data, so presets for test Mode can be easily stored
unsigned long elapsedTime = 0; // measures elapsed time from certain flag by using milis() function
unsigned long startTime = 0;
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
// wilgotnosc i temperatura
# define DHTPIN 8          // numer pinu sygnałowego
# define DHTTYPE DHT11     // typ czujnika (DHT11). Jesli posiadamy DHT22 wybieramy DHT22
DHT dht(DHTPIN, DHTTYPE); // definicja czujnika

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
  dht.begin();            // inicjalizacja czujnika
  
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

void printHumAndTemp()  // ************************************************    printHumAndTemp
{

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  // Sprawdzamy czy są odczytane wartości
  if (isnan(temp) || isnan(hum))
  {
    // Jeśli nie, wyświetlamy informację o błędzie
    Serial.println("Blad odczytu danych z czujnika");
  } else
  {
    // Jeśli tak, wyświetlamy wyniki pomiaru
    Serial.print("Wilgotnosc: ");
    Serial.print(hum);
    Serial.print(" % ");
    Serial.print("Temperatura: ");
    Serial.print(temp);
    Serial.println(" *C");
  }
}

boolean emergencyStop()  // ***********************************************    emergencyStop()
{
  char inChar = (char)Serial.read();
  if (inChar == '\n')
  {
    Serial.println("------------------");
    Serial.println("| EMERGENCY STOP |");
    Serial.println("------------------");
    return true;
  }
  else
  {
    return false;
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
void customPrintC(int var)  // ***************************************************    customPrintC
{
  Serial.print("c = ");
  printFloat(c, var); // prints c to given decimal place
  Serial.println(); // ends line
  inputString = "";
}

// convert input string to integer
int commandToInt(String input, byte comLen)  // ***************************    commandToInt
{
  input = input.substring(comLen);
  return input.toInt();
}

// convert input string to float
float commandToFloat(String input, byte comLen)  // ***********************    commandToFloat
{
  input = input.substring(comLen);
  return input.toFloat();
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

float elapsedTimeSeconds()  // ********************************************    elapsedTimeSeconds
{
  elapsedTime = millis() - startTime;
  return float(elapsedTime)/1000.0;
}

void printPosition()  // **************************************************    printPosition
{
  Serial.print("x = ");
  printFloat(x, 2);
  Serial.print(" mm,  ");
  Serial.print("xi = ");
  Serial.print(xi);
  Serial.print(", test time: ");
  printFloat(elapsedTimeSeconds(), 1);
  Serial.print(" s");
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
  Serial.println("start      - starts time measurement");
  Serial.println("testModeOn - start Test Mode program");
  Serial.println("t          - shows temperature and humidity from DHT");
  Serial.println("<void>     - EMERGENCY SWITCH");
  Serial.println("--------------------------------------------------------------------------------");
  Serial.println("AUTO MODE:");
  Serial.println("--------------------------------------------------------------------------------");
  Serial.println("autoOn     - starts auto mode");
  Serial.println("autoOff    - stops auto mode");
  Serial.println("step+n     - AM: sets step in [mm] for next move");
  Serial.println("w / s      - AM: increase distance / decrease distance");
  Serial.println("--------------------------------------------------------------------------------");

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

void MSpeed(int var)  // *********************************************************    MSpeed
{
//  inputString = inputString.substring(1); // removes 's' from inputString
  moveSpeed = var; // changes inputString to integer
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

void nextXi(int var)  // *********************************************************    nextXi
{
  nxi = var;
  dxi = nxi - xi;
  dx = c * dxi;
  nx = x + dx;
  printNewPosition();
  inputString = "";
}

void nextX(float var)  // **********************************************************    nextX
{
  nx = var;
  dx = nx - x;
  dxi = dx/c;
  nxi = xi + dxi;
  printNewPosition();
  inputString = "";
}

void nextDxi(int var)  // ********************************************************    nextDxi
{
  dxi = var;
  dx = c * dxi;
  nxi = xi + dxi;
  nx = x + dx;
  printNewPosition();
  inputString = "";
}

void nextDx(float var)  // *********************************************************    nextDx
{
  dx = var;
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
    { 
      if (emergencyStop())
      {
        break;
      } 
      else 
      {
        digitalWrite(X_STP_5v, HIGH);
        Serial.print("xi=");
        delayMicroseconds(moveSpeed);
        digitalWrite(X_STP_5v, LOW);
        delayMicroseconds(moveSpeed);
        Serial.println(xi);
      }
    }
  }
  else
  { for (; xi != nxi; xi = xi + dir)
    { 
      if (emergencyStop())
      {
        break;
      }
      else
      {
        digitalWrite(X_STP_5v, HIGH);
        delayMicroseconds(moveSpeed);
        digitalWrite(X_STP_5v, LOW);
        delayMicroseconds(moveSpeed);
      }
    }
  }
  if (calibrationMode == false)
  {
    x = nx - (nxi - xi) * c; // now it's ok even for emergencyStop
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

void setXi(int var)  // **********************************************************    setXi
{
  xi = var;
  printPosition();
  setCalib_data();
  EEPROM_writeAnything(0, calib_data);
  inputString = "";
}

void setX(float var)  // ***********************************************************    setX
{
  x = var;
  printPosition();
  setCalib_data();
  EEPROM_writeAnything(0, calib_data);
  inputString = "";
}

void setC(float var)  // ***********************************************************    setC
{
  c = var;
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
  cleaningService();
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

void setStep(float var) // *********************************************************    setStep
{
  step = var;
  printStep();
  inputString = "";
}

void autoMove(float s, int dir)  // ***************************************    autoMove
{
  if (autoModeOn == false)
  {
    Serial.println("Auto Mode is OFF!");
    inputString = "";
  }
  else {
  switch (dir) {
      case 0:
          dx = -s; break;
      case 1:
          dx = s; break;
  }
  dxi = dx/c;
  nxi = xi + dxi;
  nx = x + dx;
  printPosition();
  moveDef(); }
}

// Inner program: Test Mode:
// Set of functions:
// struct for preset data and declaration for three presets
struct presetStruct
{
  float tmStep;
  byte tmLoops; // limits memory usage
  unsigned int tmTime; // how much time will elapse after i-1 command to execute i command
} presetA[16], presetB[16], presetC[16];

unsigned int presetSize = sizeof(presetA);
char presetChoice;
char presetChoiceEEPROM;

void tmHelp()  // *********************************************************    tmHelp
{
  Serial.println("--------------------------------------------------------------------------------");
  Serial.println("Available commands:");
  Serial.println("--------------------------------------------------------------------------------");
  Serial.println("help         - print help");
  Serial.println("testModeOff  - turn off Test Mode");
  Serial.println("setPreset    - start the procedure of creating new preset");
  Serial.println("choosePreset - choose preset from RAM");
  Serial.println("writePreset  - write preset to EEPROM");
  Serial.println("readPreset   - read new preset from EEPROM");
  Serial.println("editPreset   - edit existing preset");
  Serial.println("printPreset  - print selected preset");
  Serial.println("start        - start the test");
//  Serial.println("end    - end test prematurely (in between tests)");
  Serial.println("--------------------------------------------------------------------------------");
  Serial.println("on         - turns TB6600 on");
  Serial.println("x+n        - sets next move position [mm]");
  Serial.println("dx+n       - sets delta position [mm]");
  Serial.println("m          - motor moves to the next position");
  Serial.println("setx+n     - sets new position [mm]");
  Serial.println("off        - turns TB6600 off");
  Serial.println("<void>     - EMERGENCY SWITCH");
  Serial.println("--------------------------------------------------------------------------------");
}

// pass id by reference to control command repeat etc
void setPresetData(struct presetStruct* preset, byte& id) // *********************    setPresetData    
{
  if (inputString == "end\n")
  { preset[id].tmLoops = 255; return; }
  if (inputString.charAt(0) != 's') {Serial.print("Incorrect Command: "); Serial.println(inputString); return;} 
  boolean isCommandCorrect = false; // is it necessary? check
  byte startID = 1;
  byte endID = 2;
  byte k; // iterator
  unsigned int inputLength = inputString.length();
  for(k = startID; k < inputLength; k++)
  {
    if (inputString.charAt(k) == 'l')
    {
      isCommandCorrect = true;
      endID = k;
      preset[id].tmStep = (inputString.substring(startID, endID)).toFloat();
      break;
    }
  }
  if ((k == (inputLength - 1)) || !isCommandCorrect) {Serial.print("Incorrect Command: "); Serial.println(inputString); return;}
  startID = k+1;
  for(k = startID; k < inputLength; k++)
  {
    if (inputString.charAt(k) == 't')
    {
      isCommandCorrect = true;
      endID = k;
      preset[id].tmLoops = (inputString.substring(startID, endID)).toFloat();
      break;
    }
  }
  if ((k == (inputLength - 1)) || !isCommandCorrect) {Serial.print("Incorrect Command: "); Serial.println(inputString); return;}
  startID = k+1;
  preset[id].tmTime = (int) (((inputString.substring(startID)).toFloat())*1000); // cast time in seconds to miliseconds
  // increment command id:
  id++;
}

void tmPresetHelp(byte mode)  // ******************************************    tmPresetHelp
{
  switch (mode)
  {
    case 0: // set mode
        Serial.println("*******************");
        Serial.println("* Creating Preset *");
        Serial.println("*******************");
        break;
    case 1: // edit mode
        break;
  }
  Serial.println("Command structure: s[float]l[byte]t[float]");
  Serial.println("Finish command: end");
  Serial.println("--------------------------------------------------------------------------------");
}

void choosePreset()  // ***************************************************    choosePreset
{
  cleaningService();
  Serial.println("Choose preset [A/B/C]:");
  for(;;)
  {
    serialEvent();
    if(stringComplete)
    {
      if (inputString == "A\n") {presetChoice = 'A'; break;}
      if (inputString == "B\n") {presetChoice = 'B'; break;}
      if (inputString == "C\n") {presetChoice = 'C'; break;}
      Serial.println("Error: incorrect preset chosen");
      break; 
    }
  }
  loopBreak: cleaningService();
  Serial.print("Preset "); Serial.print(presetChoice); Serial.println(" chosen");
}

// setPreset() writes user serial input into chosen preset
void setPreset()  // ******************************************************    setPreset
{
  choosePreset();
  tmPresetHelp(0);
  byte id = 0;
  Serial.print("Command 0:");
  for(;;)
  {
    serialEvent();
    if (stringComplete)
    {
      Serial.print(inputString);
      switch (presetChoice)
      {
        case 'A': setPresetData(presetA, id); break;
        case 'B': setPresetData(presetB, id); break;
        case 'C': setPresetData(presetC, id); break;
      }
      if (inputString == "end\n") {break;} // wrong commands will be resolved in setPresetData function
      if (id > 15) {Serial.println("Preset full"); break;}  // indicates the end of space
      Serial.print("Command "); Serial.print(id); Serial.print(":");
      cleaningService(); // clear the string and reset stringComplete
    }
  }
  Serial.print("Preset "); Serial.print(presetChoice); Serial.println(" set");
  Serial.println("--------------------------------------------------------------------------------");
}

void choosePresetEEPROM()  // *********************************************    choosePresetEEPROM
{
  cleaningService();
  for(;;)
  {
    serialEvent();
    if (stringComplete)
    {
      if (inputString == "A\n") {presetChoiceEEPROM = 'A'; cleaningService(); break;}
      if (inputString == "B\n") {presetChoiceEEPROM = 'B'; cleaningService(); break;}
      if (inputString == "C\n") {presetChoiceEEPROM = 'C'; cleaningService(); break;}
      if (inputString == "abort\n") {Serial.println("writing aborted");}
      if (inputString != "") 
      { Serial.println("Wrong Preset=" + inputString); }
      cleaningService(); // clear the string and reset stringComplete
    }
  }
  cleaningService();
}

void writePresetData(presetStruct* preset)  // ****************************    writePresetData
{
  switch(presetChoiceEEPROM) 
  {
    case 'A': EEPROM_writeAnything(calib_data_size, preset); break;
    case 'B': EEPROM_writeAnything(calib_data_size + presetSize, preset); break;
    case 'C': EEPROM_writeAnything(calib_data_size + 2*presetSize, preset); break;
  }
  Serial.println("Write to EEPROM completed");
}

void writePreset()  // ****************************************************    writePreset
{
  choosePreset();
  Serial.println("To abort process execute: abort");
  Serial.println("Select EEPROM preset (current preset will be written):");
  choosePresetEEPROM();
  switch (presetChoice) // execute writePresetData for appropriate preset; EEPROM preset is chosen inside writePresetData function
  {
    case 'A': writePresetData(presetA); break;
    case 'B': writePresetData(presetB); break;
    case 'C': writePresetData(presetC); break;
  }
  Serial.println("Preset written");
}

void readPresetData(presetStruct* preset)  // *****************************    readPresetData
{
  switch(presetChoiceEEPROM)
  {
    case 'A': EEPROM_readAnything(calib_data_size, preset); break;
    case 'B': EEPROM_readAnything(calib_data_size + presetSize, preset); break;
    case 'C': EEPROM_readAnything(calib_data_size + 2*presetSize, preset); break;
  }
}

void readPreset()  // *****************************************************    readPreset
{
  choosePreset();
  Serial.println("To abort process execute: abort");
  Serial.println("Select EEPROM preset (will be written to current preset):");
  choosePresetEEPROM();
  switch (presetChoice)
  {
    case 'A': readPresetData(presetA); break;
    case 'B': readPresetData(presetB); break;
    case 'C': readPresetData(presetC); break;
  }
  Serial.println("Preset ready");
}

void editPreset()  // *****************************************************    editPreset
{
  byte id;
  choosePreset();
  for(;;)
  {
    Serial.println("Choose command id (0-15): [abort] to stop editing");
    serialEvent();
    if (stringComplete)
    {
      if (inputString == "abort\n") { break; }
      id = inputString.toInt();
      if(id >= 0 && id <= 15)
      {
        Serial.println("Command:");
        serialEvent();
        if (stringComplete)
        {
          switch (presetChoice)
          {
            case 'A': setPresetData(presetA, id); break;
            case 'B': setPresetData(presetB, id); break;
            case 'C': setPresetData(presetC, id); break;
          }
          if (inputString == "end\n") {break;} // wrong commands will be resolved in setPresetData function
          cleaningService(); // clear the string and reset stringComplete
          Serial.print("Command "); Serial.print(id); Serial.println(":");
        }
      }
    }
  }
  Serial.println("Editing finished");
}

void printPresetData(presetStruct* preset, byte& id)  // ******************    printPresetData
{
  if (preset[id].tmLoops == 255) {id = 16; return;}
  Serial.print("Step: "); Serial.print(preset[id].tmStep); Serial.print(" mm, ");
  Serial.print("Loops: "); Serial.print(preset[id].tmLoops); Serial.print(", ");
  Serial.print("Time: "); Serial.print((float)preset[id].tmTime/1000.0); Serial.println(" s");
  id++;
}

void printPreset()  // ****************************************************    printPreset
{
  choosePreset();
  byte id = 0;
  for(;;)
  {
    switch (presetChoice)
    {
      case 'A': printPresetData(presetA, id); break;
      case 'B': printPresetData(presetB, id); break;
      case 'C': printPresetData(presetC, id); break;
    }
    if (id > 15) {break;}
  }
  Serial.println("End of preset");
}

// Setting variable so array does not to be accessed each time;
void setTMVariables(const presetStruct* preset, const byte& id, float& varStep, byte& varLoops, unsigned int& varTime)
{
  varStep = preset[id].tmStep;
  varLoops = preset[id].tmLoops;
  varTime = preset[id].tmTime;
}

// function to execute commands stored in preset data
void testModeExecute(const struct presetStruct* preset)  // ****************************    testModeExecute
{
  ENAXON();
  byte id = 0, loopCount = 0;
  startTime = millis();
  unsigned int deltaTime = millis();
  printPosition();
  printHumAndTemp();
  float tmStep; byte tmLoops; unsigned int tmTime;
  for(;;)
  { 
    // preparing data
    setTMVariables(preset, id, tmStep, tmLoops, tmTime);
    // wait for the appropriate time to start the command
    for(;;)
    {
      if(emergencyStop()) {goto emergencyContinue;}
      if((millis()-deltaTime) >= tmTime) {break;}
    }
    // check for end of preset flag
    if (tmLoops == 255) {break;}
    // set new position depending on definition type
    if (tmLoops == 0) 
    {
      printPosition();
      nextX(tmStep);
      moveDef(); // within moveDef printNewPosition() is called -> new time written; but deltaTime also updated to store absolute time
      deltaTime = millis();
      ++id; // indicate that this step is finished
    }
    else  // code for executing all loops within command
    {
      printPosition();
      nextDx(tmStep);
      moveDef();
      deltaTime = millis();
      ++loopCount;
      // check whether desired number of loops was executed. If yes, increment id and reset loopCount
      if (loopCount == tmLoops) 
      {
        loopCount = 0;
        ++id;
      }
    }
    // check whether we would be outside of preset:
    if (id > 15) {break;}
    }
emergencyContinue: ;
// printPosition();
printHumAndTemp();
ENAXOFF();
Serial.println("Test finished");
}



void testMode()  // *******************************************************    testMode
{
  cleaningService();
  // information for user
  Serial.println("*************");
  Serial.println("* Test Mode *");
  Serial.println("*************");
  tmHelp();
  for(;;) // program control
  {
    serialEvent();
    if (stringComplete)
    {
      if (inputString == "testModeOff\n") {ENAXOFF(); Serial.println("Test Mode deactivated"); inputString = ""; break;}
      if (inputString == "help\n") {tmHelp(); inputString = "";}
      if (inputString == "setPreset\n") {setPreset(); inputString = "";}
      if (inputString == "choosePreset\n") {choosePreset(); inputString = "";}
      if (inputString == "writePreset\n") {writePreset(); inputString = "";}
      if (inputString == "readPreset\n") {readPreset(); inputString = "";}
      if (inputString == "editPreset\n") {editPreset(); inputString = "";}
      if (inputString == "printPreset\n") {printPreset(); inputString = "";}
      if (inputString == "start\n") {
                                      switch (presetChoice)
                                      {
                                        case 'A': testModeExecute(presetA); break;
                                        case 'B': testModeExecute(presetB); break;
                                        case 'C': testModeExecute(presetC); break;
                                        default: Serial.println("Wrong Preset, call 'choosePreset'"); break;
                                      }  
                                      inputString = "";
                                    }
      if (inputString != "") 
      { Serial.println("BAD COMMAND=" + inputString); }
      cleaningService(); // clear the string and reset stringComplete
    }
  }
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
    if (inputString == "m\n")            {printPosition(); moveDef();}
    if (inputString == "calibrate\n")    {calibration();}
    if (inputString == "printc\n")       {Serial.print("c="); printFloat(c); Serial.println(); inputString="";}
    if (inputString == "pos\n")          {printPosition(); inputString = "";}
    if (inputString == "newPos\n")       {printNewPosition(); inputString = "";}
    if (inputString == "start\n")        {
                                          Serial.println("--------------------------------------------------------------------------------");
                                          startTime = millis(); inputString = "";}
    if (inputString == "testModeOn\n")   {testMode(); inputString = "";}
    if (inputString == "t\n")            {printHumAndTemp();  inputString = "";}
    if (inputString.startsWith("step"))  {setStep(commandToFloat(inputString, 4));}
    if (inputString.startsWith("printc")){customPrintC(commandToInt(inputString, 6));}
    if (inputString.startsWith("setxi")) {setXi(commandToInt(inputString, 5));}
    if (inputString.startsWith("setx"))  {setX(commandToFloat(inputString, 4));}
    if (inputString.startsWith("setc"))  {setC(commandToFloat(inputString, 4));}
    if (inputString.startsWith("dxi"))   {nextDxi(commandToInt(inputString, 3));}
    if (inputString.startsWith("dx"))    {nextDx(commandToFloat(inputString, 2));}
    if (inputString.startsWith("xi"))    {nextXi(commandToInt(inputString, 2));}
    if (inputString.charAt(0) == 's')    {MSpeed(commandToInt(inputString, 1));}
    if (inputString.charAt(0) == 'x')    {nextX(commandToFloat(inputString, 1));}

    if (inputString != "") 
    { Serial.println("BAD COMMAND=" + inputString);    }
    cleaningService(); // clear the string and reset stringComplete
  }
}
