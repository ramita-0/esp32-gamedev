# Dual-ESP32 Gaming Engine (CPU / GPU Architecture)

Este repositorio contiene el código fuente para el motor de juegos y sistema de procesamiento gráfico basado en una arquitectura distribuida de dos microcontroladores ESP32 trabajando en paralelo.

## 1. Arquitectura del Sistema

El sistema replica la división de responsabilidades de las consolas de videojuegos clásicas:

* **CPU (NodeMCU ESP32 - LX6 Dual-Core):**
  * **Motor de Juego y Física:** Procesa reglas de juego, colisiones, vectores de movimiento, puntuación y estados globales (Pausa, Game Over).
  * **Gestor de Entradas (Inputs):** Mantiene la pila de Bluetooth Classic (BR/EDR) para conectar mandos físicos (ej. DualShock 4 de PS4) mediante `bluepad32`.
  * **Expansión I/O:** Controla periféricos externos directos como buzzers paso a paso, displays de 7 segmentos, LEDs de estado o matriz de botones.
  * **Transmisión:** Empaqueta y envía el estado del mundo por UART Serie.

* **GPU (ESP32-S3 - LX7 Dual-Core):**
  * **Motor Gráfico & Rasterizado:** Almacena la librería de sprites, mapas de nivel (*tilemaps*) y fuentes en memoria Flash/RAM.
  * **Renderizado I2C Paralelo:** Administra la matriz de pantallas (4x SSD1306 OLED) usando buses hardware independientes.
  * **Recepción:** Lee el paquete de estado más reciente desde la UART y renderiza los fotogramas en tiempo real.

---

## 2. Protocolo de Comunicación Inter-Chip (UART State Sync)

La comunicación entre placas se realiza mediante **UART por Hardware a 921,600 baudios** conectando el pin `TX` del CPU al pin `RX` de la GPU (compartiendo GND).

### Estrategia de Sincronización
Para evitar el *input lag* o el atascamiento de datos (*buffer bloat*), no se usan sockets de red ni colas FIFO extensas. La GPU implementa el patrón **State Sync**:
1. El CPU transmite la foto del mundo (`GameState`) a una tasa constante de 60 Hz.
2. La GPU vacía el buffer UART de entrada descartando tramas viejas si existiese alguna demora.
3. La GPU procesa únicamente la versión de datos más reciente recibida para renderizar el cuadro en pantalla.

---

## 3. Estructura del Proyecto Unificado en PlatformIO

A través de un único repositorio y un archivo `platformio.ini`, se administran los dos entornos sin necesidad de duplicar código ni mantener dos proyectos separados.

### Estructura de Directorios

```text
├── platformio.ini
└── src/
    ├── common/
    │   └── protocol.h       <-- Estructura de datos empaquetada compartida
    ├── cpu/
    │   └── main_cpu.cpp     <-- Código del NodeMCU ESP32 (Lógica + BT + I/O)
    └── gpu/
        └── main_gpu.cpp     <-- Código del ESP32-S3 (I2C + Motor gráfico)
```

## 4. Configuración del `platformio.ini`

```toml
[platformio]
default_envs = esp32s3_gpu, nodemcu_cpu

# --- ENTORNO GPU (ESP32-S3) ---
[env:esp32s3_gpu]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
build_src_filter = +<gpu/> +<common/>

# --- ENTORNO CPU (NodeMCU ESP32) ---
[env:nodemcu_cpu]
platform = espressif32
board = nodemcu-32s
framework = arduino
monitor_speed = 115200
lib_deps = 
    bluepad32
build_src_filter = +<cpu/> +<common/>
```

## 5. Código de Protocolo Compartido (`src/common/protocol.h`)

```cpp
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <Arduino.h>

// Estructura empaquetada binaria para evitar alineamiento de bytes inconsistente
struct __attribute__((packed)) GameState {
  uint8_t header = 0xAA; // Byte de sincronización
  int8_t playerX;
  int8_t playerY;
  uint8_t playerState;
  uint16_t score;
  uint16_t buttons;
};

#endif
```

## 6. Comandos de Compilación y Carga

Para flashear cada microcontrolador desde la CLI de PlatformIO o la extensión de VS Code:  
```bash
pio run -e esp32s3_gpu -t upload
```

Cargar firmware al CPU (NodeMCU ESP32):
```bash
pio run -e nodemcu_cpu -t upload
```

Abrir el monitor serie para el CPU:
```bash
pio device monitor -e nodemcu_cpu
```

## 7. Pendientes y Futuras Investigaciones (Roadmap)

### Optimización de Comunicación I2C (Matriz de Displays):

- Investigar e implementar la apertura de dos canales hardware I2C en paralelo (`Wire` y `Wire1`) en la GPU (ESP32-S3).

- Mapear 2 pantallas OLED SSD1306 por canal para transmitir buffers simultáneamente y elevar la tasa de refresco a 35+ FPS a frecuencias de 800 kHz.

### Profundización en Arquitectura de Hardware de los Chips:

- Analizar las especificaciones internas de ambos SoC (frecuencias de reloj, cores Xtensa LX6 vs. LX7, memoria SRAM, caché e instrucciones vectoriales/PIE del ESP32-S3).

- Estudiar la transferencia por DMA (Direct Memory Access) para buses de pantalla sin bloqueo de CPU.

- Evaluar la asignación explícita de tareas a cada núcleo de los chips mediante FreeRTOS (`xTaskCreatePinnedToCore`) para separar, por ejemplo, el procesamiento de Bluetooth/UART en un core y la física/renderizado en el otro.
