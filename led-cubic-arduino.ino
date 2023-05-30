#include <SPI.h>
#include <esp32-hal-timer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#define MOSI 11
#define SCK 13
#define latch_pin 2
#define blank_pin 4

// Replace with your network credentials
char ssid[50];
char password[50];
char hostname[50];

// AP
char ap_ssid[50] = { 'l', 'e', 'd', '-', 'c', 'u', 'b', 'i', 'c', '\0' };
char ap_password[50] = "";

// Create an instance of the web server
WebServer server(80);

int shift_out;
byte anode[4];

// 用于存储 LED 亮度
byte  red0[8], red1[8], red2[8], red3[8];
byte  blue0[8], blue1[8], blue2[8], blue3[8];
byte  green0[8], green1[8], green2[8], green3[8];


int level = 0; // 用于立方层数
int anodelevel = 0; // 用于层数步进
int BAM_Bit, BAM_Counter = 0; // 步数跟踪变量

unsigned long start; // 用于毫秒计时器

hw_timer_t *timer = NULL;

void IRAM_ATTR refreshCube() {
  digitalWrite(blank_pin, HIGH);
  if (BAM_Counter == 8)
    BAM_Bit++;
  else if (BAM_Counter == 24)
    BAM_Bit++;
  else if (BAM_Counter == 56)
    BAM_Bit++;

  BAM_Counter++;

  switch (BAM_Bit) { 
    case 0:
      for (shift_out = level; shift_out < level + 2; shift_out++)
        SPI.transfer(red0[shift_out]);
      for (shift_out = level; shift_out < level + 2; shift_out++)
        SPI.transfer(green0[shift_out]);
      for (shift_out = level; shift_out < level + 2; shift_out++)
        SPI.transfer(blue0[shift_out]);
      break;
    case 1:
      for (shift_out = level; shift_out < level + 2; shift_out++)
        SPI.transfer(red1[shift_out]);
      for (shift_out = level; shift_out < level + 2; shift_out++)
        SPI.transfer(green1[shift_out]);
      for (shift_out = level; shift_out < level + 2; shift_out++)
        SPI.transfer(blue1[shift_out]);
      break;
    case 2:
      for (shift_out = level; shift_out < level + 2; shift_out++)
        SPI.transfer(red2[shift_out]);
      for (shift_out = level; shift_out < level + 2; shift_out++)
        SPI.transfer(green2[shift_out]);
      for (shift_out = level; shift_out < level + 2; shift_out++)
        SPI.transfer(blue2[shift_out]);
      break;
    case 3:
      for (shift_out = level; shift_out < level + 2; shift_out++)
        SPI.transfer(red3[shift_out]);
      for (shift_out = level; shift_out < level + 2; shift_out++)
        SPI.transfer(green3[shift_out]);
      for (shift_out = level; shift_out < level + 2; shift_out++)
        SPI.transfer(blue3[shift_out]);
     
      if (BAM_Counter == 120) {
        BAM_Counter = 0;
        BAM_Bit = 0;
      }
      break;
  }

  SPI.transfer(anode[anodelevel]);

  // 刷新 74HC595 数据
  digitalWrite(latch_pin, HIGH);
  digitalWrite(latch_pin, LOW);
  digitalWrite(blank_pin, LOW);

  anodelevel++;
  level = level + 2; 

  if (anodelevel == 4)
    anodelevel = 0;
  if (level == 8) 
    level = 0;
  pinMode(blank_pin, OUTPUT);
}

void setup() {
  Serial.begin(115200);
  Serial.println("start up");
  WiFi.softAP(ap_ssid, ap_password);

  // Initialize the mDNS
  if (!MDNS.begin("ledcubic")) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started.");
    Serial.print("mDNS hostname: ");
    Serial.println("ledcubic");
  }
  //MDNS.addService("http","tcp",80);

  // Serve the HTML page with the text box to modify net worth
  server.on("/", HTTP_GET, handleRoot);

  server.on("/", HTTP_POST, handleRoot);

  // Define and assign the function that will be called when the text box is modified
  //server.on("/", HTTP_POST, handleMoney);

  server.onNotFound(handleNotFound);

  //server.sendHeader("Content-Type", "text/html; charset=utf-8");

  // Start the web server
  server.begin();
  Serial.println("Web server start");

  SPI.setBitOrder(MSBFIRST); // Most Significant Bit First
  SPI.setDataMode(SPI_MODE0); // Mode 0 Rising edge of data, keep clock low
  SPI.setClockDivider(SPI_CLOCK_DIV2); // Run the data in at 16MHz/2 - 8MHz

  // 计时器1 用于刷新 LED 立方
  timer = timerBegin(1, 80, true); // timer1, prescale = 80, count up
  timerAttachInterrupt(timer, &refreshCube, true); // attach ISR
  timerAlarmWrite(timer, 120, true); // set alarm value, autoreload = true
  timerAlarmEnable(timer); // enable alarm

  anode[0] = B00000001;
  anode[1] = B00000010;
  anode[2] = B00000100;
  anode[3] = B00001000;

  pinMode(latch_pin, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(SCK, OUTPUT);
  pinMode(blank_pin, OUTPUT);
  SPI.begin(SCK, -1, MOSI, -1);
  Serial.println("ok");
  // 自检
  check();
}

void loop() {
  server.handleClient();
  //check();
  //blinKing();
  //movePlane();
  //moveSingle();
  //moveSqure();
  //moveOnePixel();
  //allLeds(); 
  //planeSwipe();
  //randomLeds();
}

void handleRoot() {
  String message = "<html><head><meta charset='utf-8' name='viewport' content='width=device-width, initial-scale=1.0'>";
  message += "<style>";
  message += "html.dark {";
  message += "  background-color: #222;";
  message += "  color: #fff;";
  message += "}";
  message += ".dark h1,";
  message += ".dark p {";
  message += "  color: #fff;";
  message += "}";
  message += ".dark input[type=\"number\"] {";
  message += "  color: #fff;";
  message += "  background-color: #444;";
  message += "}";
  message += ".dark input[type=\"submit\"] {";
  message += "  color: #fff;";
  message += "  background-color: #333;";
  message += "  font-size: 20px;";
  message += "  padding: 10px 20px;";
  message += "}";
  message += ".dark .theme-toggle {";
  message += "  color: #fff;";
  message += "  background-color: #333;";
  message += "  border: 2px solid #fff;";
  message += "}";
  message += ".theme-toggle {";
  message += "  color: #333;";
  message += "  background-color: #fff;";
  message += "  border: 2px solid #333;";
  message += "  border-radius: 5px;";
  message += "  padding: 10px;";
  message += "  cursor: pointer;";
  message += "}";
  message += ".theme-toggle-right {";
  message += "  position: absolute;";
  message += "  top: 10px;";
  message += "  right: 10px;";
  message += "}";
  message += "</style>";
  message += "<script>";
  message += "function toggleTheme() {";
  message += "  document.documentElement.classList.toggle('dark');";
  message += "  if (document.documentElement.classList.contains('dark')) {";
  message += "    localStorage.setItem('theme', 'dark');";
  message += "  } else {";
  message += "    localStorage.setItem('theme', 'light');";
  message += "  }";
  message += "}";
  message += "var theme = localStorage.getItem('theme');";
  message += "if (theme) {";
  message += "  document.documentElement.classList.add(theme);";
  message += "}";
  message += "</script>";
  message += "</head><body>";
  message += "<a href='/setting' class='page-toggle-left'>Settings</a>";
  message += "<button class='theme-toggle theme-toggle-right' onclick='toggleTheme()'>切换主题</button>";
  message += "<div style='text-align:center; padding-top:60px;'>";
  message += "<h1>LED 立方控制台</h1>";
  message += "<form method='POST' action='/'>";
  message += "<label for='action'>选择样式: </label>";
  message += "<select name='action' id='action'>";
  message += "<option value='check'>自检</option>";
  message += "<option value='blinKing'>闪烁</option>";
  message += "<option value='movePlane'>跑马灯</option>";
  message += "<option value='moveSingle'>鞭炮</option>";
  message += "<option value='moveSqure'>推箱子</option>";
  message += "<option value='moveOnePixel'>快闪</option>";
  message += "<option value='allLeds'>呼吸灯</option>";
  message += "<option value='planeSwipe'>流水灯</option>";
  message += "<option value='randomLeds'>炫彩</option>";
  message += "</select>";
  message += "<br><br>";
  message += "<input style='display:block; margin:20px auto; font-size: 20px; padding: 10px 20px; text-align:center;' type='submit' value='提交'>";
  message += "</form>";
  message += "</div>";

  if (server.method() == HTTP_POST) {
    String action = server.arg("action");
    if (action == "check") {
      check();
    } else if (action == "blinKing") {
      blinKing();
    } else if (action == "movePlane") {
      movePlane();
    } else if (action == "moveSingle") {
      moveSingle();
    } else if (action == "moveSqure") {
      moveSqure();
    } else if (action == "moveOnePixel") {
      moveOnePixel();
    } else if (action == "allLeds") {
      allLeds();
    } else if (action == "planeSwipe") {
      planeSwipe();
    } else if (action == "randomLeds") {
      randomLeds();
    }
  }
  message += "</body></html>";
  server.send(200, "text/html", message);
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not found");
}

// 自检
void check() {
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      for (int k = 0; k < 4; k++) {
        LED(i, j, k, 15, 15, 15);
      }
  delay(5000);
  clean();
}

// 闪烁
void blinKing() {
  start = millis();
  unsigned long previousMillis = start;
  unsigned long interval = 1000;
  while (millis() - start < 11000) {
    if (millis() - previousMillis >= interval) {
      previousMillis = millis();
      toggle();
    }
  }
  clean();
}

void toggle() {
  static bool state = true;
  int ii, jj, kk;
  for (ii = 0; ii < 4; ii++)
    for (jj = 0; jj < 4; jj++)
      for (kk = 0; kk < 4; kk++)
        LED(ii, jj, kk, state ? 15 : 0, state ? 15 : 0, state ? 15 : 0);
  state = !state;
}

// 炫彩
void randomLeds() {
  Serial.println("Random Leds");
  int x, y, z, red, green, blue;
  start = millis();

  while (millis() - start < 5000) {
    x = random(4);
    y = random(4);
    z = random(4);
    red = random(16);
    green = random(16);
    blue = random(16);
    LED(x, y, z, red, green, blue);

    delay(20);
  }
  clean();
}

void planeSwipe() {
  Serial.println("Plane Swipe");
  for (int i = 0; i < 3; i++) {
    int child = 0;
    for (int j = 0; j < 4; j++) {
      if (j >= (3 - i)) {
        for (int k = 0; k < 4; k++)
          LED(k, j, child, 0, 15, 0);
        child++;
      } else {
        for (int k = 0; k < 4; k++)
          LED(k, j, child, 0, 15, 0);
      }
    }
    delay(100);
    clean();
  }
  for (int i = 0; i < 4; i++) {
    int child = 0;
    for (int j = 0; j < 4; j++) {
      if (j >= i) {
        for (int k = 0; k < 4; k++)
          LED(k, child, j , 0, 15, 0);
        child++;
      } else {
        for (int k = 0; k < 4; k++)
          LED(k, child, j, 0, 15, 0);
      }
    }
    delay(100);
    clean();
  }
  for (int i = 0; i < 3; i++) {
    int child = 0;
    for (int j = 0; j < 4; j++) {
      if (j >= (3 - i)) {
        for (int k = 0; k < 4; k++)
          LED(k, child, j, 0, 15, 0);
        child++;
      } else {
        for (int k = 0; k < 4; k++)
          LED(k, child, j, 0, 15, 0);
      }
    }
    delay(100);
    clean();
  }
  for (int i = 0; i < 4; i++) {
    int child = 0;
    for (int j = 0; j < 4; j++) {
      if (j >= i) {
        for (int k = 0; k < 4; k++)
          LED(k, j, child , 0, 15, 0);
        child++;
      } else {
        for (int k = 0; k < 4; k++)
          LED(k, j, child, 0, 15, 0);
      }
    }
    delay(100);
    clean();
  }
  ///////////////
  for (int i = 0; i < 3; i++) {
    int child = 0;
    for (int j = 0; j < 4; j++) {
      if (j >= (3 - i)) {
        for (int k = 0; k < 4; k++)
          LED(j, child, k,  0, 0, 15);
        child++;
      } else {
        for (int k = 0; k < 4; k++)
          LED(j, child, k,   0, 0, 15);
      }
    }
    delay(100);
    clean();
  }
  for (int i = 0; i < 4; i++) {
    int child = 0;
    for (int j = 0; j < 4; j++) {
      if (j >= i) {
        for (int k = 0; k < 4; k++)
          LED(child, j , k,  0, 0, 15);
        child++;
      } else {
        for (int k = 0; k < 4; k++)
          LED(child, j, k , 0, 0, 15);
      }
    }
    delay(100);
    clean();
  }
  for (int i = 0; i < 3; i++) {
    int child = 0;
    for (int j = 0; j < 4; j++) {
      if (j >= (3 - i)) {
        for (int k = 0; k < 4; k++)
          LED(child, j, k,   0, 0, 15);
        child++;
      } else {
        for (int k = 0; k < 4; k++)
          LED( child, j, k,  0, 0, 15);
      }
    }
    delay(100);
    clean();
  }
  for (int i = 0; i < 4; i++) {
    int child = 0;
    for (int j = 0; j < 4; j++) {
      if (j >= i) {
        for (int k = 0; k < 4; k++)
          LED(j, child , k,  0, 0, 15);
        child++;
      } else {
        for (int k = 0; k < 4; k++)
          LED(j, child, k, 0, 0, 15);
      }
    }
    delay(100);
    clean();
  }
  //////////////////////
  for (int i = 0; i < 3; i++) {
    int child = 0;
    for (int j = 0; j < 4; j++) {
      if (j >= (3 - i)) {
        for (int k = 0; k < 4; k++)
          LED(j, k, child,  15, 0, 0);
        child++;
      } else {
        for (int k = 0; k < 4; k++)
          LED(j, k, child,   15, 0, 0);
      }
    }
    delay(100);
    clean();
  }
  for (int i = 0; i < 4; i++) {
    int child = 0;
    for (int j = 0; j < 4; j++) {
      if (j >= i) {
        for (int k = 0; k < 4; k++)
          LED(child, k, j ,  15, 0, 0);
        child++;
      } else {
        for (int k = 0; k < 4; k++)
          LED(child, k , j, 15, 0, 0);
      }
    }
    delay(100);
    clean();
  }
  for (int i = 0; i < 3; i++) {
    int child = 0;
    for (int j = 0; j < 4; j++) {
      if (j >= (3 - i)) {
        for (int k = 0; k < 4; k++)
          LED( child, k, j,  15, 0, 0);
        child++;
      } else {
        for (int k = 0; k < 4; k++)
          LED( child, k, j,  15, 0, 0);
      }
    }
    delay(100);
    clean();
  }
  for (int i = 0; i < 4; i++) {
    int child = 0;
    for (int j = 0; j < 4; j++) {
      if (j >= i) {
        for (int k = 0; k < 4; k++)
          LED(j , k, child,  15, 0, 0);
        child++;
      } else {
        for (int k = 0; k < 4; k++)
          LED( j, k, child, 15, 0, 0);
      }
    }
    delay(100);
    clean();
  }
}

void movePlane() {
  Serial.println("Move Plane");
  start = millis();

  while (millis() - start < 5000) {
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++)
        for (int k = 0; k < 4; k++) {
          LED(i, k, j, 15, 0, 0);
          LED(j, i, k, 0, 10, 0);
          LED(j, k, i, 0, 0, 10);
        }

      delay(100);
      clean();
    }
  }
}

void moveSingle() {
  Serial.println("Move Single");
  start = millis();

  while (millis() - start < 5000) {
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        for (int k = 0; k < 4; k++) {
          LED(i, j, k, 15, 0, 15);
          LED(j, i, k, 15, 0, 15);
          LED(i, k, j, 0, 15, 15);
          delay(50);
          clean();
        }
  }
}

void moveOnePixel() {
  Serial.println("Move OnePixel");
  start = millis();
  int layer, column, row, red, green, blue;
  while (millis() - start < 5000) {
    layer = random(4);
    column = random(4);
    row = random(4);
    red = random(16);
    green = random(16);
    blue = random(16);
    LED(layer, column, row, red, green, blue);
    LED(column, layer, row, red, green, blue);
    LED(row , layer, column ,  red, green, blue);
    delay(50);
    clean();
  }
}

void moveSqure() {
  Serial.println("Move Squre");
  start = millis();

  while (millis() - start < 5000) {
    int red = random(15);
    int green = random(15);
    int blue = random(15);
    LED(1, 1, 1, red, green, blue);
    LED(1, 1, 2, red, green, blue);
    LED(1, 2, 1,  red, green, blue);
    LED(1, 2, 2,  red, green, blue);
    LED(2, 1, 1,  red, green, blue);
    LED(2, 1, 2,  red, green, blue);
    LED(2, 2, 1,  red, green, blue);
    LED(2, 2, 2,  red, green, blue);
    delay(200);
    clean();
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        LED(i, j, 0,  red, green, blue);
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        LED(i, 0, j,  red, green, blue);
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        LED(0, i, j,  red, green, blue);
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        LED(i, j, 3, red, green, blue);
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        LED(i, 3, j, red, green, blue);
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        LED(3, i, j, red, green, blue);

    delay(200);
    clean();
  }
}

// 呼吸灯
void allLeds() {
  for (int brightness = 0; brightness < 16; brightness++) {
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        for (int k = 0; k < 4; k++) {
          LED(i, j, k, brightness, 0, 0);
        }
    delay(30);
  }
  delay(300);
  for (int brightness = 15; brightness >= 0; brightness--) {
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        for (int k = 0; k < 4; k++) {
          LED(i, j, k, brightness, 0, 0);
        }
    delay(30);
  }
  for (int brightness = 0; brightness < 16; brightness++) {
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        for (int k = 0; k < 4; k++) {
          LED(i, j, k, 0, brightness, 0);
        }
    delay(30);
  }
  delay(300);
  for (int brightness = 15; brightness >= 0; brightness--) {
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        for (int k = 0; k < 4; k++) {
          LED(i, j, k, 0, brightness, 0);
        }
    delay(30);
  }
  for (int brightness = 0; brightness < 16; brightness++) {
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        for (int k = 0; k < 4; k++) {
          LED(i, j, k, 0, 0, brightness);
        }
    delay(30);
  }
  delay(300);
  for (int brightness = 15; brightness >= 0; brightness--) {
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        for (int k = 0; k < 4; k++) {
          LED(i, j, k, 0, 0, brightness);
        }
    delay(30);
  }
}

void LED(int level, int row, int column, byte red, byte green, byte blue) {
  int whichbyte = int(((level * 16) + (row * 4) + column) / 8);

  int wholebyte = (level * 16) + (row * 4) + column;

  bitWrite(red0[whichbyte], wholebyte - (8 * whichbyte), bitRead(red, 0));
  bitWrite(red1[whichbyte], wholebyte - (8 * whichbyte), bitRead(red, 1));
  bitWrite(red2[whichbyte], wholebyte - (8 * whichbyte), bitRead(red, 2));
  bitWrite(red3[whichbyte], wholebyte - (8 * whichbyte), bitRead(red, 3));

  bitWrite(green0[whichbyte], wholebyte - (8 * whichbyte), bitRead(green, 0));
  bitWrite(green1[whichbyte], wholebyte - (8 * whichbyte), bitRead(green, 1));
  bitWrite(green2[whichbyte], wholebyte - (8 * whichbyte), bitRead(green, 2));
  bitWrite(green3[whichbyte], wholebyte - (8 * whichbyte), bitRead(green, 3));

  bitWrite(blue0[whichbyte], wholebyte - (8 * whichbyte), bitRead(blue, 0));
  bitWrite(blue1[whichbyte], wholebyte - (8 * whichbyte), bitRead(blue, 1));
  bitWrite(blue2[whichbyte], wholebyte - (8 * whichbyte), bitRead(blue, 2));
  bitWrite(blue3[whichbyte], wholebyte - (8 * whichbyte), bitRead(blue, 3));
}

void clean() {
  int ii, jj, kk;
  for (ii = 0; ii < 4; ii++)
    for (jj = 0; jj < 4; jj++)
      for (kk = 0; kk < 4; kk++)
        LED(ii, jj, kk, 0, 0, 0);
}
