#include <EEPROM.h>  // إضافة مكتبة EEPROM

// تعريف منافذ التوصيل للشاشة 7-segment
byte segPin[8] = {2, 3, 4, 5, 6, 7, 8, 9}; // {a,b,c,d,e,f,g,dp}
byte digitPin[2] = {A0, A1}; // منافذ الشاشة

// تعريف منافذ الأزرار
const int upButtonPin = A2;   // زر UP متصل بالمنفذ A2
const int downButtonPin = A3; // زر DOWN متصل بالمنفذ A3
const int startButtonPin = A4; // زر START متصل بالمنفذ A4

// تعريف limit switch
const int limitSwitchPin = A5; // limit switch متصل بالمنفذ A5
bool limitSwitchState = false; // متغير لتخزين حالة limit switch
bool limitSwitchPreviousState = false; // متغير لتخزين الحالة السابقة للـ limit switch

// تعريف منفذ التحكم في الموتور
const int motorControlPin = 10; // منفذ التحكم في الموتور (ترانزستور)

// تعريف منفذ الجرس
const int buzzerPin = 11; // منفذ الجرس

// تعريف قيم الأرقام من 0 إلى 9 على الشاشة 7-segment (كاثود مشترك)
byte segValue[10][7] = {
   {1, 1, 1, 1, 1, 1, 0}, //0
   {0, 1, 1, 0, 0, 0, 0}, //1
   {1, 1, 0, 1, 1, 0, 1}, //2
   {1, 1, 1, 1, 0, 0, 1}, //3
   {0, 1, 1, 0, 0, 1, 1}, //4
   {1, 0, 1, 1, 0, 1, 1}, //5
   {1, 0, 1, 1, 1, 1, 1}, //6
   {1, 1, 1, 0, 0, 0, 0}, //7
   {1, 1, 1, 1, 1, 1, 1}, //8
   {1, 1, 1, 1, 0, 1, 1}  //9
};

// تعريف قيم الحروف "O" و "F" على الشاشة 7-segment (كاثود مشترك)
byte segValueOF[2][7] = {
  {1, 1, 1, 1, 1, 1, 0}, // O
  {1, 0, 0, 0, 1, 1, 1}  // F
};

// تعريف قيم الحروف "d" و "r" على الشاشة 7-segment (كاثود مشترك)
byte segValueDR[2][7] = {
  {0, 1, 1, 1, 1, 0, 1}, // d
  {1, 1, 1, 0, 1, 1, 1}  // r
};

// متغيرات لتخزين الرقم الحالي وحالة العد التنازلي
int currentNumber = 0;  // الرقم الحالي
int lastNumber = 0;     // الرقم الأخير الذي اختاره المستخدم
bool countdownStarted = false;
bool displayOF = false;  // متغير للتحكم في عرض "OF"
bool displayDR = false;  // متغير للتحكم في عرض "dr"
unsigned long previousMillis = 0;
const long interval = 1000;  // 1 ثانية للعد التنازلي
bool limitSwitchPreviouslyPressed = false; // لتتبع ما إذا تم الضغط على الـ limit switch سابقًا

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
  // قراءة حالة الأزرار
  bool upButtonState = digitalRead(upButtonPin) == LOW;
  bool downButtonState = digitalRead(downButtonPin) == LOW;
  bool startButtonState = digitalRead(startButtonPin) == LOW; // حالة زر START

  // قراءة حالة الـ limit switch وتحديد ما إذا كان NO أو NC
  limitSwitchState = digitalRead(limitSwitchPin) == LOW; // في حال كان NO سيعود HIGH عند الفتح

  // التحقق من الضغط على زر START/STOP
  if (startButtonState) {
    if (countdownStarted) {
      countdownStarted = false; // إيقاف العد التنازلي
      stopMotor(); // إيقاف الموتور
      displayOF = true; // عرض "OF" على الشاشة
      Serial.println("Countdown Paused"); // طباعة رسالة للإيقاف
    } else {
      countdownStarted = true; // بدء العد التنازلي
      previousMillis = millis(); // بدء العد التنازلي
      startMotor(); // تشغيل الموتور
      displayOF = false; // عدم عرض "OF"
      soundBuzzer(600); // إصدار صوت عند بدء العد (600 مللي ثانية)
    }
    delay(200); // تأخير لمنع التكرار السريع
  }

  // التعامل مع الـ limit switch حسب حالته
  if (!limitSwitchState && limitSwitchPreviouslyPressed) {
    // في حال تم تغيير الـ limit switch إلى NO
    countdownStarted = true; // استئناف العد
    startMotor(); // تشغيل الموتور
    displayDR = false; // إخفاء "dr" على الشاشة
    soundBuzzer(600); // إصدار صوت عند تغيير حالة الـ limit switch
  }

  if (limitSwitchState && !limitSwitchPreviouslyPressed) {
    // في حال تم الضغط على limit switch، أوقف الموتور
    countdownStarted = false; // إيقاف العد
    stopMotor(); // إيقاف الموتور
    displayDR = true; // عرض "dr" على الشاشة
    soundBuzzer(600); // إصدار صوت عند الضغط على limit switch
  }

  limitSwitchPreviouslyPressed = limitSwitchState; // حفظ الحالة السابقة للـ limit switch

  // تحديث الشاشة حسب حالة العرض
  if (displayOF) {
    display_OF(); // عرض "OF" على الشاشة
  } else if (displayDR) {
    display_DR(); // عرض "dr" على الشاشة
  } else if (countdownStarted) {
    // التحقق من حالة العد التنازلي وتحديث الرقم
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

        // العودة إلى آخر رقم اختاره المستخدم
        currentNumber = lastNumber; // إعادة الرقم إلى آخر قيمة
        EEPROM.write(0, currentNumber); // تحديث الرقم في EEPROM
      }
    }
    display_N(currentNumber); // عرض الرقم الحالي أثناء العد التنازلي
  } else {
    // تحديث الشاشة بالرقم الحالي عند عدم تشغيل العد
    display_N(currentNumber);
  }

  // إذا تم الضغط على زر UP
  if (!countdownStarted && upButtonState) {
    if (currentNumber < 99) {
      currentNumber++; // زيادة الرقم
      lastNumber = currentNumber; // حفظ الرقم الأخير
      EEPROM.write(0, currentNumber); // تخزين الرقم في EEPROM
      soundBuzzer(100); // إصدار صوت عادي عند الضغط على زر UP
      delay(200); // تأخير لمنع التكرار السريع
    }
  }

  // إذا تم الضغط على زر DOWN
  if (!countdownStarted && downButtonState) {
    if (currentNumber > 0) {
      currentNumber--; // تقليل الرقم
      lastNumber = currentNumber; // حفظ الرقم الأخير
      EEPROM.write(0, currentNumber); // تخزين الرقم في EEPROM
      soundBuzzer(100); // إصدار صوت عادي عند الضغط على زر DOWN
      delay(200); // تأخير لمنع التكرار السريع
    }
  }
}

// دالة لعرض الرقم على الشاشة 7-segment
void display_N(int num) {
  int tens = num / 10; // العشرات
  int ones = num % 10; // الآحاد

  for (int i = 0; i < 7; i++) {
    digitalWrite(segPin[i], segValue[tens][i]);
  }
  digitalWrite(digitPin[0], LOW);  // تشغيل الرقم الأول (العشرات)
  delay(5);
  digitalWrite(digitPin[0], HIGH); // إيقاف الرقم الأول

  for (int i = 0; i < 7; i++) {
    digitalWrite(segPin[i], segValue[ones][i]);
  }
  digitalWrite(digitPin[1], LOW);  // تشغيل الرقم الثاني (الآحاد)
  delay(5);
  digitalWrite(digitPin[1], HIGH); // إيقاف الرقم الثاني
}

// دالة لعرض "OF" على الشاشة
void display_OF() {
  for (int i = 0; i < 7; i++) {
    digitalWrite(segPin[i], segValueOF[0][i]);
  }
  digitalWrite(digitPin[0], LOW);
  delay(5);
  digitalWrite(digitPin[0], HIGH);
  
  for (int i = 0; i < 7; i++) {
    digitalWrite(segPin[i], segValueOF[1][i]);
  }
  digitalWrite(digitPin[1], LOW);
  delay(5);
  digitalWrite(digitPin[1], HIGH);
}

// دالة لعرض "dr" على الشاشة
void display_DR() {
  for (int i = 0; i < 7; i++) {
    digitalWrite(segPin[i], segValueDR[0][i]);
  }
  digitalWrite(digitPin[0], LOW);
  delay(5);
  digitalWrite(digitPin[0], HIGH);
  
  for (int i = 0; i < 7; i++) {
    digitalWrite(segPin[i], segValueDR[1][i]);
  }
  digitalWrite(digitPin[1], LOW);
  delay(5);
  digitalWrite(digitPin[1], HIGH);
}

// دالة لإصدار صوت من الجرس
void soundBuzzer(int duration) {
  digitalWrite(buzzerPin, HIGH); // تشغيل الجرس
  delay(duration);               // تأخير لمدة الصوت
  digitalWrite(buzzerPin, LOW);  // إيقاف الجرس
}

// دالة لتشغيل الموتور
void startMotor() {
  digitalWrite(motorControlPin, HIGH); // تشغيل الموتور
}

// دالة لإيقاف الموتور
void stopMotor() {
  digitalWrite(motorControlPin, LOW); // إيقاف الموتور
}