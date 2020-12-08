#include <Keyboard.h>
#include <Mouse.h>
#include <Wire.h>

bool LOG = true;
bool isMaster = true;
bool isLeft = isMaster;

//################### Common #########################
#define SLAVE_ADDR 0x0F
//################### Trackball #########################
#define TRACKBALL_INT_PIN 7
#define TRACKBALL_ADDR 0x0A
#define TRACKBALL2_ADDR 0x0B
#define TRACK_BALL_REG_LEFT 0x04
#define XAXIS_COEF 2
#define YAXIS_COEF 1.5
#define CHIP_ID 0xBA11
#define VERSION 1
#define avgRollingSetSize 10
#define REG_LED_RED 0x00
#define REG_LED_GRN 0x01
#define REG_LED_BLU 0x02
#define REG_LED_WHT 0x03
#define REG_INT 0xF9
#define MSK_INT_OUT_EN 0b00000010

const int xScaleFactor = 2;

int tbReadCount = 100;

float boost = 1;
float y = 0;
float x = 0;
int clck = 0;

String xLabel = "X: ";
String yLabel = "     Y: ";

String clickLabel = "Click: ";
String boostLabel = "     Boost: ";

void tbSetup()
{
  Mouse.begin();
  tbWriteColor(0xff, 0xfc, 0xf0, 0x00, TRACKBALL_ADDR);
  tbWriteColor(0xff, 0xfc, 0xf0, 0x00, TRACKBALL2_ADDR);
}

void tbWriteColor(char r, char g, char b, char w, byte trackBallAddr)
{
  Serial.println("Colors");
  char bytes[5] = {REG_LED_RED, r, g, b, w};
  Wire.beginTransmission(trackBallAddr);
  Wire.write(bytes, 5);
  Wire.endTransmission();
}

void tbRead(int trackBallAddr)
{
  Wire.beginTransmission(trackBallAddr);
  Wire.write(TRACK_BALL_REG_LEFT);
  Wire.endTransmission();
  delay(10);
  Wire.requestFrom(trackBallAddr, 5);
  if (Wire.available() < 5)
  {
    return;
  }
  byte bytes[5];
  for (int i = 0; i < 5; i++)
  {
    bytes[i] = Wire.read();
  }

  y = ((bytes[0] - bytes[1]));
  x = ((bytes[3] - bytes[2]));

  int btn = bytes[4];

  if (y != 0 || x != 0)
  {
    printInt(int(bytes[0]));
    printInt(int(bytes[1]));
    printInt(int(bytes[2]));
    printInt(int(bytes[3]));
    print(" ");

    if (x < 0)
    {
      x = pow(x * -1, 3) * -1;
    }
    else
    {
      x = pow(x, 3);
    }
    if (y < 0)
    {
      y = pow(y * -1, 3) * -1;
    }
    else
    {
      y = pow(y, 3);
    }
    x = min(x, 127);
    x = max(x, -127);
    x = x * xScaleFactor;
    y = min(y, 127);
    y = max(y, -127);

    if (!isLeft)
    {
      y = y * -1;
      x = x * -1;
    }
    println(xLabel + x + yLabel + y);
    Mouse.move(x, y, 0);
  }

  if (btn != clck)
  {
    clck = btn;
    if (clck == 129)
    {
      Mouse.click();
    }
    else if (clck == 128)
    {
      Mouse.press();
    }
    else if (Mouse.isPressed())
    {
      Mouse.release();
    }

    println(clickLabel + clck);
  }
}
//################### keyboard #########################
const byte colMask = 0xE0;
const byte rowMask = 0X1C;
const byte mod = 255;
const byte lms = 254;
const byte rms = 253;
const byte layers = 2;
const byte keyBufSize = 6;
int currentLayer = 0;

byte rows[] = {6, 7, 8, 9, 10, 11};
const int rowCount = sizeof(rows) / sizeof(rows[0]);

byte cols[] = {13, 18, 19, 20, 21, 22, 23};
const int colCount = sizeof(cols) / sizeof(cols[0]);

byte keys[colCount][rowCount];
byte keyStates[keyBufSize];

//left map - break is currently set to insert 209
byte leftKeyMap[layers][colCount][rowCount] = {
    {{61, 49, 50, 51, 52, 53},
     {177, 113, 119, 101, 114, 116},
     {9, 97, 115, 100, 102, 103},
     {129, 122, 120, 99, 118, 98},
     {130, 96, 211, 210, 32, 212},
     {0, 0, 214, 213, 8, 0},
     {0, 0, 209, 128, 176, mod}},
    {{194, 195, 196, 197, 198, 199},
     {0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, mod}}};
// right map
byte rightKeyMap[layers][colCount][rowCount] = {
    {{54, 55, 56, 57, 48, 45},
     {121, 117, 105, 111, 112, 39},
     {104, 106, 107, 108, 59, 133},
     {110, 109, 44, 46, 47, 92},
     {lms, rms, 218, 217, 91, 93},
     {0, 32, 216, 215, 0, 0},
     {
         176,
         mod,
         132,
         135,
         0,
         0,
     }},
    {{200, 201, 202, 203, 204, 205},
     {0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0},
     {
         0,
         mod,
         0,
         0,
         0,
         0,
     }}};

byte (*keyMap)[colCount][rowCount] = rightKeyMap;

void kbSetup()
{
  if (isLeft)
  {
    keyMap = leftKeyMap;
  }

  Keyboard.begin();
  for (int x = 0; x < rowCount; x++)
  {
    pinMode(rows[x], INPUT);
  }
  for (int x = 0; x < colCount; x++)
  {
    pinMode(cols[x], INPUT_PULLUP);
  }
}

void processKeyStates()
{
  for (int i = 0; i < keyBufSize; i++)
  {
    if (keyStates[i] != 0)
    {
      byte col = (keyStates[0] & colMask) >> 5;
      byte row = (keyStates[0] & rowMask) >> 2;
      byte btnState = keyStates[0] & 1;

      printIntln(col);
      printIntln(row);
      printIntln(btnState);
      handleKeyPress(keyMap[currentLayer][col][row], btnState);
      keyStates[i] = 0;
    }
  }
}
void handleKeyPress(byte key, byte btnState)
{
  if (btnState == 0)
  {
    if (key == lms)
    {
      Mouse.press(MOUSE_LEFT);
    }
    else if (key == rms)
    {
      Mouse.press(MOUSE_RIGHT);
    }
    else if (key == mod)
    {
      currentLayer = 1;
      println("layer 1");
    }
    else
    {
      Keyboard.press(key);
      printInt(int(key));
      println(" pressed");
    }
  }
  else
  {
    if (key == lms)
    {
      Mouse.release(MOUSE_LEFT);
    }
    else if (key == rms)
    {
      Mouse.release(MOUSE_RIGHT);
    }
    else if (key == mod)
    {
      currentLayer = 0;
      println("layer 0");
    }
    else
    {
      Keyboard.release(key);
      printInt(int(key));
      println(" released");
    }
  }
}

void kbRead()
{
  // iterate the columns
  int keyIdx = 0;
  for (int colIndex = 0; colIndex < colCount; colIndex++)
  {
    // col: set to output to low
    byte curCol = cols[colIndex];
    pinMode(curCol, OUTPUT);
    digitalWrite(curCol, LOW);

    // row: iterate through the rows
    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
    {
      byte rowCol = rows[rowIndex];
      pinMode(rowCol, INPUT_PULLUP);
      byte btnState = digitalRead(rowCol);
      if (keys[colIndex][rowIndex] != btnState)
      {
        if (keyIdx >= keyBufSize)
        {
          return; // buffer full
          println("Key buffer full");
        }

        byte state = colIndex;
        state <<= 3;
        state += rowIndex;
        state <<= 2;
        state += 2; // flip the empty bit as a nonce so we can distinguish between no value and 0,0 released
        state += btnState;
        printBits(state);
        keyStates[keyIdx++] = state;
      }
      keys[colIndex][rowIndex] = btnState;
      pinMode(rowCol, INPUT);
    }
    // disable the column
    pinMode(curCol, INPUT);
  }
}

void kbSlaveRead()
{

  Wire.requestFrom(SLAVE_ADDR, 5);
  while (Wire.available())
  {
    printInt(Wire.read());
    println("");
  }
}
void kbReadBytes()
{

  Wire.write("hello");
}
//###############################################################################

void setup()
{
  for (int i = 0; i < keyBufSize; i++)
  {
    keyStates[i] = 0;
  }

  Serial.begin(115200);

  if (isMaster == true)
  {
    println("primary");
    Wire.setClock(400000);
    Wire.begin();

    tbSetup();
  }
  else
  {
    Wire.begin(SLAVE_ADDR);      // join i2c bus with address #slave
    Wire.onRequest(kbReadBytes); // register event
  }

  kbSetup();
}

void loop()
{
  if (isMaster == true)
  {
    tbRead(TRACKBALL_ADDR);
    tbRead(TRACKBALL2_ADDR);
    kbSlaveRead();
  }
  kbRead();
  processKeyStates();
}

void printIntln(int msg)
{
  if (LOG == true)
  {
    Serial.println(msg);
  }
}

void printInt(int msg)
{
  if (LOG == true)
  {
    Serial.print(msg);
  }
}

void print(String msg)
{
  if (LOG == true)
  {
    Serial.print(msg);
  }
}

void println(String msg)
{
  if (LOG == true)
  {
    Serial.println(msg);
  }
}

void printBits(byte data)
{
  byte mask = 1; //our bitmask
  for (mask = 128; mask > 0; mask >>= 1)
  { //iterate through bit mask
    if (data & mask)
    { // if bitwise AND resolves to true
      print("1");
    }
    else
    { //if bitwise and resolves to false
      print("0");
    }
  }

  println("");
}