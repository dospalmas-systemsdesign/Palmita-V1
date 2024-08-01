/**************************************************************

  Ejemplo: Palmita Testing
  Consulta Información: -------
 **************************************************************/

// PINOUT PALMITA

#define RX_GSM 19
#define TX_GSM 23

// Seleccion Modem SIMCom SIM800L
#define TINY_GSM_MODEM_SIM800

// Ver todos los comandos AT, si lo desea
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG Serial

/*
   Pruebas habilitadas
*/

#define TINY_GSM_TEST_GPRS true
#define TINY_GSM_TEST_TCP true
#define TINY_GSM_TEST_SSL true
#define TINY_GSM_TEST_CALL true
#define TINY_GSM_TEST_SMS true
#define TINY_GSM_TEST_USSD true
#define TINY_GSM_TEST_BATTERY true
#define TINY_GSM_TEST_TEMPERATURE true
#define TINY_GSM_TEST_TIME true
#define TINY_GSM_POWERDOWN true

// Configurar el PIN GSM, si lo hubiera
#define GSM_PIN ""

// Establece números de teléfono, si quieres probar SMS y llamadas

#define SMS_TARGET "+529999999999"
#define CALL_TARGET "+529999999999"

// Tus credenciales GPRS, si las hay
const char apn[] = "internet.itelcel.com";
const char gprsUser[] = "webgprs";
const char gprsPass[] = "webgprs2003";

// Server details to test TCP/SSL
const char server[] = "vsh.pp.ua";
const char resource[] = "/TinyGSM/logo.txt";

#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(Serial1, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(Serial1);
#endif

void setup()
{
  // Establecer la velocidad en baudios de la consola
  Serial.begin(115200);
  delay(10);

  // !!!!!!!!!!!
  // Configure sus pines de reinicio, habilitación y encendido aquí
  // !!!!!!!!!!!

  // Establecer la velocidad en baudios del módulo GSM
  Serial1.begin(9600, SERIAL_8N1, RX_GSM, TX_GSM);
}

void loop()
{

  /**************************************************************
    Prueba: Prueba comandos AT
    Extra: -------
   **************************************************************/

  Serial.println("Probando Modem Test AT ...");
  if (!modem.testAT())
  {
    Serial.println("Modem Test AT ERROR");
    return;
  }

  Serial.println("Modem Test AT OK");

  /**************************************************************
    Prueba: Inicializar modem para conexion a red y gprs
    Extra: -------
   **************************************************************/

  Serial.println("Probando Modem Init ...");
  if (!modem.init())
  {
    Serial.println("Modem Init ERROR");
    return;
  }

  Serial.println("Modem Init OK");

  /**************************************************************
    Prueba: Información Modem
    Extra: -------
   **************************************************************/

  String name = modem.getModemName();
  DBG("Nombre Modem:", name);

  String modemInfo = modem.getModemInfo();
  DBG("Info Modem:", modemInfo);

  /**************************************************************
  Prueba: Conexion a red celular
  Extra: -------
 **************************************************************/

#if TINY_GSM_TEST_GPRS
  // Desbloquee su tarjeta SIM con un PIN si es necesario
  if (GSM_PIN && modem.getSimStatus() != 3)
  {
    modem.simUnlock(GSM_PIN);
  }
#endif

  DBG("Waiting for network...");
  if (!modem.waitForNetwork(600000L, true))
  {
    delay(10000);
    return;
  }

  if (modem.isNetworkConnected())
  {
    DBG("Network connected");
  }

  /**************************************************************
  Prueba: Conexion gprs
  Extra: -------
 **************************************************************/


#if TINY_GSM_TEST_GPRS
  DBG("Connecting to", apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass))
  {
    delay(10000);
    return;
  }

  bool res = modem.isGprsConnected();
  DBG("GPRS status:", res ? "connected" : "not connected");

  String ccid = modem.getSimCCID();
  DBG("CCID:", ccid);

  String imei = modem.getIMEI();
  DBG("IMEI:", imei);

  String imsi = modem.getIMSI();
  DBG("IMSI:", imsi);

  String cop = modem.getOperator();
  DBG("Operator:", cop);

  IPAddress local = modem.localIP();
  DBG("Local IP:", local);

  int csq = modem.getSignalQuality();
  DBG("Signal quality:", csq);
#endif

  /**************************************************************
  Prueba: Comandos USSD
  Extra: -------
 **************************************************************/


#if TINY_GSM_TEST_USSD && defined TINY_GSM_MODEM_HAS_SMS
  String ussd_balance = modem.sendUSSD("*111#");
  DBG("Balance (USSD):", ussd_balance);

  String ussd_phone_num = modem.sendUSSD("*161#");
  DBG("Phone number (USSD):", ussd_phone_num);
#endif


  /**************************************************************
  Prueba: Conexión TCP
  Extra: -------
 **************************************************************/


#if TINY_GSM_TEST_TCP && defined TINY_GSM_MODEM_HAS_TCP
  TinyGsmClient client(modem, 0);
  const int port = 80;
  DBG("Connecting to", server);
  if (!client.connect(server, port))
  {
    DBG("... failed");
  }
  else
  {
    // Make a HTTP GET request:
    client.print(String("GET ") + resource + " HTTP/1.0\r\n");
    client.print(String("Host: ") + server + "\r\n");
    client.print("Connection: close\r\n\r\n");

    // Wait for data to arrive
    uint32_t start = millis();
    while (client.connected() && !client.available() &&
           millis() - start < 30000L)
    {
      delay(100);
    };

    // Read data
    start = millis();
    char logo[640] = {
        '\0',
    };
    int read_chars = 0;
    while (client.connected() && millis() - start < 10000L)
    {
      while (client.available())
      {
        logo[read_chars] = client.read();
        logo[read_chars + 1] = '\0';
        read_chars++;
        start = millis();
      }
    }
    Serial.println(logo);
    DBG("#####  RECEIVED:", strlen(logo), "CHARACTERS");
    client.stop();
  }
#endif


  /**************************************************************
  Prueba: Conexión SSL
  Extra: -------
 **************************************************************/
#if TINY_GSM_TEST_SSL && defined TINY_GSM_MODEM_HAS_SSL
  TinyGsmClientSecure secureClient(modem, 1);
  const int securePort = 443;
  DBG("Connecting securely to", server);
  if (!secureClient.connect(server, securePort))
  {
    DBG("... failed");
  }
  else
  {
    // Make a HTTP GET request:
    secureClient.print(String("GET ") + resource + " HTTP/1.0\r\n");
    secureClient.print(String("Host: ") + server + "\r\n");
    secureClient.print("Connection: close\r\n\r\n");

    // Wait for data to arrive
    uint32_t startS = millis();
    while (secureClient.connected() && !secureClient.available() &&
           millis() - startS < 30000L)
    {
      delay(100);
    };

    // Read data
    startS = millis();
    char logoS[640] = {
        '\0',
    };
    int read_charsS = 0;
    while (secureClient.connected() && millis() - startS < 10000L)
    {
      while (secureClient.available())
      {
        logoS[read_charsS] = secureClient.read();
        logoS[read_charsS + 1] = '\0';
        read_charsS++;
        startS = millis();
      }
    }
    Serial.println(logoS);
    DBG("#####  RECEIVED:", strlen(logoS), "CHARACTERS");
    secureClient.stop();
  }
#endif

  /**************************************************************
  Prueba: Llamadas
  Extra: -------
 **************************************************************/

#if TINY_GSM_TEST_CALL && defined TINY_GSM_MODEM_HAS_CALLING && \
    defined CALL_TARGET
  DBG("Calling:", CALL_TARGET);

  // This is NOT supported on M590
  res = modem.callNumber(CALL_TARGET);
  DBG("Call:", res ? "OK" : "fail");

  if (res)
  {
    delay(1000L);

    // Play DTMF A, duration 1000ms
    modem.dtmfSend('A', 1000);

    // Play DTMF 0..4, default duration (100ms)
    for (char tone = '0'; tone <= '4'; tone++)
    {
      modem.dtmfSend(tone);
    }

    delay(5000);

    res = modem.callHangup();
    DBG("Hang up:", res ? "OK" : "fail");
  }
#endif

  /**************************************************************
  Prueba: Enviar SMS
  Extra: -------
 **************************************************************/


#if TINY_GSM_TEST_SMS && defined TINY_GSM_MODEM_HAS_SMS && defined SMS_TARGET
  bool sms = modem.sendSMS(SMS_TARGET, String("Hello from ") + imei);
  DBG("SMS:", sms ? "OK" : "fail");

  DBG("UTF8 SMS:", sms ? "OK" : "fail");

#endif

  /**************************************************************
  Prueba: Obtener Hora
  Extra: -------
 **************************************************************/


#if TINY_GSM_TEST_TIME && defined TINY_GSM_MODEM_HAS_TIME
  int year3 = 0;
  int month3 = 0;
  int day3 = 0;
  int hour3 = 0;
  int min3 = 0;
  int sec3 = 0;
  float timezone = 0;
  for (int8_t i = 5; i; i--)
  {
    DBG("Requesting current network time");
    if (modem.getNetworkTime(&year3, &month3, &day3, &hour3, &min3, &sec3,
                             &timezone))
    {
      DBG("Year:", year3, "\tMonth:", month3, "\tDay:", day3);
      DBG("Hour:", hour3, "\tMinute:", min3, "\tSecond:", sec3);
      DBG("Timezone:", timezone);
      break;
    }
    else
    {
      DBG("Couldn't get network time, retrying in 15s.");
      delay(15000L);
    }
  }
  DBG("Retrieving time again as a string");
  String time = modem.getGSMDateTime(DATE_FULL);
  DBG("Current Network Time:", time);
#endif

  /**************************************************************
  Prueba: Datos Bateria (opcional)
  Extra: -------
 **************************************************************/


#if TINY_GSM_TEST_BATTERY && defined TINY_GSM_MODEM_HAS_BATTERY
  uint8_t chargeState = -99;
  int8_t percent = -99;
  uint16_t milliVolts = -9999;
  modem.getBattStats(chargeState, percent, milliVolts);
  DBG("Battery charge state:", chargeState);
  DBG("Battery charge 'percent':", percent);
  DBG("Battery voltage:", milliVolts / 1000.0F);
#endif


  /**************************************************************
  Prueba: Temperatura (opcional)
  Extra: -------
 **************************************************************/
#if TINY_GSM_TEST_TEMPERATURE && defined TINY_GSM_MODEM_HAS_TEMPERATURE
  float temp = modem.getTemperature();
  DBG("Chip temperature:", temp);
#endif

  /**************************************************************
  Prueba: Apagar Modulo 
  Extra: -------
 **************************************************************/

#if TINY_GSM_POWERDOWN

#if TINY_GSM_TEST_GPRS
  modem.gprsDisconnect();
  delay(5000L);
  if (!modem.isGprsConnected())
  {
    DBG("GPRS disconnected");
  }
  else
  {
    DBG("GPRS disconnect: Failed.");
  }
#endif

  // Try to power-off (modem may decide to restart automatically)
  // To turn off modem completely, please use Reset/Enable pins
  modem.poweroff();
  DBG("Poweroff.");
#endif

  DBG("End of tests.");

  // Do nothing forevermore
  while (true)
  {
    modem.maintain();
  }
}
