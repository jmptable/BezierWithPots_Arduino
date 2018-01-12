#include <AccelStepper.h>
#include <MultiStepper.h>

const int BUTTON_START = 24;

const int SPEED_GOTO = 1800;
const int SPEED_DRAW = 800;
const int SPEED_Z = 1000;

const int ACCELERATION = 1000;

const int PEN_UP_HEIGHT = 200;

enum Mode {
  MODE_GOTO,
  MODE_DRAW,
};

enum Mode mode;

MultiStepper steppers; 

AccelStepper stepper1(1, 2, 3);
AccelStepper stepper2(1, 10, 11);
AccelStepper stepper3(1, 5, 6);

typedef struct Point {
  float x, y;
} Point;

void setup() {
  Serial.begin(9600);

  pinMode(9, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);

  pinMode(BUTTON_START, INPUT_PULLUP);

  setModeDraw();
}

void loop() {
  // Bezier control points
  Point ctrlPtA, ctrlPtB, ctrlPtC, ctrlPtD;

  while(digitalRead(BUTTON_START)) {
    int pots[8] = {
      analogRead(A13),
      analogRead(A12),
      analogRead(A11),
      analogRead(A10),
      analogRead(A1),
      analogRead(A0),
      analogRead(A4),
      analogRead(A3),
    };

    sendPotValues(pots, sizeof(pots) / sizeof(int));

    ctrlPtA.x = pots[0] * 10;
    ctrlPtA.y = pots[1] * 10;

    ctrlPtB.x = pots[2] * 11;
    ctrlPtB.y = pots[3] * 11;
    
    ctrlPtC.x = pots[4] * 11;
    ctrlPtC.y = pots[7] * 11;

    ctrlPtD.x = pots[6] * 10;
    ctrlPtD.y = pots[5] * 10;
  }

  unsigned long seed = seedOut(31);

  randomSeed(seed);

  drawBezierCircles(&ctrlPtA, &ctrlPtB, &ctrlPtC, &ctrlPtD);
  delay(1000);
}

void setModeGoto() {
  stepper1.setMaxSpeed(SPEED_GOTO);
  stepper2.setMaxSpeed(SPEED_GOTO);

  stepper3.setMaxSpeed(SPEED_Z);

  stepper1.setAcceleration(ACCELERATION);
  stepper2.setAcceleration(ACCELERATION);
  stepper3.setAcceleration(ACCELERATION);

  mode = MODE_GOTO;
}

void setModeDraw() {
  stepper1.setMaxSpeed(SPEED_DRAW);
  stepper2.setMaxSpeed(SPEED_DRAW);

  stepper3.setMaxSpeed(SPEED_Z);

  stepper1.setAcceleration(ACCELERATION);
  stepper2.setAcceleration(ACCELERATION);
  stepper3.setAcceleration(ACCELERATION);

  mode = MODE_DRAW;
}

void penUp() {
  stepper3.moveTo(PEN_UP_HEIGHT);
  stepper3.runToPosition();

  setModeGoto();
}

void penDown() {
  stepper3.moveTo(0);
  stepper3.runToPosition();

  setModeDraw();
}

void interpolate(Point *result, Point *start, Point *end, float ratio) {
  float diffX = end->x - start->x;
  float diffY = end->y - start->y;

  result->x = start->x + ratio * diffX;
  result->y = start->y + ratio * diffY;
}

Point bezierPoint(Point *result, Point *p1, Point *p2, Point *p3, Point *p4, float ratio) {
  Point midA; 
  interpolate(&midA, p1, p2, ratio);
  Point midB; 
  interpolate(&midB, p2, p3, ratio);
  Point midC; 
  interpolate(&midC, p3, p4, ratio);

  Point centerA; 
  interpolate(&centerA, &midA, &midB, ratio);
  Point centerB; 
  interpolate(&centerB, &midB, &midC, ratio);

  interpolate(result, &centerA, &centerB, ratio);
}

void drawBezierCircles(Point *p1, Point *p2, Point *p3, Point *p4) {
  float inc = 1.0f / 200.0f;
  int r = random(300, 1000);

  penUp();

  for (float ratio = inc; ratio < 1; ratio += inc) {
    Point currentPoint; 
    bezierPoint(&currentPoint, p1, p2, p3, p4, ratio - inc);
    Point nextPoint; 
    bezierPoint(&nextPoint, p1, p2, p3, p4, ratio);

    for (float spiralRatio = 0; spiralRatio < 1; spiralRatio += 1.0f / 64.0f) {
      float angle = 2 * PI * spiralRatio;
      Point center; 
      interpolate(&center, &currentPoint, &nextPoint, spiralRatio);

      float modr = 200 * cos(4 * 2 * PI * (ratio + spiralRatio * inc)) + r;

      long position[2];
      position[0] = center.x; //+ modr * sin(angle);//uncomment for circles
      position[1] = center.y; //+ modr * cos(angle);//uncomment for circles

      steppers.moveTo(position);
      steppers.runSpeedToPosition();

      if (mode != MODE_DRAW) {
        penDown();
      }
    }
  }

  penUp();
}

void sendPotValues(int (&pots)[8], int len) {
  Serial.print(pots[0], DEC);

  for (int i = 1; i < len; i += 1) {
    Serial.print(",");
    Serial.print(pots[i], DEC);
  }

  Serial.println();
}

unsigned int bitOut(void) {
  static unsigned long firstTime = 1, prev = 0;
  unsigned long bit1 = 0, bit0 = 0, x = 0, port = 0, limit = 99;
  
  if (firstTime) {
    firstTime = 0;
    prev = analogRead(port);
  }
  
  while (limit--) {
    x = analogRead(port);
    bit1 = (prev != x ? 1 : 0);
    prev = x;

    x = analogRead(port);
    bit0 = (prev != x ? 1 : 0);
    prev = x;

    if (bit1 != bit0) {
      break;
    }
  }
  
  return bit1;
}

unsigned long seedOut(unsigned int noOfBits) {
  // return value with 'noOfBits' random bits set
  unsigned long seed = 0;

  for (int i = 0; i < noOfBits; ++i) {
    seed = (seed << 1) | bitOut();
  }
    
  return seed;
}
