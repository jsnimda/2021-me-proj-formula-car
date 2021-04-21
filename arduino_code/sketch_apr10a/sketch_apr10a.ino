#include <ESP32Servo.h>
//This program is for test TCRT5000///////
void setup() {
  Serial.begin(115200);
  xTaskCreate(vTaskA, "vTaskA", 1000, NULL, 1, NULL);
  pinMode(13, INPUT); pinMode(12, INPUT); pinMode(14, INPUT); pinMode(27, INPUT); pinMode(25, INPUT);
}

void vTaskA( void * pvParameters ) {
  /* 阻塞500ms. 注:宏pdMS_TO_TICKS用于将毫秒转成节拍数,FreeRTOS V8.1.0及
     以上版本才有这个宏,如果使用低版本,可以使用 500 / portTICK_RATE_MS */
  static double gg1 = 0;
  double a = 0.25;
  portTickType xLastWakeTime;
  const portTickType xFrequency = pdMS_TO_TICKS(1);
  for ( ;; )  {
    int TCRTe = analogRead(14);
    gg1 = TCRTe * a + gg1 * (1-a);
    Serial.print("Pin14: ");
    Serial.println(gg1);
    delay(1);
//    vTaskDelayUntil( &xLastWakeTime, xFrequency );
  }
}

void loop() {
  delay(250);
  return;
  //  int TCRTa =digitalRead(13);
  //  int TCRTb =digitalRead(12);
  //  int TCRTc =digitalRead(14);
  //  int TCRTd =digitalRead(27);
  //  int TCRTe =digitalRead(16);
  int TCRTa = analogRead(13);
  int TCRTb = analogRead(12);
  int TCRTc = analogRead(14);
  int TCRTd = analogRead(27);
  int TCRTe = analogRead(25);

  Serial.print("Pin13: ");
  Serial.println(TCRTa);
  Serial.print("Pin12: ");
  Serial.println(TCRTb);
  Serial.print("Pin14: ");
  Serial.println(TCRTc);
  Serial.print("Pin27: ");
  Serial.println(TCRTd);
  Serial.print("Pin16: ");
  Serial.println(TCRTe);
//  TCRTa -= 200;
//  TCRTb -= 200;
//  TCRTc -= 200;
//  TCRTd -= 200;
//  TCRTe -= 200;

  double aa = (TCRTa * -2.0 + TCRTb * -1.0 + TCRTd * 1.0 + TCRTe * 2.0) / (TCRTa + TCRTb + TCRTc + TCRTd + TCRTe);
  Serial.print("aa: ");
  Serial.println(aa);
  Serial.println();

  //}
}

//This program is for test TCRT5000///////
