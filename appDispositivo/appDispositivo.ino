#include <SoftwareSerial.h>
#include <DHT.h>
SoftwareSerial esp8266(9, 8); // RX, TX
boolean conectado;
long int tempoA;
#define porta2  2
#define DHTPIN 7
#define DHTTYPE DHT22  
DHT dht(DHTPIN, DHTTYPE);
boolean inUse;

void setup()  
{
  pinMode(2,OUTPUT);
  digitalWrite(porta2,LOW);
  Serial.begin(9600);
  esp8266.begin(9600);  
  conectado = false;
  inUse = false;
  delay(3000);
  while(!startEspTest()){
    delay(5000);
    reiniciaESP();
  }
  Serial.println(getEspStatus());
  conectar();
  tempoA = millis();
  Serial.println("Inicializado!!");
}

String getEspStatus(){
  String result = serialSend("AT+CIPSTATUS\r\n",3000);
  if (result.indexOf("STATUS:2")>-1)
    return "Aguardando";  
  if (result.indexOf("STATUS:3")>-1)
    return "Conectado";
  if (result.indexOf("STATUS:4")>-1)
    return "Desconectado";  
}
void reiniciaESP(){
  serialSend("AT+RST\r\n",5000);
  String result = serialSend("AT+GMR\r\n",2000);
  Serial.println(result);
}

boolean startEspTest(){
  String result = serialSend("AT\r\n",3000);
  if ( result.indexOf("OK") > -1 ) {
    return true; 
  }
  return false;  
}
void conectar(){
  serialSend("AT+CWJAP=\"JARVIS_ENIAC\",\"sincondrose\"\r\n", 5000);
  serialSend("AT+CWMODE=1\r\n", 3000);
  String result = serialSend("AT+CIFSR\r\n", 5000);
  serialSend("AT+CIPMUX=0\r\n", 3000);
  Serial.println(result);
}

void loop() // run over and over
{
  if ( (tempoA+5000) < millis() ) {
    tempoA = millis();
    verificarAcao();
  }
}

void callAction(String acao) {

  if ( acao == "L2"  ) 
  {
        //ligar a porta 2
        digitalWrite(porta2,HIGH);
        Serial.println("Ligando porta 2");
  } else 
  if ( acao == "D2" ) 
  {
        //desligar a porta 2
        digitalWrite(porta2,LOW);      
        Serial.println("Desligando porta 2");
  } else 
  if ( acao == "S2" ) 
  {
      //Status da porta 2
      if (digitalRead(2)) {
        //ligado
        Serial.println("Ligado");
      } else {
        //desligado 
        Serial.println("Desligado");
      }
  } else 
  if (acao == "GT" )
  {
    delay(10000);
      //get temperatura
    registerEvent("READED_TEMPERATURE",(String)getTemperatura());
  } else 
  if ( acao == "GU" )
  {
      //get umidade
      Serial.println(getUmidade());
  } else {
      // fazer nada
  } 
}

void verificarAcao(){
  if (!inUse) {
    inUse = true;
    char *requisicao = "GET /avws/interface-controller/request-actions/ HTTP/1.1\r\nHost: 192.168.0.101:80\r\n\r\n";
    String resposta = "";
    String cmdResp = serialSend("AT+CIPSTART=\"TCP\",\"192.168.0.101\",80\r\n", 1000);
    if (cmdResp.indexOf("OK")) {
      serialSend("AT+CIPSEND=",20);  
      char textToWrite[16];
      uint32_t len = strlen(requisicao);
      sprintf(textToWrite,"%lu",len);
      serialSend(textToWrite,20);
      serialSend("\r\n",20);
      resposta = serialSend(requisicao,5000);
      serialSend("AT+CIPCLOSE\r\n",1000);
    }
    if (resposta.indexOf("act\":")>-1) {
       int idx = resposta.indexOf("act\":");
       String acao = resposta.substring((idx+6),(idx+8));
      callAction(acao); 
    }
    Serial.println(resposta);
    inUse = false;
  }
}
void registerEvent(String eventName,String value){
  int nlen = eventName.length();
  nlen+=value.length();
  nlen+=15;
  String req = "POST /avws/interface-controller/register-event/ HTTP/1.1\r\nHost: 192.168.0.101:80\r\n";
  req.concat("Content-Type: application/x-www-form-urlencoded\r\n");
  req.concat("Content-Length: ");
  req.concat((String)nlen);
  req.concat("\r\nCache-Control: no-cache\r\n\r\n");
  req.concat("ID=5cFe&EN=");
  req.concat(eventName);
  req.concat("&VL=");
  req.concat(value);
  req.concat("\r\n");
  String ret = "";
  String cmdResp = serialSend("AT+CIPSTART=\"TCP\",\"192.168.0.101\",80\r\n", 1000);
  if (cmdResp.indexOf("OK")) {
    serialSend("AT+CIPSEND=",20);  
    char textToWrite[16];
    uint32_t len = req.length();
    sprintf(textToWrite,"%lu",len);
    serialSend(textToWrite,20);
    serialSend("\r\n",20);
    ret = serialSend(req,10000);
    serialSend("AT+CIPCLOSE\r\n",1000);
  }
}
String serialSend(String cmd, const int timeout)
{
  uint8_t buffer[255] = {0};
  int i = 0;
  String response = "";
  esp8266.print(cmd);
  long int time = millis();
  while ( (time + timeout) > millis() )
  {
    while (esp8266.available())
    {
        buffer[i] = esp8266.read();
        i++;
    }
  }
  for (int i =0; i < 255 ; i++) {    
    if (buffer[i]=='\0') {
      return response; 
    }
    response += (char)buffer[i];
  }
  return response;
}

float getUmidade(){
   float h = dht.readHumidity();
  if (isnan(h))
  {
    return -1.0;
  }
  return h;
}
float getTemperatura(){
  float t = dht.readTemperature();
  if (isnan(t))
  {
    return 0.0;
  }
  return t;
}
