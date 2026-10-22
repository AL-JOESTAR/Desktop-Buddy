
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==================================================
// PIN ESP32-C3 SUPER MINI
// ==================================================
#define REED_GPIO       5
#define MUSIC_PIN       4
#define MODE_BUTTON     6
#define SDA_PIN         8
#define SCL_PIN         9

// ==================================================
// OLED SSD1306
// ==================================================
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define OLED_ADDRESS    0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// ==================================================
// PENGATURAN
// ==================================================
#define DEBOUNCE_TIME       50
#define DOOR_MESSAGE_TIME   3000

#define MUSIC_THRESHOLD     1800
#define BEAT_INTERVAL       500
#define BEAT_PULSE          140

// ==================================================
// 4 MODE TAMPILAN
// 0 = Jam dan tanggal
// 1 = Robot
// 2 = Pintu
// 3 = Music visualizer
// ==================================================
const int TOTAL_MODES = 4;
int currentMode = 0;

// Notifikasi pintu otomatis
bool doorAlertActive = false;
unsigned long doorAlertStart = 0;

// ==================================================
// SENSOR PINTU / REED SWITCH
// ==================================================
bool lastReading = HIGH;
bool stableState = HIGH;

unsigned long lastDebounceTime = 0;

// ==================================================
// TOMBOL MODE
// ==================================================
bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;

unsigned long lastButtonDebounce = 0;

// ==================================================
// REFRESH DISPLAY
// ==================================================
unsigned long lastDisplayUpdate = 0;

// ==================================================
// ANIMASI ROBOT
// ==================================================
unsigned long lastBlinkTime = 0;
bool blinking = false;

// ==================================================
// JAM DAN TANGGAL SIMULASI
// ==================================================
int jam = 10;
int menit = 30;
int detik = 0;

int tanggal = 22;
int bulan = 10;
int tahun = 2026;

unsigned long lastClockUpdate = 0;

// ==================================================
// MUSIK SIMULASI DARI POTENTIOMETER
// ==================================================
int musicLevel = 0;
bool musicDetected = false;
bool beatPulse = false;

unsigned long beatStartTime = 0;
unsigned long beatNumber = 0;

// ==================================================
// UPDATE LEVEL MUSIK
// ==================================================
void updateMusic(unsigned long now) {
  musicLevel = analogRead(MUSIC_PIN);

  musicDetected = musicLevel > MUSIC_THRESHOLD;

  unsigned long elapsed = now - beatStartTime;

  beatNumber = elapsed / BEAT_INTERVAL;

  unsigned long beatPhase = elapsed % BEAT_INTERVAL;

  beatPulse = musicDetected &&
              (beatPhase < BEAT_PULSE);
}

// ==================================================
// KALENDER
// ==================================================
bool isLeapYear(int y) {
  return (y % 400 == 0) ||
         ((y % 4 == 0) && (y % 100 != 0));
}

int daysInMonth(int m, int y) {
  switch (m) {
    case 2:
      return isLeapYear(y) ? 29 : 28;

    case 4:
    case 6:
    case 9:
    case 11:
      return 30;

    default:
      return 31;
  }
}

// ==================================================
// UPDATE JAM DAN TANGGAL
// ==================================================
void updateClock(unsigned long now) {
  while (now - lastClockUpdate >= 1000) {
    lastClockUpdate += 1000;

    detik++;

    if (detik >= 60) {
      detik = 0;
      menit++;
    }

    if (menit >= 60) {
      menit = 0;
      jam++;
    }

    if (jam >= 24) {
      jam = 0;
      tanggal++;

      if (tanggal > daysInMonth(bulan, tahun)) {
        tanggal = 1;
        bulan++;

        if (bulan > 12) {
          bulan = 1;
          tahun++;
        }
      }
    }
  }
}

// ==================================================
// MODE 1: JAM DAN TANGGAL
// ==================================================
void drawClock() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(10, 16);

  display.printf(
    "%02d:%02d:%02d",
    jam, menit, detik
  );

  display.drawLine(
    10, 40, 118, 40, SSD1306_WHITE
  );

  display.setTextSize(1);
  display.setCursor(28, 48);

  display.printf(
    "%02d/%02d/%04d",
    tanggal, bulan, tahun
  );

  display.display();
}

// ==================================================
// MODE 2: MATA ROBOT
// ==================================================
void drawRobot(bool blink, bool beat) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (blink) {
    // Mata tertutup
    display.drawLine(
      18, 29, 48, 29, SSD1306_WHITE
    );

    display.drawLine(
      80, 29, 110, 29, SSD1306_WHITE
    );
  } else {
    // Mata membesar saat beat musik
    int eyeY = beat ? 8 : 11;
    int eyeH = beat ? 39 : 35;
    int pupil = beat ? 10 : 7;

    display.fillRoundRect(
      13, eyeY, 40, eyeH, 10, SSD1306_WHITE
    );

    display.fillRoundRect(
      75, eyeY, 40, eyeH, 10, SSD1306_WHITE
    );

    display.fillCircle(
      33, 28, pupil, SSD1306_BLACK
    );

    display.fillCircle(
      95, 28, pupil, SSD1306_BLACK
    );
  }

  // Mode robot hanya menampilkan mata
  display.display();
}

// ==================================================
// MODE 3 DAN NOTIFIKASI: PINTU
// ==================================================
void drawDoor() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (stableState == HIGH) {
    // Ikon pintu terbuka
    display.drawRect(
      8, 15, 25, 34, SSD1306_WHITE
    );

    display.drawLine(
      8, 15, 25, 22, SSD1306_WHITE
    );

    display.drawLine(
      25, 22, 25, 49, SSD1306_WHITE
    );

    display.fillCircle(
      21, 35, 1, SSD1306_WHITE
    );

    display.setTextSize(1);
    display.setCursor(42, 25);
    display.println("PINTU");

    display.setTextSize(2);
    display.setCursor(42, 37);
    display.println("TERBUKA");
  } else {
    // Ikon pintu tertutup
    display.drawRect(
      8, 15, 25, 34, SSD1306_WHITE
    );

    display.drawLine(
      8, 15, 33, 15, SSD1306_WHITE
    );

    display.fillCircle(
      27, 33, 2, SSD1306_WHITE
    );

    display.setTextSize(1);
    display.setCursor(42, 25);
    display.println("PINTU");

    display.setTextSize(2);
    display.setCursor(42, 37);
    display.println("TUTUP");
  }

  display.display();
}

// ==================================================
// MODE 4: MUSIC BEAT VISUALIZER
// ==================================================
void drawMusicVisualizer(unsigned long now) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(20, 0);
  display.println("MUSIC VISUALIZER");

  display.drawLine(
    0, 10, 127, 10, SSD1306_WHITE
  );

  // Level potentiometer
  int levelBar = map(musicLevel, 0, 4095, 0, 79);
  levelBar = constrain(levelBar, 0, 79);

  display.setCursor(0, 14);
  display.print("LVL:");
  display.print(musicLevel);

  display.drawRect(
    44, 13, 83, 9, SSD1306_WHITE
  );

  if (levelBar > 0) {
    display.fillRect(
      46, 15, levelBar, 5, SSD1306_WHITE
    );
  }

  // Equalizer
  unsigned long elapsed = now - beatStartTime;

  for (int i = 0; i < 12; i++) {
    int x = 4 + i * 10;
    int h;

    if (musicDetected && beatPulse) {
      h = 8 + ((i + beatNumber) % 4) * 4;
    } else if (musicDetected) {
      h = 4 + ((i * 3 + elapsed / 100) % 5) * 3;
    } else {
      h = 2 + ((i + elapsed / 150) % 3) * 2;
    }

    h = constrain(h, 2, 22);

    display.fillRect(
      x, 51 - h, 6, h, SSD1306_WHITE
    );
  }

  display.setCursor(20, 56);

  if (musicDetected && beatPulse) {
    display.print(">>> BEAT <<<");
  } else if (musicDetected) {
    display.print("MUSIC ACTIVE");
  } else {
    display.print("NO MUSIC");
  }

  display.display();
}

// ==================================================
// SETUP
// ==================================================
void setup() {
  Serial.begin(115200);

  pinMode(REED_GPIO, INPUT_PULLUP);
  pinMode(MODE_BUTTON, INPUT_PULLUP);
  pinMode(MUSIC_PIN, INPUT);

  analogReadResolution(12);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS)) {
    Serial.println("OLED tidak ditemukan!");

    while (true) {
      delay(10);
    }
  }

  unsigned long now = millis();

  stableState = digitalRead(REED_GPIO);
  lastReading = stableState;

  lastButtonReading = digitalRead(MODE_BUTTON);
  stableButtonState = lastButtonReading;

  lastDebounceTime = now;
  lastButtonDebounce = now;

  lastClockUpdate = now;
  lastDisplayUpdate = 0;
  lastBlinkTime = now;
  beatStartTime = now;

  currentMode = 0;
  doorAlertActive = false;

  // Tampilan awal adalah jam
  drawClock();

  Serial.println("SISTEM AKTIF");
  Serial.println("GPIO 5 = Reed switch");
  Serial.println("GPIO 6 = Tombol MODE");
  Serial.println("GPIO 4 = Potentiometer musik");
  Serial.println("Mode awal: JAM DAN TANGGAL");
}

// ==================================================
// LOOP UTAMA
// ==================================================
void loop() {
  unsigned long now = millis();

  // 1. Update jam dan level musik
  updateClock(now);
  updateMusic(now);

  // 2. Baca reed switch
  bool reading = digitalRead(REED_GPIO);

  if (reading != lastReading) {
    lastReading = reading;
    lastDebounceTime = now;
  }

  // 3. Debounce dan deteksi perubahan pintu
  if (now - lastDebounceTime >= DEBOUNCE_TIME) {
    if (reading != stableState) {
      stableState = reading;

      // Aktifkan notifikasi otomatis selama 3 detik
      doorAlertActive = true;
      doorAlertStart = now;

      Serial.println(
        stableState == HIGH
          ? "PINTU TERBUKA"
          : "PINTU TERTUTUP"
      );
    }
  }

  // 4. Baca tombol MODE
  bool buttonReading = digitalRead(MODE_BUTTON);

  if (buttonReading != lastButtonReading) {
    lastButtonReading = buttonReading;
    lastButtonDebounce = now;
  }

  if (now - lastButtonDebounce >= DEBOUNCE_TIME) {
    if (buttonReading != stableButtonState) {
      stableButtonState = buttonReading;

      // Tombol ditekan
      if (stableButtonState == LOW) {
        currentMode = (currentMode + 1) % TOTAL_MODES;

        lastBlinkTime = now;

        Serial.print("Mode tampilan: ");
        Serial.println(currentMode);
      }
    }
  }

  // 5. Selesaikan notifikasi setelah 3 detik
  if (doorAlertActive &&
      now - doorAlertStart >= DOOR_MESSAGE_TIME) {
    doorAlertActive = false;
  }

  // 6. Animasi mata berkedip setiap 3 detik
  unsigned long blinkPhase = now - lastBlinkTime;

  if (blinkPhase >= 3000) {
    lastBlinkTime = now;
    blinkPhase = 0;
  }

  blinking = blinkPhase < 120;

  // 7. Refresh OLED
  if (now - lastDisplayUpdate >= 80) {
    lastDisplayUpdate = now;

    // Notifikasi pintu memiliki prioritas tertinggi.
    // currentMode tidak berubah selama notifikasi.
    if (doorAlertActive) {
      drawDoor();
    } else {
      switch (currentMode) {
        case 0:
          drawClock();
          break;

        case 1:
          drawRobot(
            blinking,
            musicDetected && beatPulse
          );
          break;

        case 2:
          drawDoor();
          break;

        case 3:
          drawMusicVisualizer(now);
          break;
      }
    }
  }

  delay(5);
}
