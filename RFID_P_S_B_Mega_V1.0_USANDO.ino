/*
   --------------------------------------------------------------------------------------------------------------------
   Example sketch/program showing An Arduino Door Access Control featuring RFID, EEPROM, Relay
   --------------------------------------------------------------------------------------------------------------------
   This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid

   This example showing a complete Door Access Control System

  Simple Work Flow (not limited to) :
                                     +---------+
  +----------------------------------->READ TAGS+^------------------------------------------+
  |                              +--------------------+                                     |
  |                              |                    |                                     |
  |                              |                    |                                     |
  |                         +----v-----+        +-----v----+                                |
  |                         |MASTER TAG|        |OTHER TAGS|                                |
  |                         +--+-------+        ++-------------+                            |
  |                            |                 |             |                            |
  |                            |                 |             |                            |
  |                      +-----v---+        +----v----+   +----v------+                     |
  |         +------------+READ TAGS+---+    |KNOWN TAG|   |UNKNOWN TAG|                     |
  |         |            +-+-------+   |    +-----------+ +------------------+              |
  |         |              |           |                |                    |              |
  |    +----v-----+   +----v----+   +--v--------+     +-v----------+  +------v----+         |
  |    |MASTER TAG|   |KNOWN TAG|   |UNKNOWN TAG|     |GRANT ACCESS|  |DENY ACCESS|         |
  |    +----------+   +---+-----+   +-----+-----+     +-----+------+  +-----+-----+         |
  |                       |               |                 |               |               |
  |       +----+     +----v------+     +--v---+             |               +--------------->
  +-------+EXIT|     |DELETE FROM|     |ADD TO|             |                               |
          +----+     |  EEPROM   |     |EEPROM|             |                               |
                     +-----------+     +------+             +-------------------------------+

//--------------------------------------------------------------------------------------------

///---------MONTAGEM DO RFID MFRC522 PORTAO  SOCIAL PLACA/SHIELD MEGA 2560
   RST/Reset   RST          5       |||||     (marcacao do fio com traço vermelho)   
   SPI SS      SDA(SS)      53      |      
   SPI MOSI    MOSI         51      |||
   SPI MISO    MISO         50      ||||
   SPI SCK     SCK          52      ||
   GND----------------------------PRETO
   VCC-3V3------------------------VERMELHO

   LCD 20X4
   SDA----PIN-20
   SCL----PIN-21
-----------------------------------------------------
//---LED RGB ANODO COMUM (+)
redLed   6    // D6--LED VERMELHO
greenLed 7    // D7--LED VERDE
blueLed  8    // D8--LED AZUL

relay 12     // D12-- Relay usado para acionar a bobina da fechadura do portao social--
wipeB 3     // D3--Button pin for WipeMode // Ativar o resistor pull up do pino
BUZZER 22    // D22--buzzer pin (+)//----------GND(-) (MODELO SL-2305)
BUTTON_EXTERNO  27 // D-27 botao externo (CONTATO NA),(usando pullup interno ativado, COM RESISTOR 10K EXTERNO). botao ao lado do portao social( button led rgb)
//----- Create MFRC522 instance para Arduino Mega 2560.
SS_PIN 53   ///-----SDA(SS) 
RST_PIN 5   ///-----RST




-------------DEFINIÇAO DE PULLUP:
   Ao conectar um sensor a um pino configurado com INPUT_PULLUP, a outra extremidade deve ser conectada ao aterramento. 
   No caso de uma chave simples, isso faz com que a leitura do pino seja HIGH quando a chave estiver aberta e LOW quando a chave for pressionada.

-----   Nota sobre o pino Slave Select (SS) em placas baseadas em AVR
Todas as placas baseadas em AVR têm um pino SS que é útil quando atuam como um escravo controlado por um mestre externo. 
Uma vez que esta biblioteca suporta apenas o modo mestre, este pino deve ser sempre definido como OUTPUT, caso contrário, 
a interface SPI poderia ser colocada automaticamente no modo escravo pelo hardware, tornando a biblioteca inoperante.

No entanto, é possível usar qualquer pino como Slave Select (SS) para os dispositivos. 
Por exemplo, o escudo Arduino Ethernet usa o pino 4 para controlar a conexão SPI com o cartão SD integrado 
e o pino 10 para controlar a conexão com o controlador Ethernet.

*/

#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM
#include <SPI.h>        // RC522 Module uses SPI protocol
#include <MFRC522.h>  // Library for Mifare RC522 Devices

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);
//LiquidCrystal_I2C lcd(0x27,16,2);


/*
  Em vez de um relé, você pode querer usar um servo. Servos também podem trancar e destrancar fechaduras
   O relé será usado por padrão
*/

// #include <Servo.h>

/*
  Para visualizar o que está acontecendo no hardware, precisamos de alguns leds e para controlar a trava da porta, um relé e um botão de limpeza (APAGA EEPROM)
   (ou algum outro hardware) led ânodo comum usado(+), digitalWriting HIGH desliga led led Lembre-se de que se você estiver indo
   para usar LEDs catódicos comuns (-) ou apenas leds separados, basta comentar #define COMMON_ANODE,

--- COMO PROGRAMAR OS CARTOES, PRIMEIRA PROGRAMACAO DE :
INICIE O SKETCH, OBSERVE QUE O LED AZUL ESTA ACESSO, INDICANDO PRONTO ( AGUARDANDO).
APROXIME UM  CARTAO QUE FOI ESCOLHIDO COMO MASTER DO LEITOR.
AGUARDE A SEQUENCIA DE LEDS ( RGB COMECAR)
OBSERVA A MENSAGEM DE BOAS VINDAS DO CARTAO MASTER
APROXIME UM CARTAO DO LEITOR, PARA ADICIONAR COMO MASTER



EXEMPLO ADICIONAR:
OBSERVE QUE O LED AZUL ESTA ACESO, INDICANDO PRONTO ( AGUARDANDO).
APROXIME O CARTAO  MASTER DO LEITOR,E RETIRE, AGUARDE A SEQUENCIA DOS LEDS RGB COMECAR A PISCAR
QUANDO OS LEDS RGB COMECAREM A PISCAR APROXIME O CARTAO QUE IRA ADICIONAR
----SE QUIZER INTERROMPER A PROGRAMACAO, APROXIME E RETIRE RAPIDAMENTE O CARTAO MASTER DO LEITOR E OBERSERVE O LED AZUL ASCENDER.----
SE BEM SUCEDIDO O LED VERDE IRA ASCENDER, INDICADO CARTAO ADICIONADO 
AGUARDE A SEQUENCIA DOS LEDS RGB COMECAR NOVAMENTE E APROXIME E RETIRE, OUTRA VEZ O CARTAO MASTER PARA FINALIZAR
OBSERVE QUE O LED AZUL ESTA ACESO, INDICANDO PRONTO ( AGUARDANDO).
CONFIRME SE CARTAO FOI CADASTRADO 
APROXIME O CARTAO DO LEITOR, E OBSERVE O LED VERDE ASCENDER,  TOM DA BUZINA, E FECHADURA ABRIR



EXEMPLO REMOVER:
OBSERVE QUE O LED AZUL ESTA ACESSO, INDICANDO PRONTO ( AGUARDANDO).
APROXIME E RETIRE O CARTAO MASTER DO LEITOR, AGUARDE A SEQUENCIA DOS LEDS RGB A PISCAR
QUANDO OS LEDS RGB COMECAREM A PISCAR APROXIME E RETIRE  O CARTAO QUE IRA REMOVER
QUANDO OS LEDS RGB COMECAREM A PISCAR APROXIME E RETIRE  O CARTAO MASTER
OBSERVE QUE O LED AZUL ESTA ACESO, INDICANDO PRONTO ( AGUARDANDO).
*/

//---COMENTAR PARA USAR LED CATODO COMUM(-)
#define COMMON_ANODE

#ifdef COMMON_ANODE
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif

//-------IMPORTANTE   USANDO PLACA SHIELD SOLDADA------

//---LED RGB ANODO COMUM (+)
#define redLed   6    // D6--LED VERMELHO
#define greenLed 7    // D7--LED VERDE
#define blueLed  8    // D8--LED AZUL

#define relay 12     // D12-- Relay usado para acionar a bobina da fechadura do portao social--
#define wipeB 3     // D3--Button pin for WipeMode // Ativar o resistor pull up do pino
#define BUZZER 22    // D22--buzzer pin (+)//----------GND(-) (MODELO SL-2305)
#define BUTTON_EXTERNO  27 // D-27 botao externo (CONTATO NA),(usando pullup interno ativado, COM RESISTOR 10K EXTERNO). botao ao lado do portao social( button led rgb)


bool programMode = false;  // inicialize o modo de programação para false

uint8_t successRead;    // Número inteiro variável a ser mantido se tivermos êxito na leitura do Reader

byte storedCard[4];   // Armazena um ID lido da EEPROM
byte readCard[4];   // Armazena a leitura de ID digitalizada do módulo RFID
byte masterCard[4];   // Armazena o ID do cartão principal lido na EEPROM


//----- Create MFRC522 instance para Arduino Mega 2560.
#define SS_PIN 53   ///-----SDA(SS) 
#define RST_PIN 5   ///-----RST
MFRC522 mfrc522(SS_PIN, RST_PIN);


///////////////////////////////////////// Setup ///////////////////////////////////
void setup() {
  //Arduino Pin Configuration
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(wipeB, INPUT_PULLUP);   // Ativar o resistor pull up do pino
  pinMode(BUZZER, OUTPUT);
  noTone(BUZZER);
  pinMode(relay, OUTPUT);
   //configure pin 2 as an input and enable the internal pull-up resistor
  pinMode(BUTTON_EXTERNO, INPUT_PULLUP);// USANDO RESISTOR DE PULLUP INTERNO ATIVADO
 
  //---------------------------------------------------------------------------------------------------------------------//
  //--Tenha cuidado com o comportamento do circuito do relé durante a redefinição ou o ciclo de energia do seu Arduino--//
  //--FOI ALTERADO NA LINHA 373 A ACAO DO RELAY PARA NIVEL LOGICO BAIXO---LOW-------------------------------------------//
  //--------------------------------------------------------------------------------------------------------------------//
  digitalWrite(relay, HIGH);    //HIGH-- Verifique se a porta está trancada
  digitalWrite(redLed, LED_OFF);  // Verifique se o led está desligado
  digitalWrite(greenLed, LED_OFF);  //Verifique se o led está desligado
  digitalWrite(blueLed, LED_OFF); // Verifique se o led está desligado

 //-----Define o número de colunas e linhas do LCD:  
  lcd.begin(); 
  lcd.backlight();  
  lcd.clear();
   mensageminicial(); 
  
 
  //----Protocol Configuration
  Serial.begin(9600);  // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware

  //----Se você definir Antena Gain para Max, aumentará a distância de leitura
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  Serial.println(F("Access Control Example v0.1"));   // For debugging purposes
  ShowReaderDetails();  // Show details of PCD - MFRC522 Card Reader details

  //---Código de limpeza - Se o botão (wipeB) pressionado durante a instalação (ligada), limpa EEPROM
  if (digitalRead(wipeB) == LOW) 
  {  // quando o pino pressionado deve ficar baixo, o botão conectado ao terra
    digitalWrite(redLed, LED_ON); // Led vermelho permanece ligado para informar ao usuário que vamos limpar
    Serial.println(F("Botão Limpar EEPROM pressionado"));
    Serial.println(F("Você tem 10 segundos para cancelar"));
    Serial.println(F("Isso removerá todos os registros e não poderá ser desfeito"));
    
    bool buttonState = monitorWipeButton(10000); // Dê ao usuário tempo suficiente para cancelar a operação
   
    if (buttonState == true && digitalRead(wipeB) == LOW) 
    {    // Se o botão ainda estiver pressionado, limpe EEPROM
      Serial.println(F("Iniciando a limpeza da EEPROM"));
      for (uint16_t x = 0; x < EEPROM.length(); x = x + 1) 
      {    //Fim do loop do endereço EEPROM
        if (EEPROM.read(x) == 0)
        {              //Se o endereço 0 da EEPROM
          // não faça nada, já claro, vá para o próximo endereço para economizar tempo e reduzir as gravações na EEPROM
        }
        else
        {
          EEPROM.write(x, 0);       // se não escrever 0 para limpar, são necessários 3,3mS
        }
      }
      Serial.println(F("EEPROM Successfully Limpo"));
      digitalWrite(redLed, LED_OFF);  // visualize a successful limpar
      delay(200);
      digitalWrite(redLed, LED_ON);
      delay(200);
      digitalWrite(redLed, LED_OFF);
      delay(200);
      digitalWrite(redLed, LED_ON);
      delay(200);
      digitalWrite(redLed, LED_OFF);
    }
    else 
    {
      Serial.println(F("Limpeza cancelada")); // Mostre algum feedback de que o botão de limpeza não foi pressionado por 15 segundos
      digitalWrite(redLed, LED_OFF);
    }
  }
  //-- Verifique se o cartão principal está definido; caso contrário, deixe o usuário escolher um cartão principal
  //-- Isso também é útil para redefinir apenas o Master Card
  //-- Você pode manter outros registros EEPROM, basta escrever outros que não sejam 143 no endereço EEPROM 1
  //-- O endereço 1 da EEPROM deve conter um número mágico que é '143'
  if (EEPROM.read(1) != 143) 
  {
    Serial.println(F("Nenhum cartão mestre definido"));
    Serial.println(F("Scanear um PICC para definir como cartão mestre"));
    do 
    {
      successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0
      digitalWrite(blueLed, LED_ON);    // Visualize Master Card need to be defined
      delay(200);
      digitalWrite(blueLed, LED_OFF);
      delay(200);
    }
    while (!successRead);                  // O programa não irá além enquanto você não obtiver uma leitura bem-sucedida
    for ( uint8_t j = 0; j < 4; j++ ) 
    {        // Loop 4 vezes
      EEPROM.write( 2 + j, readCard[j] );  // Escreva o UID do PICC digitalizado na EEPROM, inicie do endereço 3
    }
    EEPROM.write(1, 143);                  // Escreva para EEPROM que definimos como Master Card.
    Serial.println(F("Cartão Mestre Definido"));
  }
  Serial.println(F("-------------------"));
  Serial.println(F("UID do cartão principal"));
  for ( uint8_t i = 0; i < 4; i++ ) 
  {          // Leia o UID do cartão principal da EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Escreva no masterCard
    Serial.print(masterCard[i], HEX);
  }
  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Tudo está pronto"));
  Serial.println(F("Aguardando verificação de PICCs"));
  cycleLeds();    // Tudo pronto permite que o usuário dê algum feedback pedalando leds
}
//----------mensagem inicial no lcd---------------------------------------------------------------------------------

void mensageminicial()
{
  lcd.clear();
  lcd.print(" Aproxime o seu");  
  lcd.setCursor(0,1);
  lcd.print("cartao do leitor");
  lcd.setCursor(0,2);
  lcd.print("V1.0_MEGA");   
  lcd.setCursor(0,3);
  lcd.print("Botao-Social");  
}

//-----PARA ADICIONAR OU REMOVER TAG DA EEPROM---------------------------------------
void mensagemprograma()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" Aproxime o Cartao");  
  lcd.setCursor(0,1);
  lcd.print(" Master do leitor");  
  lcd.setCursor(0,2);
  lcd.print("Para Adicionar TAG...");
  lcd.setCursor(0,3);
  lcd.print("Para Remover TAG...");
}

//---PARA REDEFINIR O MASTER CARTAO NOVO-------------------------------------------------------
void mensagemMASTER()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" BOTAO LIMPA EEPROM");  
  lcd.setCursor(0,1);
  lcd.print(" FOI PRESSIONADO");  
  lcd.setCursor(0,2);
  lcd.print(" MASTER SERA...");
  lcd.setCursor(0,3);
  lcd.print("APAGADO EM 10 SEG.");
}


///////////////////////////////////////// Main Loop ///////////////////////////////////

void loop () {
   
  do 
  {
//---------botao externo  para abrir portao social    
   int sensorVal = digitalRead(BUTTON_EXTERNO); // 
   //////Serial.println(sensorVal); 
   if (sensorVal == HIGH) {
   digitalWrite(relay, HIGH); // USANDO RELAY NIVEL LOGICO low
   noTone(BUZZER); // DESLIGA BUZZER
 
  }else {
   digitalWrite(relay, LOW);// USANDO RELAY NIVEL LOGICO LOW
  
    tone(BUZZER, 900); // ACIONA BUZZER
    granted(500);         // 300--Abra a fechadura da porta por 300 ms
  }
  
  //---------------------------------------------------------------------------------------------------------------------
    successRead = getID();  // define successRead para 1 quando lemos do leitor, caso contrário 0
    
    //--- Quando o dispositivo estiver em uso, se o botão de limpeza for pressionado por 10 segundos, inicialize a limpeza do cartão principal
    if (digitalRead(wipeB) == LOW)
    
    { // Verifique se o botão está pressionado
      //--- Visualize a operação normal é interrompida pressionando o botão wipe Vermelho é como mais Aviso ao usuário
      digitalWrite(redLed, LED_ON);       // // Verifique se o LED vermelho está aceso
      digitalWrite(greenLed, LED_OFF);   //// Verifique se o LED verde está apagado
      digitalWrite(blueLed, LED_OFF);   // // Verifique se o LED azul está apagado
      //-- Dê algum feedback
      Serial.println(F("Botão Limpar EEPROM pressionado"));
      Serial.println(F("O cartão mestre será apagado! em 10 segundos"));
       mensagemMASTER();
      bool buttonState = monitorWipeButton(10000); // Dê ao usuário tempo suficiente para cancelar a operação
      
      if (buttonState == true && digitalRead(wipeB) == LOW) 
      {    // Se o botão ainda estiver pressionado, limpe EEPROM
        EEPROM.write(1, 0);                  // Redefina o número mágico.
 //---------------------------------------------------------------------------------------------------------------------
 //------------------------------------------------------------------------------------------------------------------------
 //-------------------------------------------------------------------------------------------------------------------------
   
      //-----mensagem lcd---------------------------------------------------------------
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(" Master Cartao !");
      lcd.setCursor(0,1);
      lcd.print("APAGADO DA EEPROM !!!");
      lcd.setCursor(0,2);
      lcd.print(" MASTER CANCELADO!!!");
       lcd.setCursor(0,3);
      lcd.print(" RESET A PLACA!!!");
     
      //---------------------------------------------------------------------------------       
        Serial.println(F("Master Card apagado do dispositivo"));
        Serial.println(F("Redefina para reprogramar o Master Card"));
        while (1);
      }
      Serial.println(F(" cartão mestre cancelado"));
       
     //-----------------------------------------------------------------------------------------------------------------------
     //--------------------------------------------------------------------------------------------------------------------------
      
    }
    if (programMode) 
    {
      cycleLeds();              // O modo de programação pisca os leds vermelho, verde, azul aguardando a leitura de um novo cartão
    }
    else 
    {
      normalModeOn();     // Modo normal, o LED azul de energia está aceso, todos os outros estão apagados
   
    }
  }
  while (!successRead);   //o programa não irá além enquanto você não estiver obtendo uma leitura bem-sucedida
  if (programMode) 
  {
    if ( isMaster(readCard) ) 
    {     //-----Quando estiver no modo de programaçao, verifique Primeiro Se o cartão principal foi digitalizado novamente para sair do modo de programaçao
      Serial.println(F("Master Card digitalizado"));
      Serial.println(F("Saindo do modo de programacao"));
      Serial.println(F("-----------------------------"));
      programMode = false;
      return;
    }
    else 
    {
      if ( findID(readCard) ) { // If scanned card is known delete it
        Serial.println(F("Eu conheço esse PICC, removendo..."));
        deleteID(readCard);
        Serial.println("-----------------------------");
        Serial.println(F("Digitalize um PICC para ADICIONAR ou REMOVER para EEPROM"));
      }
      else 
      {                    // If scanned card is not known add it
        Serial.println(F("Eu não conheço este PICC, adicionando..."));
        writeID(readCard);
        Serial.println(F("-----------------------------"));
        Serial.println(F("Digitalize um PICC para ADICIONAR ou REMOVER para EEPROM"));
      }
    }
  }
  else
  {  //-------AQUI FAZ A PROGRAMAÇAO DOS CARTOES NA EEPROM---------------------------------------------------------
    if ( isMaster(readCard)) 
    {    //-- Se o ID do cartão digitalizado corresponder ao ID do Master Card - entre no modo de programacao-----------------------
      programMode = true;
      Serial.println(F("Hello Master Card - Entrando no Modo de Programacao"));
       lcd.clear();
      //-----mensagem lcd---------------------------------------------------------------
      lcd.setCursor(0,0);
      lcd.print("Ola Cartao Master !");
      lcd.setCursor(0,1);
      lcd.print("Entrando no Modo !");
      lcd.setCursor(0,2);
      lcd.print(" Programacao!");
     
      //---------------------------------------------------------------------------------
      uint8_t count = EEPROM.read(0);   // Leia o primeiro byte da EEPROM que
      Serial.print(F("EU TENHO "));     // armazena o número de IDs na EEPROM
      Serial.print(count);
       //-----mensagem lcd---------------------------------------------------------------
     
      lcd.setCursor(0,3);
      lcd.print("Registros.EEPROM.");
      lcd.setCursor(18,3);
      lcd.print(count);
      delay(3000);
      Serial.print(F(" registros na EEPROM"));
      Serial.println("");
      Serial.println(F("Digitalize um PICC para ADICIONAR ou REMOVER DA EEPROM"));
      Serial.println(F("Digitalize o Master Card novamente para sair do modo de programacao"));
      Serial.println(F("-----------------------------"));
      //---------mensagem inicial lcd--------------------------------------
      lcd.clear();
      mensagemprograma();
 
    }
    else 
    {
      if ( findID(readCard) ) { // Caso contrário, verifique se o cartão está na EEPROM
        Serial.println(F("Bem-vindo, voce pode passar"));

        //------DELAY DO RELAY PARA ABRIR A FECHADURA------------------//--------------------------------//
       
        granted(500);         // 300--Abra a fechadura da porta por 300 ms

        //--------------------------------------------------------------//-------------------------------//
      }
      else
      {      // Caso contrário, mostre que o ID não era válido
        Serial.println(F("Voce nao pode passar"));
        denied();
      }
    }
  }
}

///////////////////////////////////////// Acesso concedido   ///////////////////////////////////
void granted ( uint16_t setDelay) 
{
  digitalWrite(blueLed, LED_OFF);    // Desligue o LED azul
  digitalWrite(redLed, LED_OFF);     // Desligue o LED vermelho
  digitalWrite(greenLed, LED_ON);   // Ligue o LED verde
 //----mensagem lcd---------------------------------------------------
  lcd.setCursor(0,1);
  lcd.print("Acesso liberado!");
  lcd.setCursor(0,2);
  lcd.print("Voce Pode Entrar!");
  tone(BUZZER, 900); // ACIONA BUZZER
 //-----------------------------------------------------------------
  digitalWrite(relay, LOW);      // LOW-- Destranque a porta!
  delay(setDelay);              // Mantenha a trava da porta aberta por alguns segundos
  digitalWrite(relay, HIGH);    //HIGH--- Porta de bloqueio
  delay(1000);                   // Mantenha o LED verde aceso por um segundo
  noTone(BUZZER); // DESLIGA BUZZER
 //-------------mensagem inicial lcd-----------------------------------------------
  lcd.clear();
   mensageminicial();
}

///////////////////////////////////////// Acesso negado ///////////////////////////////////
void denied() 
{
  digitalWrite(greenLed, LED_OFF);  // // Verifique se o LED vermelho está apagado
  digitalWrite(blueLed, LED_OFF);   // // Verifique se o LED azul está apagado
  digitalWrite(redLed, LED_ON);   // // Verifique se o LED vermelho está aceso
  //----------mensagem lcd-----------------------------------------------------
  lcd.setCursor(0,1);
  lcd.print("Acesso Negado !!!!");
  lcd.setCursor(0,2);
  lcd.print("Tag Nao Cadastrada.");
  tone(BUZZER, 300);
  delay(1000);
  noTone(BUZZER);
  //-------------mensagem inicial lcd-----------------------------------------------
   lcd.clear();
   mensageminicial();
}


///////////////////////////////////////// Obtenha o UID do PICC ///////////////////////////////////
uint8_t getID() {
  // Preparando-se para ler PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  { //Se um novo PICC colocado no leitor RFID continuar
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {   //Com um PICC colocado, obtém o Serial e continua
    return 0;
  }
  //------------- Existem PICCs da Mifare com  UID de 4 ou 7 bytes se você usar PICC de 7 bytes
  // Eu acho que devemos assumir todos os PICCs, pois eles têm UID de 4 bytes
  // Até suportarmos PICCs de 7 bytes
  Serial.println(F("UID do PICC digitalizado"));
  for ( uint8_t i = 0; i < 4; i++)
  {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

void ShowReaderDetails()
{
  //--- Obtenha a versão do software MFRC522
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (desconhecido), provavelmente um clone chinês?"));
  Serial.println("");
  //--- Quando 0x00 ou 0xFF é retornado, a comunicação provavelmente falhou
  if ((v == 0x00) || (v == 0xFF)) 
  {
    Serial.println(F("AVISO: Falha na comunicação. O MFRC522 está conectado corretamente?"));
    Serial.println(F("SISTEMA ALTERADO: Verifique as conexões."));
    //--- O sistema de visualização está parado
    digitalWrite(greenLed, LED_OFF);  // // Verifique se o LED verde está apagado
    digitalWrite(blueLed, LED_OFF);   // // Verifique se o LED azul está apagado
    digitalWrite(redLed, LED_ON);   // // Verifique se o LED vermelho está aceso
    while (true); // do not go further
  }
}

///////////////////////////////////////// Ciclos de Leds (Modo Programacao) ///////////////////////////////////
void cycleLeds()
{
  digitalWrite(redLed, LED_OFF);  // Verifique se o LED vermelho está apagado
  digitalWrite(greenLed, LED_ON);   // Verifique se o LED verde está aceso
  digitalWrite(blueLed, LED_OFF);   // Verifique se o LED azul está apagado
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Verifique se o LED vermelho está apagado
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED verde está apagado
  digitalWrite(blueLed, LED_ON);  // Verifique se o LED azul está aceso
  delay(200);
  digitalWrite(redLed, LED_ON);   // Verifique se o LED vermelho está aceso
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED verde está apagado
  digitalWrite(blueLed, LED_OFF);   // Verifique se o LED azul está apagado
  delay(200);
}

//////////////////////////////////////// Led de modo normal  ///////////////////////////////////
void normalModeOn ()
{
  digitalWrite(blueLed, LED_ON);  // LED azul aceso e pronto para ler o cartão
  digitalWrite(redLed, LED_OFF);  // Verifique se o LED vermelho está apagado
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED verde está apagado
  
  //------------------------------------------------------------------------------------------------------------------
  
  //-------AQUI ALTERAR ACAO DO RELAY DE ACORDO COM O RELAY ADOTADO---NIVEL LOGICO ALTO OU BAIXO NA ENERGIZACAO--------
  
  digitalWrite(relay, HIGH);    // HIGH----Verifique se a porta está travada

  
  //---------------------------------------------------------------------------------------------------------------------
}

//////////////////////////////////////// Leia um ID da EEPROM //////////////////////////////
void readID( uint8_t number )
{
  uint8_t start = (number * 4 ) + 2;    // Descobrir a posição inicial
  for ( uint8_t i = 0; i < 4; i++ ) {     // Faça um loop 4 vezes para obter os 4 bytes
    storedCard[i] = EEPROM.read(start + i);   // Atribuir valores lidos da EEPROM à matriz( array)
  }
}

///////////////////////////////////////// Adicionar ID à EEPROM   ///////////////////////////////////
void writeID( byte a[] ) 
{
  if ( !findID( a ) ) 
  {     // Antes de escrever para a EEPROM, verifique se já vimos esse cartão antes!
    uint8_t num = EEPROM.read(0);     // Obtenha o número de espaços usados, a posição 0 armazena o número de cartões de identificação
    uint8_t start = ( num * 4 ) + 6;  // Descobrir onde o próximo slot começa
    num++;                // Incremente o contador em um
    EEPROM.write( 0, num );     // Escreva a nova contagem no  counter
    for ( uint8_t j = 0; j < 4; j++ ) {   // Loop 4 vezes
      EEPROM.write( start + j, a[j] );  // Escreva os valores da ARRAY na EEPROM na posição correta
    }
    successWrite();
    Serial.println(F("Registro de ID adicionado com êxito à EEPROM"));

//------------------------------MESAGEM LCD------------------------------------------------------

 //----------mensagem lcd-----------------------------------------------------
  lcd.setCursor(0,0);
  lcd.print("ID adicionado ....");
  lcd.setCursor(0,1);
  lcd.print("...Com Exito.....");
  lcd.setCursor(0,2);
  lcd.print("...Na EEPROM.....");
  delay(3000);
  //-------------mensagem inicial lcd-----------------------------------------------
   lcd.clear();
   mensageminicial();


//-------------------------------------------------------------------------------------------------

    
  }
  else 
  {
    failedWrite();
    Serial.println(F("Falhou! Há algo errado com ID ou EEPROM ruim"));
    //----------mensagem lcd-----------------------------------------------------
  lcd.setCursor(0,0);
  lcd.print("Falha! Algo errado ....");
  lcd.setCursor(0,1);
  lcd.print("...Verifique ID.....");
  lcd.setCursor(0,2);
  lcd.print("..Ou EEPROM..Ruim...");
  delay(3000);
  //-------------mensagem inicial lcd-----------------------------------------------
   lcd.clear();
   mensageminicial();
  }
}

///////////////////////////////////////// Remover ID da EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) 
{
  if ( !findID( a ) )
  {     // Antes de excluirmos da EEPROM, verifique se temos este cartão!
    failedWrite();      // Se não
    Serial.println(F("Falhou! Há algo errado com ID ou EEPROM ruim"));
  }
  else 
  {
    uint8_t num = EEPROM.read(0);   // Obtenha o número de espaços usados, a posição 0 armazena o número de cartões de identificação
    uint8_t slot;       // Descobrir o número do slot do cartão
    uint8_t start;      // = ( num * 4 ) + 6; // Descobrir onde o próximo slot começa
    uint8_t looping;    // O número de vezes que o loop se repete
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Leia o primeiro byte da EEPROM que armazena o número de cartões
    slot = findIDSLOT( a );   // Descobrir o número do slot do cartão a ser excluído
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrementar o contador em um
    EEPROM.write( 0, num );   // Escreva a nova contagem no  counter
    for ( j = 0; j < looping; j++ )
    {         // Repetir os tempos de troca do cartão
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Mude os valores da matriz (ARRAY) para 4 lugares anteriormente na EEPROM
    }
    for ( uint8_t k = 0; k < 4; k++ ) 
    {         // Loop de deslocamento
      EEPROM.write( start + j + k, 0);
    }
    successDelete();
    Serial.println(F("Registro de identificação removido com êxito da EEPROM"));
    //------------------------------MESAGEM LCD------------------------------------------------------

 //----------mensagem lcd-----------------------------------------------------
  lcd.setCursor(0,0);
  lcd.print("ID removido ....");
  lcd.setCursor(0,1);
  lcd.print("...Com Exito.....");
  lcd.setCursor(0,2);
  lcd.print("...Na EEPROM.....");
  delay(3000);
  //-------------mensagem inicial lcd-----------------------------------------------
   lcd.clear();
   mensageminicial();


//-------------------------------------------------------------------------------------------------

  }
}

///////////////////////////////////////// Verificar bytes   ///////////////////////////////////
bool checkTwo ( byte a[], byte b[] ) 
{   
  for ( uint8_t k = 0; k < 4; k++ ) 
  {   // Loop 4 vezes
    if ( a[k] != b[k] ) 
    {     // SE a! = B então false, porque: um falha, todos falham
       return false;
    }
  }
  return true;  
}

///////////////////////////////////////// Localizar slot  ///////////////////////////////////
uint8_t findIDSLOT( byte find[] ) 
{
  uint8_t count = EEPROM.read(0);       // Leia o primeiro byte da EEPROM que
  for ( uint8_t i = 1; i <= count; i++ ) 
  {    // Loop uma vez para cada entrada EEPROM
    readID(i);                // Leia um ID da EEPROM, ele é armazenado no cartão armazenado [4]
    if ( checkTwo( find, storedCard ) ) 
    {   // Verifique se o cartão armazenado lê da EEPROM
      // é o mesmo que o achado []Cartão de identificação passado
      return i;         // O número do slot do cartão
    }
  }
}

///////////////////////////////////////// Localizar ID da EEPROM  ///////////////////////////////////
bool findID( byte find[] ) 
{
  uint8_t count = EEPROM.read(0);     // Leia o primeiro byte da EEPROM que
  for ( uint8_t i = 1; i < count; i++ ) 
  {    // Loop uma vez para cada entrada EEPROM
    readID(i);          // Leia um ID da EEPROM, ele é armazenado no cartão armazenado [4]
    if ( checkTwo( find, storedCard ) )
    {   // Verifique se o cartão armazenado lê da EEPROM
      return true;
    }
    else 
    {    // Caso contrário, retorne false
    }
  }
  return false;
}

///////////////////////////////////////// Gravar Sucesso na EEPROM   ///////////////////////////////////
//--------- Pisca o LED verde 3 vezes para indicar uma gravação bem-sucedida na EEPROM
void successWrite() 
{
  digitalWrite(blueLed, LED_OFF);   // Verifique se o LED azul está apagado
  digitalWrite(redLed, LED_OFF);   // Verifique se o LED VERMELHO está apagado
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED VERDE está apagado
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Verifique se o LED VERDE está ACESO
  delay(200);
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED VERDE está apagado
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Verifique se o LED VERDE está ACESO
  delay(200);
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED VERDE está apagado
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Verifique se o LED VERDE está ACESO
  delay(200);
}

///////////////////////////////////////// Falha na gravação na EEPROM   ///////////////////////////////////
// Pisca o LED vermelho 3 vezes para indicar uma falha na gravação na EEPROM
void failedWrite()
{
  digitalWrite(blueLed, LED_OFF);   // Verifique se o LED azul está apagado
  digitalWrite(redLed, LED_OFF);  // Verifique se o LED VERMELHO está apagado
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED VERDE está apagado
  delay(200);
  digitalWrite(redLed, LED_ON);   // Verifique se o LED vermelho está ACESO
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Verifique se o LED vermelho está apagado
  delay(200);
  digitalWrite(redLed, LED_ON);   // Verifique se o LED vermelho está aceso
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Verifique se o LED vermelho está apagado
  delay(200);
  digitalWrite(redLed, LED_ON);   // Verifique se o LED vermelho está aceso
  delay(200);
}

///////////////////////////////////////// Sucesso remover UID da EEPROM  ///////////////////////////////////
// Pisca o LED azul 3 vezes para indicar uma exclusão bem-sucedida na EEPROM
void successDelete()
{
  digitalWrite(blueLed, LED_OFF);   // Verifique se o LED azul está apagado
  digitalWrite(redLed, LED_OFF);    // Verifique se o LED vermelho está apagado
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED verde está apagado
  delay(200);
  digitalWrite(blueLed, LED_ON);  // Verifique se o LED azul está aceso
  delay(200);
  digitalWrite(blueLed, LED_OFF);   // Verifique se o LED azul está apagado
  delay(200);
  digitalWrite(blueLed, LED_ON);  // Verifique se o LED azul está aceso
  delay(200);
  digitalWrite(blueLed, LED_OFF);   // Verifique se o LED azul está apagado
  delay(200);
  digitalWrite(blueLed, LED_ON);  // Verifique se o LED azul está aceso
  delay(200);
}

////////////////////// Verifique o cartão lido----SE é masterCard   ///////////////////////////////////

//----------- Verifique se o ID passado é o cartão de programação principal
bool isMaster( byte test[] )
{
	return checkTwo(test, masterCard);
}

bool monitorWipeButton(uint32_t interval) 
{
  uint32_t now = (uint32_t)millis();
  while ((uint32_t)millis() - now < interval) 
  {
    // check on every half a second
    if (((uint32_t)millis() % 500) == 0)
    {
      if (digitalRead(wipeB) != LOW)
        return false;
    }
  }
  return true;
}
