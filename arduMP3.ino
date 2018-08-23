// Importaciones
#include <SoftwareSerial.h>
#include <DFMiniMp3.h>
#include "IRLremote.h"
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

// Definiciones
#define pinIR 2
#define I2C_ADDRESS 0x3C // Normalmente 0x3C o 0x3D
#define RST_PIN -1

// Variables
bool reproduciendo = false;
bool encendido = false;
int volumen;
int ficheros;
int cancionActual;
String estado;
int eq;
String eqMode;

// Protocolo IR. Seleccionar el que interese...
CNec IRLremote;
// CPanasonic IRLremote;
// CHashIR IRLremote;
//#define IRLremote Sony12

// Clase que permite el módulo MP3 notificar ciertos eventos
class Mp3Notify
{
  public:
    static void OnError(uint16_t errorCode)
    {
      // see DfMp3_Error for code meaning
      Serial.println();
      Serial.print("Com Error ");
      Serial.println(errorCode);
    }

    static void OnPlayFinished(uint16_t globalTrack)
    {
      Serial.println();
      Serial.print("Play finished for #");
      Serial.println(globalTrack);
    }

    static void OnCardOnline(uint16_t code)
    {
      Serial.println();
      Serial.print("Card online ");
      Serial.println(code);
    }

    static void OnCardInserted(uint16_t code)
    {
      Serial.println();
      Serial.print("Card inserted ");
      Serial.println(code);
    }

    static void OnCardRemoved(uint16_t code)
    {
      Serial.println();
      Serial.print("Card removed ");
      Serial.println(code);
    }
};

// Objeto que modela el reproductor MP3
SoftwareSerial secondarySerial(10, 11); // RX, TX
DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(secondarySerial);

// Objeto que modela la pantalla SSD130
SSD1306AsciiWire oled;

void setup()
{
  Serial.begin(115200);
  Serial.println("Inicializando...");

  // Para poder utilizar I2C
  Wire.begin();
  Wire.setClock(400000L);

  // Iniciamos la pantalla
  #if RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
  #endif // RST_PIN >= 0

  // Establecemos la fuente
  oled.setFont(font8x8); // ver posibles fuentes en libraries/SSD1306Ascii/doc/html/index.html

  // Iniciamos sensor de infrarrojos.
  if (!IRLremote.begin(pinIR)){
    Serial.println(F("Verifique el PIN de conexión.")); 
  }

  // Iniciamos el reproductor MP3
  mp3.begin();
  configuracionInicio();

  Serial.println("Iniciado!");
}

void configuracionInicio() {
  volumen = 15;
  mp3.setVolume(volumen);
  ficheros = mp3.getTotalTrackCount();
  Serial.print("Ficheros: ");
  Serial.println(ficheros);
  cancionActual = 0;
  estado = "Stop    ";
  eq = 0;
  mp3.setEq(eq);
  eqMode = "Normal";
}

void loop()
{
  // esto permite recibir notificaciones sin interrupciones
  mp3.loop();

  // Hemos recibido algo en el sensor IR?
  if (IRLremote.available())
  {
    auto data = IRLremote.read();

    // Imprimimos la información en el puerto serie
    Serial.print(F("Address: 0x"));
    Serial.println(data.address, HEX);
    Serial.print(F("Command: 0x"));
    Serial.println(data.command, HEX);
    Serial.println();

    switch (data.command) // en función del comando recibido...
    { case 0x12:
        if (encendido) {
          encendido = false;
          mp3.sleep();
          muestraADormirPantalla();
          delay(2000);
          oled.clear();
        } else {
          encendido = true;
          mp3.reset();
          configuracionInicio();
          muestraADespertarPantalla();
          delay(2000);
          muestraDatosPantalla();
        }
        break;
      case 0x1E:
        Serial.println("Subir volumen");
        if (volumen < 30) {
          volumen ++;
          mp3.setVolume(volumen);
        }
        break;
      case 0x1B:
        Serial.println("Bajar volumen");
        if (volumen > 0) {
          volumen --;
          mp3.setVolume(volumen);
        }
        break;
      case 0xE:
        Serial.println("Siguiente canción");
        if (cancionActual < ficheros) {
          cancionActual ++;
          mp3.playGlobalTrack(cancionActual);
          estado = "Play   ";
        }
        break;
      case 0x1A:
        Serial.println("Anterior canción");
        if (cancionActual > 1) {
          cancionActual --;
          mp3.playGlobalTrack(cancionActual);
          estado = "Play   ";
        }
        break;
      case 0x8:
        Serial.println("Stop");
        mp3.stop();
        reproduciendo = false;
        estado = "Stop    ";
        break;
      case 0x5:
        if (reproduciendo) {
          reproduciendo = false;
          estado = "Pause   ";
          mp3.pause();
        } else {
          reproduciendo = true;
          estado = "Play   ";
          mp3.start();
        }
        break;
      case 0x15:
        if (eq < 5) {
          eq ++;
        } else {
          eq = 0;
        }
        mp3.setEq(eq);
        switch (eq) {
          case 0:
            eqMode = "Normal ";
            break;
          case 1:
            eqMode = "Pop    ";
            break;
          case 2:
            eqMode = "Rock   ";
            break;
          case 3:
            eqMode = "Jazz   ";
            break;
          case 4:
            eqMode = "Classic";
            break;
          case 5:
            eqMode = "Bass   ";
            break;
        }

    }
    if (encendido) {
      muestraDatosPantalla();
    }
  }
  delay(200);
}

void muestraDatosPantalla() {
  oled.setCursor(0, 0);
  oled.set2X();
  oled.print("ReproMP3");
  oled.set1X();
  oled.setCursor(0, 3);
  oled.print("Vol: ");
  if (volumen < 10) {
    oled.print("0");
  }
  oled.print(volumen);
  oled.setCursor(0, 4);
  oled.print("Fichero: ");
  if (cancionActual < 100) {
    oled.print("0");
  }
  if (cancionActual < 10) {
    oled.print("0");
  }
  oled.print(cancionActual);
  oled.print("/");
  if (ficheros < 100) {
    oled.print("0");
  }
  if (ficheros < 10) {
    oled.print("0");
  }
  oled.print(ficheros);
  oled.setCursor(0, 5);
  oled.print("Estado: ");
  oled.print(estado);
  oled.setCursor(0, 6);
  oled.print("EQ: ");
  oled.print(eqMode);
}

void muestraADormirPantalla() {
  oled.clear();
  oled.setCursor(3, 0);
  oled.set2X();
  oled.print("A dormir");
}

void muestraADespertarPantalla() {
  oled.clear();
  oled.setCursor(3, 0);
  oled.set2X();
  oled.print("Inicio");
}


