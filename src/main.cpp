#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class Display {
  public:
    virtual ~Display() = default;

    void render(unsigned short x, unsigned short y) {
      clear();
      draw(x, y);
      update();
    }

  protected:
    virtual void clear() = 0;
    virtual void draw(unsigned short x, unsigned short y) = 0;
    virtual void update() = 0;
};

class OLEDDisplay: public Display {
  friend class OLEDMatrix;

  private:
    Adafruit_SSD1306 display;
    unsigned short pin_sck;
    unsigned short pin_sda;
    bool isInitialized = false; // Guarda de seguridad

    void selectBus() {
      Wire.end();                   // Libera el periférico I2C activo
      Wire.begin(pin_sda, pin_sck); // Reasigna los nuevos pines sin error
      Wire.setClock(400000);        // Vuelve a aplicar Fast Mode (400 kHz)
    }

  public:
    static constexpr unsigned short pxWidth = 128;
    static constexpr unsigned short pxHeight = 64;
  
    OLEDDisplay(unsigned short sck, unsigned short sda) : 
      pin_sck(sck), pin_sda(sda), display(pxWidth, pxHeight, &Wire, -1) {}

    bool initialize(uint8_t i2cAddr = 0x3C) {
      selectBus();
      Wire.setClock(400000);
      
      // Intentamos inicializar y guardamos el estado
      isInitialized = display.begin(SSD1306_SWITCHCAPVCC, i2cAddr);
      
      if (!isInitialized) {
        Serial.printf("  ❌ [FALLO] OLED en SDA: %d | SCK: %d\n", pin_sda, pin_sck);
      } else {
        Serial.printf("  ✅ [OK]    OLED en SDA: %d | SCK: %d\n", pin_sda, pin_sck);
      }
      return isInitialized;
    }

  protected:
    void clear() override {
      display.clearDisplay();
    }

    void draw(unsigned short x, unsigned short y) override {
      if (x >= pxWidth || y >= pxHeight) return;
      display.drawPixel(x, y, SSD1306_WHITE);
    }

    void update() override {
      selectBus();
      display.display();
    }
};

class OLEDMatrix: public Display {
  private:
    unsigned short matrixRows;
    unsigned short matrixColumns;
    OLEDDisplay** displays;

    OLEDDisplay* calculateWhichDisplayShouldRender(unsigned short x, unsigned short y) {
      if (x >= getVirtualWidth() || y >= getVirtualHeight()) return nullptr;
      
      unsigned short col = x / OLEDDisplay::pxWidth;
      unsigned short row = y / OLEDDisplay::pxHeight;
      return displays[(row * matrixColumns) + col];
    }

  public:
    OLEDMatrix(OLEDDisplay** displays, unsigned short matrixRows = 1, unsigned short matrixColumns = 4)
      : displays(displays), matrixRows(matrixRows), matrixColumns(matrixColumns) {}

    bool initialize(uint8_t i2cAddr = 0x3C) {
      bool error = false;
      forEachDisplay([&error, i2cAddr](OLEDDisplay* d) {
        if (!d->initialize(i2cAddr)) error = true;
      });
      return error;
    }
    
    unsigned short getVirtualHeight() const { return matrixRows * OLEDDisplay::pxHeight; }
    unsigned short getVirtualWidth() const { return matrixColumns * OLEDDisplay::pxWidth; }

    template <typename Action>
    void forEachDisplay(Action action) {
      size_t total = matrixColumns * matrixRows;
      for (size_t i = 0; i < total; i++) action(displays[i]);
    }

  protected:
    void clear() override {
      forEachDisplay([](OLEDDisplay* d) { d->clear(); });
    }

    void update() override {
      forEachDisplay([](OLEDDisplay* d) { d->update(); });
    }

    void draw(unsigned short x, unsigned short y) override {
      // Dibujamos un bloque de 16x16 píxeles en lugar de 1 solo píxel
      for (unsigned short dx = 0; dx < 16; dx++) {
        for (unsigned short dy = 0; dy < 16; dy++) {
          unsigned short targetX = x + dx;
          unsigned short targetY = y + dy;

          OLEDDisplay* display = calculateWhichDisplayShouldRender(targetX, targetY);
          if (display != nullptr) {
            display->draw(targetX % OLEDDisplay::pxWidth, targetY % OLEDDisplay::pxHeight);
          }
        }
      }
    }
};

unsigned short GPIO_11 = 11, GPIO_10 = 10;
unsigned short GPIO_9  = 9,  GPIO_8  = 8;
unsigned short GPIO_5  = 5,  GPIO_4  = 4;
unsigned short GPIO_2  = 2,  GPIO_1  = 1;

OLEDDisplay oledDisplay1(GPIO_11, GPIO_10);
// OLEDDisplay oledDisplay2(GPIO_9, GPIO_8);
// OLEDDisplay oledDisplay3(GPIO_5, GPIO_4);
// OLEDDisplay oledDisplay4(GPIO_2, GPIO_1);

// OLEDDisplay* displaysList[4] = { &oledDisplay1, &oledDisplay2, &oledDisplay3, &oledDisplay4 };
// OLEDMatrix oledMatrix(displaysList);

void setup() {
  Serial.begin(115200);
  // if (oledMatrix.initialize()) {
  //   Serial.println("Error de inicialización en pantallas.");
  // }
  if (!oledDisplay1.initialize()) {
    Serial.println("No se pudo iniciar la pantalla 1. Revisa conexiones.");
  }
}

void loop() {
  // oledMatrix.render(pelotaX, 24);
  Serial.println("theendisnevertheend");

  oledDisplay1.render(10,10);
  // pelotaX += dirX;
  // if (pelotaX <= 0 || pelotaX >= oledMatrix.getVirtualWidth() - 16) {
  //   dirX = -dirX;
  // }

  delay(15);
}