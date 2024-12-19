
#include <EEPROM.h>  // إضافة مكتبة EEPROM

// تعريف منافذ التوصيل للشاشة 7-segment
byte segPin[8] = {2, 3, 4, 5, 6, 7, 8, 9}; // {a,b,c,d,e,f,g,dp}
byte digitPin[2] = {A1, A0}; // منافذ الشاشة

// تعريف منافذ الأزرار
const int upButtonPin = A2;   // زر UP متصل بالمنفذ A2
const int downButtonPin = A3; // زر DOWN متصل بالمنفذ A3
const int startButtonPin = A4; // زر START متصل بالمنفذ A4

// تعريف limit switch
const int limitSwitchPin = A5; // limit switch متصل بالمنفذ A5

// تعريف منفذ التحكم في الموتور
const int motorControlPin = 10; // منفذ التحكم في الموتور (ترانزستور)

// تعريف منفذ الجرس
const int buzzerPin = 11; // منفذ الجرس

// تعريف قيم الأرقام من 0 إلى 9 على الشاشة 7-segment
byte segValue[10][7] = {
   {0,0,0,0,0,0,1}, //0
   {1,0,0,1,1,1,1}, //1
   {0,0,1,0,0,1,0}, //2
   {0,0,0,0,1,1,0}, //3
   {1,0,0,1,1,0,0}, //4
   {0,1,0,0,1,0,0}, //5
   {0,1,0,0,0,0,0}, //6
   {0,0,0,1,1,1,1}, //7
   {0,0,0,0,0,0,0}, //8
   {0,0,0,0,1,0,0}  //9
};

// متغيرات لتخزين الرقم الحالي وحالة العد التنازلي
int currentNumber = 0;
int lastNumber = 0; // متغير لتخزين الرقم الأخير
bool countdownStarted = false;
unsigned long previousMillis = 0;
const long interval = 1000;  // 1 ثانية للعد التنازلي

// متغير لتتبع حالة limit switch
bool limitSwitchStateLast = HIGH; // للحفاظ على الحالة السابقة لمفتاح الحد

void setup() {
  Serial.begin(9600);

  // ضبط منافذ الشاشة كمخارج
  for (int i = 0; i < 8; i++) {
    pinMode(segPin[i], OUTPUT);
  }

  // ضبط منافذ أرقام الشاشة كمخارج
  pinMode(digitPin[0], OUTPUT);
  pinMode(digitPin[1], OUTPUT);

  // ضبط الأزرار كمداخل مع المقاومات الداخلية
  pinMode(upButtonPin, INPUT_PULLUP);
  pinMode(downButtonPin, INPUT_PULLUP);
  pinMode(startButtonPin, INPUT_PULLUP); // حالة زر START
  pinMode(limitSwitchPin, INPUT_PULLUP); // limit switch

  // ضبط منفذ التحكم في الموتور
  pinMode(motorControlPin, OUTPUT); // إعداد المنفذ كخرج

  // ضبط منفذ الجرس
  pinMode(buzzerPin, OUTPUT); // إعداد المنفذ كخرج

  // قراءة الرقم المحفوظ من EEPROM عند بدء التشغيل
  currentNumber = EEPROM.read(0); // قراءة الرقم من العنوان 0 في EEPROM
  if (currentNumber > 99) { // التأكد من أن الرقم في النطاق
    currentNumber = 0; // إعادة تعيين الرقم إذا كان خارج النطاق
  }
  lastNumber = currentNumber; // حفظ الرقم الحالي كرقم أخير
  display_N(currentNumber); // عرض الرقم المحفوظ
  soundBuzzer(600); // إصدار صوت عند بدء التشغيل (600 مللي ثانية)
}

void loop() {
  // قراءة حالة limit switch
  bool limitSwitchState = digitalRead(limitSwitchPin);

  // قراءة حالة الأزرار
  bool upButtonState = digitalRead(upButtonPin) == LOW;
  bool downButtonState = digitalRead(downButtonPin) == LOW;
  bool startButtonState = digitalRead(startButtonPin) == LOW; // حالة زر START

  // التحقق من الضغط على زر START
  if (startButtonState) { // تم الضغط على الزر
    if (!countdownStarted) { // إذا لم يكن العد التنازلي بدأ
      lastNumber = currentNumber; // حفظ الرقم الحالي كرقم أخير
      countdownStarted = true; // بدء العد التنازلي
      previousMillis = millis(); // بدء العد التنازلي
      startMotor(); // تشغيل الموتور
      soundBuzzer(600); // إصدار صوت عند بدء العد (600 مللي ثانية)
    }
    delay(200); // تأخير لمنع التكرار السريع
  }

  // إذا كان limit switch في حالة NO (مفتوح - يتم تشغيل التايمر والموتور)
  if (limitSwitchState == HIGH) { // NO (Normally Open)
    // التحقق من حالة العد التنازلي وتحديث الرقم
    if (countdownStarted) {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        if (currentNumber > 0) {
          currentNumber--; // تقليل الرقم
        } else {
          countdownStarted = false; // إنهاء العد التنازلي
          stopMotor(); // إيقاف الموتور عند الانتهاء
          
          // إصدار 3 أصوات عند انتهاء العد
          for (int i = 0; i < 3; i++) {
            soundBuzzer(600); // إصدار صوت عند انتهاء العد (600 مللي ثانية)
            delay(100); // تأخير صغير بين الأصوات
          }

          currentNumber = lastNumber; // إعادة الرقم إلى آخر قيمة
        }
      }
    }
  } else { // حالة NC (مغلق - يتم إيقاف التايمر والموتور)
    if (countdownStarted) {
      countdownStarted = false; // إيقاف العد التنازلي
      stopMotor(); // إيقاف الموتور
    }
  }

  // إذا تم الضغط على زر UP
  if (!countdownStarted && upButtonState) {
    if (currentNumber < 99) {
      currentNumber++; // زيادة الرقم
      EEPROM.write(0, currentNumber); // تخزين الرقم في EEPROM
      soundBuzzer(100); // إصدار صوت عادي عند الضغط على زر UP
      delay(200); // تأخير لمنع التكرار السريع
    }
  }

  // إذا تم الضغط على زر DOWN
  if (!countdownStarted && downButtonState) {
    if (currentNumber > 0) {
      currentNumber--; // تقليل الرقم
      EEPROM.write(0, currentNumber); // تخزين الرقم في EEPROM
      soundBuzzer(100); // إصدار صوت عادي عند الضغط على زر DOWN
      delay(200); // تأخير لمنع التكرار السريع
    }
  }

  // تحديث الشاشة
  display_N(currentNumber);

  // الانتقال من NC إلى NO
  if (limitSwitchState == HIGH && limitSwitchStateLast == LOW) { // الانتقال من NC إلى NO
    countdownStarted = true; // بدء العد التنازلي
    previousMillis = millis(); // بدء العد التنازلي
    startMotor(); // تشغيل الموتور
  }

  limitSwitchStateLast = limitSwitchState; // تحديث الحالة الأخيرة لمفتاح الحد
}

// تشغيل الموتور
void startMotor() {
  digitalWrite(motorControlPin, HIGH); // تفعيل الترانزستور
  Serial.println("Motor Started");
}

// إيقاف الموتور
void stopMotor() {
  digitalWrite(motorControlPin, LOW); // إيقاف الترانزستور
  Serial.println("Motor Stopped");
}

// إصدار صوت من الجرس
void soundBuzzer(int duration) {
  digitalWrite(buzzerPin, HIGH); // تشغيل الجرس
  delay(duration); // مدة التشغيل
  digitalWrite(buzzerPin, LOW); // إيقاف الجرس
}

// عرض الرقم على الشاشة
void display_N(int num) {
  // استخراج الوحدات والعشرات
  int units = num % 10;
  int tens = (num / 10) % 10;

  // عرض الوحدات والعشرات على الشاشة
  segOutput(1, units, 1);  // عرض الوحدات
  segOutput(0, tens, 1);   // عرض العشرات
}

// مسح الشاشة
void segClear() {
  for (int i = 0; i < 8; i++) {
    digitalWrite(segPin[i], HIGH);        
  }
}

// إخراج القيم للشاشة
void segOutput(int digit, int number, int dp) {
  segClear(); // مسح الشاشة أولاً
  digitalWrite(digitPin[digit], HIGH); // تفعيل الديجيت المحدد

  // عرض الرقم
  for (int i = 0; i < 7; i++) {
    digitalWrite(segPin[i], segValue[number][i]); // إخراج القيم
  }

  if (dp == 1) {
    digitalWrite(segPin[7], LOW); // إذا كانت النقطة العشرية مفعلة
  } else {
    digitalWrite(segPin[7], HIGH); // إذا كانت النقطة العشرية غير مفعلة
  }z

  delay(5); // فترة العرض
  digitalWrite(digitPin[digit], LOW); // إيقاف تشغيل الديجيت المحدد
}