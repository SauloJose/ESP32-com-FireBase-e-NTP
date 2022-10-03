//carregando bibliotecas do projeto
#include <WiFi.h>                  //Conexão WiFi
#include <IOXhop_FirebaseESP32.h>  //Biblioetca para manipular dados do Firebase
#include <sys/time.h>              //Biblioteca para dados do relógio
#include "time.h"

//defines para conectar a internet
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

//Para acessar dados do firebase
#define FIREBASE_HOST ""
#define FIREBASE_AUTH ""


const char* ntpServer = "pool.ntp.org"; //host que irá retornar o tempo Unix para meu micro.


//variáveis globais para ajudar o programa
int indiceColeta = 0;
String tituloColeta = "coleta_01";

void setup() {
  Serial.begin(115200);
  delay(100);

  //Conectar ao WiFi
  conectaWiFi();

  //Iniciar conexão com o FireBase e trocar dados
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  //Obter dados de data e hora no formato Unix/Pos do servidor
  Serial.println("Obtendo informações de data e hora do servidor");
  
  /*String timeUnixString = Firebase.getString("config/TempoUnix");

  //Remove parte dos millisegundos
  timeUnixString.remove(timeUnixString.length() - 3, 3);

  //Obtem o fuso horário
  String fusoHorarioString = Firebase.getString("config/FusoHorario");

  //Converte instante para um inteiro de 32 bits com sinal
  int32_t timeUnix = atoi(timeUnixString.c_str());

  //Converte o fuso para um inteiro de 32 bits com sinal
  int32_t fusoHorario = (atoi(fusoHorarioString.c_str()) * 3600);

  //Acrescenta o fuso horáio ao instante
  timeUnix = timeUnix + fusoHorario;*/
  
  uint32_t timeUnix; //Criando variável de tempo

  //Obtem o fuso horário
  String fusoHorarioString = Firebase.getString("config/FusoHorario");
  
  int32_t fusoHorario = (atoi(fusoHorarioString.c_str()) * 3600);//Puxando fuso horário

  configTime(0, 0, ntpServer);//Irá puxar o horário para o timer interno do ESP32.

  timeUnix = getTime(); // pegando valor Unix

  Firebase.setString("config/TempoUnix",String(timeUnix));//Enviando para o servidor o horário coletado

  timeUnix=timeUnix+fusoHorario;//corrigindo horário para utilizar no esp, devido ao fusohorário

  
  //ajuste o instante no esp32
  ajustaTempoUnix(timeUnix);
  
  Serial.println("Testando normalmente e vendo se funciona.");
  //envia para a serial o instante em um formato amigável
  mostraTempo();
}

void loop() {
  conectaWiFi();  //Verifica e estabelece conxão com WiFi
  //Serial.println("Estou funcionando");

  //Obtem string da coleta
  String estado = Firebase.getString("config/EstadoColeta");

  //Obtem título da coleta
  String tituloColetaAtual = Firebase.getString("config/TituloColeta");

  //se a coleta tem um novo nome, reinicia contador do índice
  if (tituloColeta != tituloColetaAtual) indiceColeta = 0;

  //Atribui o nome atual à coleta
  tituloColeta = tituloColetaAtual;
  
  Serial.println("Titulo de coleta: " + tituloColeta + "/estado: " + estado + "/Titulo de coleta Atual: " + tituloColetaAtual);
  
  if (estado == "ativo") {
    atualizaDados();
    delay(100);
  }
  if (estado == "pausado") {
    delay(1000);
    return;
  }
  if (estado == "remover") {
    Firebase.setString("config/EstadoColeta", "pausado");
    delay(100);
    Firebase.setString(tituloColeta, "deletando...");
    delay(100);
    Firebase.remove(tituloColeta);
    return;
  }
}

//Escrevendo as funções
void atualizaDados() {
  const size_t capacity = JSON_OBJECT_SIZE(3);

  //Cria um buffer para o json
  DynamicJsonBuffer jsonBuffer(capacity);

  //Cria um objeto para representar o json
  JsonObject& json = jsonBuffer.createObject();

  //Gerar valores aleatórios para tensão e corrente apenas como teste
  float tensao = random(118, 137);
  float corrente = random(18, 32);

  //Armazena instante atual
  time_t instanteAtual = obtemInstante();

  //Monta o json
  json["tensão (V)"] = tensao;
  json["Corrente (A)"] = corrente;
  json["Instante (Unix_s)"] = instanteAtual;

  Firebase.set("tensao", tensao);
  if (Firebase.failed()) {
    Serial.println("Erro ao ajustar tensao.");
    Serial.println(Firebase.error());
    return;
  }

  //Atualiza campo independente de corrente
  Firebase.set("corrente", corrente);
  if (Firebase.failed()) {
    Serial.println("Erro ao ajustar corrente");
    Serial.println(Firebase.error());
    return;
  }

  //Atualiza o registro de coleta com um json montado e avalia uma falha;
  //Note que necessito do índice do registro.
  Firebase.set(tituloColeta + "/" + String(indiceColeta), json);  //Cria um novo registro no endereço tituloColeta
  if (Firebase.failed()) {
    Serial.println("Erro ajustando " + String(tituloColeta));
    Serial.println(Firebase.error());
    return;
  }

  //incrementa o indice da coleta
  indiceColeta++;
  Serial.println("\n Indice da coleta: " + String(indiceColeta));
}

int ajustaTempoUnix(int32_t unixtime) {
  struct timeval epoca = { (unixtime), 0 };
  return settimeofday((const timeval*)&epoca, NULL);
}

void mostraTempo() {
  struct tm agora;
  getLocalTime(&agora, 0);
  Serial.println(&agora, "%d %B %Y %H: %M: %S (%A)");
}

time_t obtemInstante() {
  struct timeval tempo;

  gettimeofday(&tempo, NULL);

  return tempo.tv_sec;
}

void conectaWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("conectando");

    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
    }

    Serial.println();
    Serial.print("Conectado em: ");
    Serial.println(WiFi.localIP());
  }
}

uint32_t getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}
