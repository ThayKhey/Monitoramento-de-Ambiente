#include <dummy.h>
//Autor: Fábio Henrique Cabrini
//Resumo: Esse programa possibilita ligar e desligar o led onboard, além de mandar o status para o Broker MQTT possibilitando o Helix saber
//se o led está ligado ou desligado.
//Revisões:
//Rev1: 26-08-2023 Código portado para o ESP32 e para realizar a leitura de luminosidade e publicar o valor em um tópico aprorpiado do broker 
//Autor Rev1: Kheyla Thais Q. Paucara
//Rev2: 28-10-2024 Ajustes para o funcionamento no FIWARE Descomplicado



#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>                       // Biblioteca DHT Sensor Adafruit 
#include <DHT.h>
#include <DHT_U.h>


#define DHTTYPE    DHT11                           // Sensor DHT11
//#define DHTTYPE      DHT22                       // Sensor DHT22 ou AM2302
#define DHTPIN 4                                   // Pino do Arduino conectado no Sensor(Data) 
DHT_Unified dht(DHTPIN, DHTTYPE);                  // configurando o Sensor DHT - pino e tipo
uint32_t delayMS;                                  // variável para atraso no tempo

// Configurações - variáveis editáveis
const char* default_SSID = "FIAP-IBM"; // Nome da rede Wi-Fi
const char* default_PASSWORD = "Challenge@24!"; // Senha da rede Wi-Fi
const char* default_BROKER_MQTT = "34.203.196.154";
//Removi por motivos de segurança o ip do Broker
const int default_BROKER_PORT = 1883; // Porta do Broker MQTT
const char* default_TOPICO_SUBSCRIBE = "/TEF/lamp055/cmd"; // Tópico MQTT de escuta
const char* default_TOPICO_PUBLISH_1 = "/TEF/lamp055/attrs"; // Tópico MQTT de envio de informações para Broker
const char* default_TOPICO_PUBLISH_2 = "/TEF/lamp055/attrs/l"; // Tópico MQTT de envio de informações para Broker
const char* default_TOPICO_PUBLISH_3 = "/TEF/lamp055/attrs/t"; // Tópico MQTT de envio de informações para Broker
const char* default_TOPICO_PUBLISH_4 = "/TEF/lamp055/attrs/u"; // Tópico MQTT de envio de informações para Broker
const char* default_ID_MQTT = "fiware_055"; // ID MQTT
const int default_D4 = 2; // Pino do LED onboard
// Declaração da variável para o prefixo do tópico
const char* topicPrefix = "lamp055";

// Variáveis para configurações editáveis
char* SSID = const_cast<char*>(default_SSID);
char* PASSWORD = const_cast<char*>(default_PASSWORD);
char* BROKER_MQTT = const_cast<char*>(default_BROKER_MQTT);
int BROKER_PORT = default_BROKER_PORT;
char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE);
char* TOPICO_PUBLISH_1 = const_cast<char*>(default_TOPICO_PUBLISH_1);
char* TOPICO_PUBLISH_2 = const_cast<char*>(default_TOPICO_PUBLISH_2);
char* TOPICO_PUBLISH_3 = const_cast<char*>(default_TOPICO_PUBLISH_3);
char* TOPICO_PUBLISH_4 = const_cast<char*>(default_TOPICO_PUBLISH_4);
char* ID_MQTT = const_cast<char*>(default_ID_MQTT);
int D4 = default_D4;
WiFiClient espClient;
PubSubClient MQTT(espClient);
char EstadoSaida = '0';

void initSerial() {
    Serial.begin(115200);
}

void initWiFi() {
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    reconectWiFi();
}

void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);
    MQTT.setCallback(mqtt_callback);
}

void setup() {
    InitOutput();
    initSerial();
    initWiFi();
    initMQTT();
    delay(5000);
    MQTT.publish(TOPICO_PUBLISH_1, "s|on");

    Serial.begin(115200);                             // monitor serial 9600 bps
    dht.begin();                                    // inicializa a função
    Serial.println("Usando o Sensor DHT");
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);           // imprime os detalhes do Sensor de Temperatura
    Serial.println("------------------------------------");
    Serial.println("Temperatura");
    Serial.print  ("Sensor:       "); Serial.println(sensor.name);
    Serial.print  ("Valor max:    "); Serial.print(sensor.max_value); Serial.println(" *C");
    Serial.print  ("Valor min:    "); Serial.print(sensor.min_value); Serial.println(" *C");
    Serial.print  ("Resolucao:   "); Serial.print(sensor.resolution); Serial.println(" *C");
    Serial.println("------------------------------------");
    dht.humidity().getSensor(&sensor);            // imprime os detalhes do Sensor de Umidade
    Serial.println("------------------------------------");
    Serial.println("Umidade");
    Serial.print  ("Sensor:       "); Serial.println(sensor.name);
    Serial.print  ("Valor max:    "); Serial.print(sensor.max_value); Serial.println("%");
    Serial.print  ("Valor min:    "); Serial.print(sensor.min_value); Serial.println("%");
    Serial.print  ("Resolucao:   "); Serial.print(sensor.resolution); Serial.println("%");
    Serial.println("------------------------------------");
    delayMS = sensor.min_delay / 5000;            // define o atraso entre as leituras
}

void loop() {
    VerificaConexoesWiFIEMQTT();
    EnviaEstadoOutputMQTT();
    handleLuminosity();
    MQTT.loop();
    
    delay(delayMS);                               // atraso entre as medições
    sensors_event_t event;                        // inicializa o evento da Temperatura
    dht.temperature().getEvent(&event);           // faz a leitura da Temperatura
    if (isnan(event.temperature))                 // se algum erro na leitura
    {
      Serial.println("Erro na leitura da Temperatura!");
    }
    else                                          // senão
    {
      Serial.print("Temperatura: ");              // imprime a Temperatura
      Serial.print(event.temperature);
      Serial.println(" *C");
      String mensagem = String(event.temperature);
      MQTT.publish(TOPICO_PUBLISH_3, mensagem.c_str());
    }
    dht.humidity().getEvent(&event);              // faz a leitura de umidade
    if (isnan(event.relative_humidity))           // se algum erro na leitura
    {
      Serial.println("Erro na leitura da Umidade!");
    }
    else                                          // senão
    {
      Serial.print("Umidade: ");                  // imprime a Umidade
      Serial.print(event.relative_humidity);
      Serial.println("%");
      String mensagem = String(event.relative_humidity);
      MQTT.publish(TOPICO_PUBLISH_4, mensagem.c_str());
    }
}

void reconectWiFi() {
    if (WiFi.status() == WL_CONNECTED)
        return;
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());

    // Garantir que o LED inicie desligado
    digitalWrite(D4, LOW);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        char c = (char)payload[i];
        msg += c;
    }
    Serial.print("- Mensagem recebida: ");
    Serial.println(msg);

    // Forma o padrão de tópico para comparação
    String onTopic = String(topicPrefix) + "@on|";
    String offTopic = String(topicPrefix) + "@off|";

    // Compara com o tópico recebido
    if (msg.equals(onTopic)) {
        digitalWrite(D4, HIGH);
        EstadoSaida = '1';
    }

    if (msg.equals(offTopic)) {
        digitalWrite(D4, LOW);
        EstadoSaida = '0';
    }
}

void VerificaConexoesWiFIEMQTT() {
    if (!MQTT.connected())
        reconnectMQTT();
    reconectWiFi();
}

void EnviaEstadoOutputMQTT() {
    if (EstadoSaida == '1') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|on");
        Serial.println("- Led Ligado");
    }

    if (EstadoSaida == '0') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|off");
        Serial.println("- Led Desligado");
    }
    Serial.println("- Estado do LED onboard enviado ao broker!");
    delay(1000);
}

void InitOutput() {
    pinMode(D4, OUTPUT);
    digitalWrite(D4, HIGH);
    boolean toggle = false;

    for (int i = 0; i <= 10; i++) {
        toggle = !toggle;
        digitalWrite(D4, toggle);
        delay(200);
    }
}

void reconnectMQTT() {
    while (!MQTT.connected()) {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE);
        } else {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Haverá nova tentativa de conexão em 2s");
            delay(2000);
        }
    }
}

void handleLuminosity() {
    const int potPin = 34;
    int sensorValue = analogRead(potPin);
    int luminosity = map(sensorValue, 0, 4095, 0, 100);
    String mensagem = String(luminosity);
    Serial.print("Valor da luminosidade: ");
    Serial.println(mensagem.c_str());
    MQTT.publish(TOPICO_PUBLISH_2, mensagem.c_str());
}