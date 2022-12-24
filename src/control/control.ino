#include <Multiservo.h>

Multiservo midservo;
Multiservo leftservo;
Multiservo rightservo;
Multiservo grabservo;

constexpr uint8_t midpin = 6;
constexpr uint8_t leftpin = 7;
constexpr uint8_t rightpin = 8;
constexpr uint8_t grabpin = 9;

float midangle, rightangle, leftangle;
float midnew, leftnew, rightnew;
float xstart, ystart, zstart;
float xnew, ynew, znew;
int signal;

float position = 60; //нынешнее положение клешни
float open = 120; // открытая клешня
float closed = 60; // закрытая клешня

// размеры робота
const float e = 138.56;     // сторона подвижной платформы
const float f = 519.6;     // сторона неподвижного основания
const float re = 150.0;    // плечо
const float rf = 250.0;    // кисть

// тригонометрические константы
const float sqrt3 = sqrt(3.0);
const float pi = 3.141592653;    // PI
const float sin120 = sqrt3/2.0;
const float cos120 = -0.5;
const float tan60 = sqrt3;
const float sin30 = 0.5;
const float tan30 = 1/sqrt3;

// прямая кинематика: (theta1, theta2, theta3) -> (x0, y0, z0)
// возвращаемый статус: 0=OK, -1=несуществующая позиция
int delta_calcForward(float theta1, float theta2, float theta3, float &x0, float &y0, float &z0) {
    float t = (f-e)*tan30/2;
    float dtr = pi/(float)180.0;

    theta1 *= dtr;
    theta2 *= dtr;
    theta3 *= dtr;

    float y1 = -(t + rf*cos(theta1));
    float z1 = -rf*sin(theta1);

    float y2 = (t + rf*cos(theta2))*sin30;
    float x2 = y2*tan60;
    float z2 = -rf*sin(theta2);

    float y3 = (t + rf*cos(theta3))*sin30;
    float x3 = -y3*tan60;
    float z3 = -rf*sin(theta3);

    float dnm = (y2-y1)*x3-(y3-y1)*x2;

    float w1 = y1*y1 + z1*z1;
    float w2 = x2*x2 + y2*y2 + z2*z2;
    float w3 = x3*x3 + y3*y3 + z3*z3;

    // x = (a1*z + b1)/dnm
    float a1 = (z2-z1)*(y3-y1)-(z3-z1)*(y2-y1);
    float b1 = -((w2-w1)*(y3-y1)-(w3-w1)*(y2-y1))/2.0;

    // y = (a2*z + b2)/dnm;
    float a2 = -(z2-z1)*x3+(z3-z1)*x2;
    float b2 = ((w2-w1)*x3 - (w3-w1)*x2)/2.0;

    // a*z^2 + b*z + c = 0
    float a = a1*a1 + a2*a2 + dnm*dnm;
    float b = 2*(a1*b1 + a2*(b2-y1*dnm) - z1*dnm*dnm);
    float c = (b2-y1*dnm)*(b2-y1*dnm) + b1*b1 + dnm*dnm*(z1*z1 - re*re);

    // дискриминант
    float d = b*b - (float)4.0*a*c;
    if (d < 0) return -1; // несуществующая позиция

    z0 = -(float)0.5*(b+sqrt(d))/a;
    x0 = (a1*z0 + b1)/dnm;
    y0 = (a2*z0 + b2)/dnm;
    return 0;
}

// обратная кинематика
// вспомогательная функция, расчет угла theta1 (в плоскости YZ)
int delta_calcAngleYZ(float x0, float y0, float z0, float &theta) {
    float y1 = -0.5 * 0.57735 * f; // f/2 * tg 30
    y0 -= 0.5 * 0.57735 * e;       // сдвигаем центр к краю
    // z = a + b*y
    float a = (x0*x0 + y0*y0 + z0*z0 +rf*rf - re*re - y1*y1)/(2*z0);
    float b = (y1-y0)/z0;
    // дискриминант
    float d = -(a+b*y1)*(a+b*y1)+rf*(b*b*rf+rf);
    if (d < 0) return -1; // несуществующая точка
    float yj = (y1 - a*b - sqrt(d))/(b*b + 1); // выбираем внешнюю точку
    float zj = a + b*yj;
    theta = 180.0*atan(-zj/(y1 - yj))/pi + ((yj>y1)?180.0:0.0);
    return 0;
}

// обратная кинематика: (x0, y0, z0) -> (theta1, theta2, theta3)
// возвращаемый статус: 0=OK, -1=несуществующая позиция
int delta_calcInverse(float x0, float y0, float z0, float &theta1, float &theta2, float &theta3) {
    theta1 = theta2 = theta3 = 0;
    int status = delta_calcAngleYZ(x0, y0, z0, theta1);
    if (status == 0) status = delta_calcAngleYZ(x0*cos120 + y0*sin120, y0*cos120-x0*sin120, z0, theta2);  // rotate coords to +120 deg
    if (status == 0) status = delta_calcAngleYZ(x0*cos120 - y0*sin120, y0*cos120+x0*sin120, z0, theta3);  // rotate coords to -120 deg
    return status;
}

void setup() {
  Serial.begin(115200);

  midservo.attach(midpin);
  leftservo.attach(leftpin);
  rightservo.attach(rightpin);
  grabservo.attach(grabpin);

  midangle = 90;
  leftangle = 90;
  rightangle = 90;

  delta_calcForward(midangle, leftangle, rightangle,  xstart,  ystart, zstart);

  //Serial.print("xstart=");
  //Serial.print(xstart);
  //Serial.print("\l ystart=");
  //Serial.print(ystart);
  //Serial.print("\l zstart=");
  //Serial.print(zstart);
  //Serial.print("\n");
  //Serial.print("\l midangle=");
  //Serial.print(midangle);
  //Serial.print("\l leftangle=");
  //Serial.print(leftangle);
  //Serial.print("\l rightangle=");
  //Serial.print(rightangle);

  midservo.write(midangle);
  leftservo.write(leftangle);
  rightservo.write(rightangle);

    xnew = xstart;
    ynew = ystart; 
    znew = zstart;
    
  Serial.flush();
}

void loop() 
{ 
  //Serial.print("Mid: ");
  //Serial.print(midservo.read());
  //Serial.print(" \l Left: ");
  //Serial.print(leftservo.read());
  //Serial.print(" \l Right: ");
  //Serial.print(rightservo.read());
  //Serial.print("\n");  
    
  if (Serial.available()) 
  {

  int val = Serial.readStringUntil('\n').toInt();

  //Serial.print(val);  

  if (val== 1)
  {
    xnew = xnew + 10;
    delta_calcInverse(xnew, ynew, znew, midnew, leftnew, rightnew);
    if (midnew==0 && leftnew ==0 &&rightnew==0)
    {
      xnew = xnew - 10;
      signal = 1;
    }
  }
  
  if (val== 2)
  {
    xnew = xnew - 10;
    delta_calcInverse(xnew, ynew, znew, midnew, leftnew, rightnew);
    if (midnew==0 && leftnew ==0 &&rightnew==0)
    {
      xnew = xnew +10;
      signal = 1;
    }
  }

 if (val== 3)
  {
    ynew = ynew + 10;
    delta_calcInverse(xnew, ynew, znew, midnew, leftnew, rightnew);
    if (midnew==0 && leftnew ==0 &&rightnew==0)
    {
      ynew = ynew - 10;
      signal = 1;
    }
  }

 if (val== 4)
  {
    ynew = ynew - 10;
    delta_calcInverse(xnew, ynew, znew, midnew, leftnew, rightnew);
    if (midnew==0 && leftnew ==0 &&rightnew==0)
    {
      ynew = ynew + 10;
      signal = 1;
    }
  }

 if (val== 5)
  {
    znew = znew + 10;
    delta_calcInverse(xnew, ynew, znew, midnew, leftnew, rightnew);
    if (midnew==0 && leftnew ==0 &&rightnew==0)
    {
      znew = znew - 10;
      signal = 1;
    }

    if (midnew >=180 || leftnew >=180 ||rightnew>=180)
    {
      znew = znew - 10;
      signal = 1;
    }
  }

 if (val==6)
  {
    znew = znew - 10;
    delta_calcInverse(xnew, ynew, znew, midnew, leftnew, rightnew);
    if (midnew==0 && leftnew ==0 &&rightnew==0)
    {
      znew = znew + 10;
    }
  }

  if (val ==7)
  {
    xnew=ynew=0;
    znew=-351.98;
  }

  if (val == 8)
  {
    position = open;
    //Serial.print("position=");
    //Serial.print(position);
    //Serial.print("\l \l");
  }

  if (val == 9)
  {
    position = closed;
    //Serial.print("position=");
    //Serial.print(position);
    //Serial.print("\l \l");
  }

  Serial.flush();
}

  delta_calcInverse(xnew, ynew, znew, midnew, leftnew, rightnew);
  
  midservo.write(midnew);
  leftservo.write(leftnew);
  rightservo.write(180 - rightnew);
  grabservo.write(position);

  signal = 0;
}
