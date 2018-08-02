// test code for
// CNC Single Axis 4A TB6600 Stepper Motor Driver Controller
// use Serial Monitor to control 115200 baud

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

# define X_ENgnd  2 //ENA-(ENA) stepper motor enable , active low     Gray 
# define X_EN_5v  3 //ENA+(+5V) stepper motor enable , active low     Orange
# define X_DIRgnd 4 //DIR-(DIR) axis stepper motor direction control  Blue
# define X_DIR_5v 5 //DIR+(+5v) axis stepper motor direction control  Brown
# define X_STPgnd 6 //PUL-(PUL) axis stepper motor step control       Black
# define X_STP_5v 7 //PUL+(+5v) axis stepper motor step control       RED


void setup() {// *************************************************************     setup
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

  Serial.begin(115200);
}

void serialEvent()// ********************************************************      Serial in
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
void cleaningService() // *******************************************  cleaningService()
{
  inputString = ""; stringComplete = false;
}

void printNewPosition() { // **************************************************     printNewPosition
  Serial.print("nx=");
  Serial.print(nx);
  Serial.print(" mm,  ");
  Serial.print("nxi=");
  Serial.println(nxi);
}

void printPosition() { // *****************************************************     printPosition
  Serial.print("x=");
  Serial.print(x);
  Serial.print(" mm,  ");
  Serial.print("xi=");
  Serial.println(xi);
}

void help() { // **************************************************************   help
  Serial.println("Commands step by step guide");
  Serial.println("Type hello -sends TB6600 Tester Ready ");
  Serial.println("Type pos - prints actual position");
  Serial.println("Type newPos - prints desired position");
  Serial.println("Type printc - prints calibration factor");
  Serial.println("Type on  -turns TB6600 on");
  Serial.println("Type x+Number eg x1904 -to set next move position [mm]");
  Serial.println("Type xi+Number(-2^31, 2^31) eg x1904 -to set next move position [steps]");
  Serial.println("Type dx+Number eg x1904 -to set delta position [mm]");
  Serial.println("Type dxi+Number(-2^31, 2^31) eg x1904 -to set delta position [steps]");
  Serial.println("Type m -to make motor move to the next position");
  Serial.println("Type sec+Numer eg c1.02 to set new calibration factor");
  Serial.println("Type setx+Number eg setx100 to set new position [mm]");
  Serial.println("Type setxi+Number eg setxi100 to set new position [steps]");
  Serial.println("Type cdon -turns on postion data when moving will increase time of step");
  Serial.println("Type s+Number(0-2000) eg s500 -to set Microseconds between steps");
  Serial.println("Type off -turns TB6600 off");
  Serial.println("Type cdoff -turns off postion data when moving");
  Serial.println("Type calibrate to start calibration program");

  inputString = ""; // resets inputString
}

void hello() { // **************************************************************   hello
  Serial.println("TB6600 Tester Ready");
  inputString = "";
}

void ENAXON() { // *************************************************************   ENAXON
  isOn = true;
  Serial.println("ENAXON");
  digitalWrite (X_EN_5v, LOW);//ENA+(+5V) low=enabled
  inputString = "";
}

void ENAXOFF() { // ***********************************************************   ENAXOFF
  isOn = false;
  Serial.println("ENAXOFF");
  digitalWrite (X_EN_5v, HIGH);//ENA+(+5V) low=enabled
  inputString = "";
}

void MSpeed() { // ************************************************************   MSpeed
  inputString = inputString.substring(1); // removes 's' from inputString
  moveSpeed = inputString.toInt(); // changes inputString to integer
  Serial.print("Speed=");
  Serial.println(moveSpeed);
  inputString = "";
}

void comDataON() { // *********************************************************   comDataON
  comData = true;
  Serial.println("comDataOn");
  inputString = "";
}

void comDataOFF() { // ********************************************************   comDataOFF
  comData = false;
  Serial.println("comDataOFF");
  inputString = "";
}

void nextXi() { // **************************************************************    nextXi
  inputString = inputString.substring(2);
  nxi = inputString.toInt();
  dxi = nxi - xi;
  dx = c * dxi;
  nx = x + dx;
  printNewPosition();
  inputString = "";
}

void nextX() { // **************************************************************    nextX
  inputString = inputString.substring(1);
  nx = inputString.toFloat();
  dx = nx - x;
  dxi = dx/c;
  nxi = xi + dxi;
  printNewPosition();
  inputString = "";
}

void nextDxi() { // **************************************************************    nextDxi
  inputString = inputString.substring(3);
  dxi = inputString.toInt();
  dx = c * dxi;
  nxi = xi + dxi;
  nx = x + dx;
  printNewPosition();
  inputString = "";
}

void nextDx() { // **************************************************************    nextDx
  inputString = inputString.substring(2);
  dx = inputString.toFloat();
  dxi = dx/c;
  nxi = xi + dxi;
  nx = x + dx;
  printNewPosition();
  inputString = "";
}

void moveDef() { // **************************************************************    moveDef
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
  }
  inputString = "";
  } else {
    Serial.println("Engine is OFF!");
    inputString = "";
  }
}

void setXi() { // ************************************************************     setXi
  inputString = inputString.substring(5);
  xi = inputString.toInt();
  printPosition();
  inputString = "";
}

void setX() { // ************************************************************     setX
  inputString = inputString.substring(4);
  x = inputString.toInt();
  printPosition();
  inputString = "";
}

void setC() { // ************************************************************     setC
  inputString = inputString.substring(4);
  c = inputString.toInt();
  Serial.print("c = ");
  Serial.println(c);
  inputString = "";
}

void calibration() { // ******************************************************    calibration
  calibrationMode = true;
  cleaningService();
  Serial.println("Provide number of steps for the calibration process");
  do
  {
      serialEvent();
  } while(stringComplete == false);
  dxi = inputString.toInt();
  Serial.print("dxi = ");
  Serial.println(dxi);
  nxi = xi + dxi;
  cleaningService();
  Serial.println("Provide actual position [mm] of the reference point");
  do
  {
      serialEvent();
  } while(stringComplete == false);
  x = inputString.toInt();
  cleaningService();
  Serial.println("Caution! Engine will start in 5 seconds...");
  if (isOn == false) // check whether engine is turned on
    {isOn = true;
    ENAXON();}
  delay(5000);
  moveDef();
  Serial.println("Provide actual position [mm] of the reference point");
  do
  {
      serialEvent();
  } while(stringComplete == false);
  nx = inputString.toInt();
  dx = nx - x;
  x = nx;
  printPosition();
  cleaningService();
  c = dx/dxi;
  Serial.print("Calibration completed: c = ");
  Serial.println(c);
  calibrationMode = false;
}

void loop()  // **************************************************************     loop
{
  serialEvent();
  if (stringComplete)
  {
    if (inputString == "help\n")      {help();}
    if (inputString == "hello\n")     {hello();}
    if (inputString == "on\n")       {ENAXON();}
    if (inputString == "off\n")      {ENAXOFF();}
    if (inputString == "cdon\n")      {comDataON();}
    if (inputString == "cdoff\n")     {comDataOFF();}
    if (inputString == "m\n")        {moveDef();}
    if (inputString == "calibrate\n") {calibration();}
    if (inputString == "printc\n") {Serial.print("c = "); Serial.println(c); inputString="";}
    if (inputString == "pos\n")  {printPosition(); inputString = "";}
    if (inputString == "newPos\n") {printNewPosition(); inputString = "";}
    if (inputString.startsWith("setxi")) {setXi();}
    if (inputString.startsWith("setx")) {setX();}
    if (inputString.startsWith("setc")) {setC();}
    if (inputString.startsWith("dxi")) {nextDxi();}
    if (inputString.startsWith("dx")) {nextDx();}
    if (inputString.startsWith("xi"))   {nextXi();}
    if (inputString.charAt(0) == 's')   {MSpeed();}
    if (inputString.charAt(0) == 'x') {nextX();}

    if (inputString != "") {
      Serial.println("BAD COMMAND=" + inputString); // "\t" tab
    }// Serial.print("\n"); }
    cleaningService(); // clear the string:
  }
}
