#include <SoftwareSerial.h>
SoftwareSerial esp8266(9, 8); // RX, TX
boolean conectado;
long int tempoA;
void setup()  
{
  Serial.begin(9600);
  esp8266.begin(9600);  
  conectado = false;
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
  String result = serialSend("AT+RST\r\n",5000);
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
void verificarAcao(){
  char *requisicao = "GET /esp/index.php?t=123&h=123 HTTP/1.1\r\nHost: 192.168.0.101:80\r\n\r\n";
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
  Serial.println(resposta);
}

String serialSend(String cmd, const int timeout)
{
  uint8_t buffer[512] = {0};
  int i = 0;
  String response = "";
  esp8266.print(cmd);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (esp8266.available())
    {
        buffer[i] = esp8266.read();
        i++;
    }
  }
  for (int i =0; i < 512 ; i++) {    
    if (buffer[i]=='\0') {
      return response; 
    }
    response += (char)buffer[i];
  }
  return response;
}

