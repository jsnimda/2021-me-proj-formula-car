#include <Arduino.h>

#include <memory>

class A {
 public:
  int val;
  A(int val) : val(val) {}
  A& operator=(A&& other) {
    val = other.val;
    other.val = -1;
    return *this;
  }
  A(A&& other) {
    val = other.val;
    other.val = -1;
  }
};

void setup() {
  Serial.begin(115200);
}

void loop() {
  delay(1000);
  A aa(5);
  A bb(6);
  Serial.println(typeid(aa).name());
  void* c;
  Serial.println(typeid(c).name());
  int* a;
  Serial.println(typeid(a).name());

  Serial.println("===========");
}
