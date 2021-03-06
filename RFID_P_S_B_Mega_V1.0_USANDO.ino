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
   RST/Reset   RST          5       |||||     (marcacao do fio com tra??o vermelho)   
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




-------------DEFINI??AO DE PULLUP:
   Ao conectar um sensor a um pino configurado com INPUT_PULLUP, a outra extremidade deve ser conectada ao aterramento. 
   No caso de uma chave simples, isso faz com que a leitura do pino seja HIGH quando a chave estiver aberta e LOW quando a chave for pressionada.

-----   Nota sobre o pino Slave Select (SS) em placas baseadas em AVR
Todas as placas baseadas em AVR t??m um pino SS que ?? ??til quando atuam como um escravo controlado por um mestre externo. 
Uma vez que esta biblioteca suporta apenas o modo mestre, este pino deve ser sempre definido como OUTPUT, caso contr??rio, 
a interface SPI poderia ser colocada automaticamente no modo escravo pelo hardware, tornando a biblioteca inoperante.

No entanto, ?? poss??vel usar qualquer pino como Slave Select (SS) para os dispositivos. 
Por exemplo, o escudo Arduino Ethernet usa o pino 4 para controlar a conex??o SPI com o cart??o SD integrado 
e o pino 10 para controlar a conex??o com o controlador Ethernet.

*/

#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM
#include <SPI.h>        // RC522 Module uses SPI protocol
#include <MFRC522.h>  // Library for Mifare RC522 Devices

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);
//LiquidCrystal_I2C lcd(0x27,16,2);


/*
  Em vez de um rel??, voc?? pode querer usar um servo. Servos tamb??m podem trancar e destrancar fechaduras
???? O rel?? ser?? usado por padr??o
*/

// #include <Servo.h>

/*
  Para visualizar o que est?? acontecendo no hardware, precisamos de alguns leds e para controlar a trava da porta, um rel?? e um bot??o de limpeza (APAGA EEPROM)
???? (ou algum outro hardware) led ??nodo comum usado(+), digitalWriting HIGH desliga led led Lembre-se de que se voc?? estiver indo
???? para usar LEDs cat??dicos comuns (-) ou apenas leds separados, basta comentar #define COMMON_ANODE,

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


bool programMode = false;  // inicialize o modo de programa????o para false

uint8_t successRead;    // N??mero inteiro vari??vel a ser mantido se tivermos ??xito na leitura do Reader

byte storedCard[4];   // Armazena um ID lido da EEPROM
byte readCard[4];   // Armazena a leitura de ID digitalizada do m??dulo RFID
byte masterCard[4];   // Armazena o ID do cart??o principal lido na EEPROM


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
  //--Tenha cuidado com o comportamento do circuito do rel?? durante a redefini????o ou o ciclo de energia do seu Arduino--//
  //--FOI ALTERADO NA LINHA 373 A ACAO DO RELAY PARA NIVEL LOGICO BAIXO---LOW-------------------------------------------//
  //--------------------------------------------------------------------------------------------------------------------//
  digitalWrite(relay, HIGH);    //HIGH-- Verifique se a porta est?? trancada
  digitalWrite(redLed, LED_OFF);  // Verifique se o led est?? desligado
  digitalWrite(greenLed, LED_OFF);  //Verifique se o led est?? desligado
  digitalWrite(blueLed, LED_OFF); // Verifique se o led est?? desligado

 //-----Define o n??mero de colunas e linhas do LCD:  
  lcd.begin(); 
  lcd.backlight();  
  lcd.clear();
   mensageminicial(); 
  
 
  //----Protocol Configuration
  Serial.begin(9600);  // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware

  //----Se voc?? definir Antena Gain para Max, aumentar?? a dist??ncia de leitura
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  Serial.println(F("Access Control Example v0.1"));   // For debugging purposes
  ShowReaderDetails();  // Show details of PCD - MFRC522 Card Reader details

  //---C??digo de limpeza - Se o bot??o (wipeB) pressionado durante a instala????o (ligada), limpa EEPROM
  if (digitalRead(wipeB) == LOW) 
  {  // quando o pino pressionado deve ficar baixo, o bot??o conectado ao terra
    digitalWrite(redLed, LED_ON); // Led vermelho permanece ligado para informar ao usu??rio que vamos limpar
    Serial.println(F("Bot??o Limpar EEPROM pressionado"));
    Serial.println(F("Voc?? tem 10 segundos para cancelar"));
    Serial.println(F("Isso remover?? todos os registros e n??o poder?? ser desfeito"));
    
    bool buttonState = monitorWipeButton(10000); // D?? ao usu??rio tempo suficiente para cancelar a opera????o
   
    if (buttonState == true && digitalRead(wipeB) == LOW) 
    {    // Se o bot??o ainda estiver pressionado, limpe EEPROM
      Serial.println(F("Iniciando a limpeza da EEPROM"));
      for (uint16_t x = 0; x < EEPROM.length(); x = x + 1) 
      {    //Fim do loop do endere??o EEPROM
        if (EEPROM.read(x) == 0)
        {              //Se o endere??o 0 da EEPROM
          // n??o fa??a nada, j?? claro, v?? para o pr??ximo endere??o para economizar tempo e reduzir as grava????es na EEPROM
        }
        else
        {
          EEPROM.write(x, 0);       // se n??o escrever 0 para limpar, s??o necess??rios 3,3mS
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
      Serial.println(F("Limpeza cancelada")); // Mostre algum feedback de que o bot??o de limpeza n??o foi pressionado por 15 segundos
      digitalWrite(redLed, LED_OFF);
    }
  }
  //-- Verifique se o cart??o principal est?? definido; caso contr??rio, deixe o usu??rio escolher um cart??o principal
  //-- Isso tamb??m ?? ??til para redefinir apenas o Master Card
  //-- Voc?? pode manter outros registros EEPROM, basta escrever outros que n??o sejam 143 no endere??o EEPROM 1
  //-- O endere??o 1 da EEPROM deve conter um n??mero m??gico que ?? '143'
  if (EEPROM.read(1) != 143) 
  {
    Serial.println(F("Nenhum cart??o mestre definido"));
    Serial.println(F("Scanear um PICC para definir como cart??o mestre"));
    do 
    {
      successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0
      digitalWrite(blueLed, LED_ON);    // Visualize Master Card need to be defined
      delay(200);
      digitalWrite(blueLed, LED_OFF);
      delay(200);
    }
    while (!successRead);                  // O programa n??o ir?? al??m enquanto voc?? n??o obtiver uma leitura bem-sucedida
    for ( uint8_t j = 0; j < 4; j++ ) 
    {        // Loop 4 vezes
      EEPROM.write( 2 + j, readCard[j] );  // Escreva o UID do PICC digitalizado na EEPROM, inicie do endere??o 3
    }
    EEPROM.write(1, 143);                  // Escreva para EEPROM que definimos como Master Card.
    Serial.println(F("Cart??o Mestre Definido"));
  }
  Serial.println(F("-------------------"));
  Serial.println(F("UID do cart??o principal"));
  for ( uint8_t i = 0; i < 4; i++ ) 
  {          // Leia o UID do cart??o principal da EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Escreva no masterCard
    Serial.print(masterCard[i], HEX);
  }
  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Tudo est?? pronto"));
  Serial.println(F("Aguardando verifica????o de PICCs"));
  cycleLeds();    // Tudo pronto permite que o usu??rio d?? algum feedback pedalando leds
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
    successRead = getID();  // define successRead para 1 quando lemos do leitor, caso contr??rio 0
    
    //--- Quando o dispositivo estiver em uso, se o bot??o de limpeza for pressionado por 10 segundos, inicialize a limpeza do cart??o principal
    if (digitalRead(wipeB) == LOW)
    
    { // Verifique se o bot??o est?? pressionado
      //--- Visualize a opera????o normal ?? interrompida pressionando o bot??o wipe Vermelho ?? como mais Aviso ao usu??rio
      digitalWrite(redLed, LED_ON);       // // Verifique se o LED vermelho est?? aceso
      digitalWrite(greenLed, LED_OFF);   //// Verifique se o LED verde est?? apagado
      digitalWrite(blueLed, LED_OFF);   // // Verifique se o LED azul est?? apagado
      //-- D?? algum feedback
      Serial.println(F("Bot??o Limpar EEPROM pressionado"));
      Serial.println(F("O cart??o mestre ser?? apagado! em 10 segundos"));
       mensagemMASTER();
      bool buttonState = monitorWipeButton(10000); // D?? ao usu??rio tempo suficiente para cancelar a opera????o
      
      if (buttonState == true && digitalRead(wipeB) == LOW) 
      {    // Se o bot??o ainda estiver pressionado, limpe EEPROM
        EEPROM.write(1, 0);                  // Redefina o n??mero m??gico.
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
      Serial.println(F(" cart??o mestre cancelado"));
       
     //-----------------------------------------------------------------------------------------------------------------------
     //--------------------------------------------------------------------------------------------------------------------------
      
    }
    if (programMode) 
    {
      cycleLeds();              // O modo de programa????o pisca os leds vermelho, verde, azul aguardando a leitura de um novo cart??o
    }
    else 
    {
      normalModeOn();     // Modo normal, o LED azul de energia est?? aceso, todos os outros est??o apagados
   
    }
  }
  while (!successRead);   //o programa n??o ir?? al??m enquanto voc?? n??o estiver obtendo uma leitura bem-sucedida
  if (programMode) 
  {
    if ( isMaster(readCard) ) 
    {     //-----Quando estiver no modo de programa??ao, verifique Primeiro Se o cart??o principal foi digitalizado novamente para sair do modo de programa??ao
      Serial.println(F("Master Card digitalizado"));
      Serial.println(F("Saindo do modo de programacao"));
      Serial.println(F("-----------------------------"));
      programMode = false;
      return;
    }
    else 
    {
      if ( findID(readCard) ) { // If scanned card is known delete it
        Serial.println(F("Eu conhe??o esse PICC, removendo..."));
        deleteID(readCard);
        Serial.println("-----------------------------");
        Serial.println(F("Digitalize um PICC para ADICIONAR ou REMOVER para EEPROM"));
      }
      else 
      {                    // If scanned card is not known add it
        Serial.println(F("Eu n??o conhe??o este PICC, adicionando..."));
        writeID(readCard);
        Serial.println(F("-----------------------------"));
        Serial.println(F("Digitalize um PICC para ADICIONAR ou REMOVER para EEPROM"));
      }
    }
  }
  else
  {  //-------AQUI FAZ A PROGRAMA??AO DOS CARTOES NA EEPROM---------------------------------------------------------
    if ( isMaster(readCard)) 
    {    //-- Se o ID do cart??o digitalizado corresponder ao ID do Master Card - entre no modo de programacao-----------------------
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
      Serial.print(F("EU TENHO "));     // armazena o n??mero de IDs na EEPROM
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
      if ( findID(readCard) ) { // Caso contr??rio, verifique se o cart??o est?? na EEPROM
        Serial.println(F("Bem-vindo, voce pode passar"));

        //------DELAY DO RELAY PARA ABRIR A FECHADURA------------------//--------------------------------//
       
        granted(500);         // 300--Abra a fechadura da porta por 300 ms

        //--------------------------------------------------------------//-------------------------------//
      }
      else
      {      // Caso contr??rio, mostre que o ID n??o era v??lido
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
  digitalWrite(greenLed, LED_OFF);  // // Verifique se o LED vermelho est?? apagado
  digitalWrite(blueLed, LED_OFF);   // // Verifique se o LED azul est?? apagado
  digitalWrite(redLed, LED_ON);   // // Verifique se o LED vermelho est?? aceso
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
  {   //Com um PICC colocado, obt??m o Serial e continua
    return 0;
  }
  //------------- Existem PICCs da Mifare com  UID de 4 ou 7 bytes se voc?? usar PICC de 7 bytes
  // Eu acho que devemos assumir todos os PICCs, pois eles t??m UID de 4 bytes
  // At?? suportarmos PICCs de 7 bytes
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
  //--- Obtenha a vers??o do software MFRC522
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (desconhecido), provavelmente um clone chin??s?"));
  Serial.println("");
  //--- Quando 0x00 ou 0xFF ?? retornado, a comunica????o provavelmente falhou
  if ((v == 0x00) || (v == 0xFF)) 
  {
    Serial.println(F("AVISO: Falha na comunica????o. O MFRC522 est?? conectado corretamente?"));
    Serial.println(F("SISTEMA ALTERADO: Verifique as conex??es."));
    //--- O sistema de visualiza????o est?? parado
    digitalWrite(greenLed, LED_OFF);  // // Verifique se o LED verde est?? apagado
    digitalWrite(blueLed, LED_OFF);   // // Verifique se o LED azul est?? apagado
    digitalWrite(redLed, LED_ON);   // // Verifique se o LED vermelho est?? aceso
    while (true); // do not go further
  }
}

///////////////////////////////////////// Ciclos de Leds (Modo Programacao) ///////////////////////////////////
void cycleLeds()
{
  digitalWrite(redLed, LED_OFF);  // Verifique se o LED vermelho est?? apagado
  digitalWrite(greenLed, LED_ON);   // Verifique se o LED verde est?? aceso
  digitalWrite(blueLed, LED_OFF);   // Verifique se o LED azul est?? apagado
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Verifique se o LED vermelho est?? apagado
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED verde est?? apagado
  digitalWrite(blueLed, LED_ON);  // Verifique se o LED azul est?? aceso
  delay(200);
  digitalWrite(redLed, LED_ON);   // Verifique se o LED vermelho est?? aceso
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED verde est?? apagado
  digitalWrite(blueLed, LED_OFF);   // Verifique se o LED azul est?? apagado
  delay(200);
}

//////////////////////////////////////// Led de modo normal  ///////////////////////////////////
void normalModeOn ()
{
  digitalWrite(blueLed, LED_ON);  // LED azul aceso e pronto para ler o cart??o
  digitalWrite(redLed, LED_OFF);  // Verifique se o LED vermelho est?? apagado
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED verde est?? apagado
  
  //------------------------------------------------------------------------------------------------------------------
  
  //-------AQUI ALTERAR ACAO DO RELAY DE ACORDO COM O RELAY ADOTADO---NIVEL LOGICO ALTO OU BAIXO NA ENERGIZACAO--------
  
  digitalWrite(relay, HIGH);    // HIGH----Verifique se a porta est?? travada

  
  //---------------------------------------------------------------------------------------------------------------------
}

//////////////////////////////////////// Leia um ID da EEPROM //////////////////////////////
void readID( uint8_t number )
{
  uint8_t start = (number * 4 ) + 2;    // Descobrir a posi????o inicial
  for ( uint8_t i = 0; i < 4; i++ ) {     // Fa??a um loop 4 vezes para obter os 4 bytes
    storedCard[i] = EEPROM.read(start + i);   // Atribuir valores lidos da EEPROM ?? matriz( array)
  }
}

///////////////////////////////////////// Adicionar ID ?? EEPROM   ///////////////////////////////////
void writeID( byte a[] ) 
{
  if ( !findID( a ) ) 
  {     // Antes de escrever para a EEPROM, verifique se j?? vimos esse cart??o antes!
    uint8_t num = EEPROM.read(0);     // Obtenha o n??mero de espa??os usados, a posi????o 0 armazena o n??mero de cart??es de identifica????o
    uint8_t start = ( num * 4 ) + 6;  // Descobrir onde o pr??ximo slot come??a
    num++;                // Incremente o contador em um
    EEPROM.write( 0, num );     // Escreva a nova contagem no  counter
    for ( uint8_t j = 0; j < 4; j++ ) {   // Loop 4 vezes
      EEPROM.write( start + j, a[j] );  // Escreva os valores da ARRAY na EEPROM na posi????o correta
    }
    successWrite();
    Serial.println(F("Registro de ID adicionado com ??xito ?? EEPROM"));

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
    Serial.println(F("Falhou! H?? algo errado com ID ou EEPROM ruim"));
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
  {     // Antes de excluirmos da EEPROM, verifique se temos este cart??o!
    failedWrite();      // Se n??o
    Serial.println(F("Falhou! H?? algo errado com ID ou EEPROM ruim"));
  }
  else 
  {
    uint8_t num = EEPROM.read(0);   // Obtenha o n??mero de espa??os usados, a posi????o 0 armazena o n??mero de cart??es de identifica????o
    uint8_t slot;       // Descobrir o n??mero do slot do cart??o
    uint8_t start;      // = ( num * 4 ) + 6; // Descobrir onde o pr??ximo slot come??a
    uint8_t looping;    // O n??mero de vezes que o loop se repete
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Leia o primeiro byte da EEPROM que armazena o n??mero de cart??es
    slot = findIDSLOT( a );   // Descobrir o n??mero do slot do cart??o a ser exclu??do
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrementar o contador em um
    EEPROM.write( 0, num );   // Escreva a nova contagem no  counter
    for ( j = 0; j < looping; j++ )
    {         // Repetir os tempos de troca do cart??o
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Mude os valores da matriz (ARRAY) para 4 lugares anteriormente na EEPROM
    }
    for ( uint8_t k = 0; k < 4; k++ ) 
    {         // Loop de deslocamento
      EEPROM.write( start + j + k, 0);
    }
    successDelete();
    Serial.println(F("Registro de identifica????o removido com ??xito da EEPROM"));
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
    {     // SE a! = B ent??o false, porque: um falha, todos falham
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
    readID(i);                // Leia um ID da EEPROM, ele ?? armazenado no cart??o armazenado [4]
    if ( checkTwo( find, storedCard ) ) 
    {   // Verifique se o cart??o armazenado l?? da EEPROM
      // ?? o mesmo que o achado []Cart??o de identifica????o passado
      return i;         // O n??mero do slot do cart??o
    }
  }
}

///////////////////////////////////////// Localizar ID da EEPROM  ///////////////////////////////////
bool findID( byte find[] ) 
{
  uint8_t count = EEPROM.read(0);     // Leia o primeiro byte da EEPROM que
  for ( uint8_t i = 1; i < count; i++ ) 
  {    // Loop uma vez para cada entrada EEPROM
    readID(i);          // Leia um ID da EEPROM, ele ?? armazenado no cart??o armazenado [4]
    if ( checkTwo( find, storedCard ) )
    {   // Verifique se o cart??o armazenado l?? da EEPROM
      return true;
    }
    else 
    {    // Caso contr??rio, retorne false
    }
  }
  return false;
}

///////////////////////////////////////// Gravar Sucesso na EEPROM   ///////////////////////////////////
//--------- Pisca o LED verde 3 vezes para indicar uma grava????o bem-sucedida na EEPROM
void successWrite() 
{
  digitalWrite(blueLed, LED_OFF);   // Verifique se o LED azul est?? apagado
  digitalWrite(redLed, LED_OFF);   // Verifique se o LED VERMELHO est?? apagado
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED VERDE est?? apagado
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Verifique se o LED VERDE est?? ACESO
  delay(200);
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED VERDE est?? apagado
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Verifique se o LED VERDE est?? ACESO
  delay(200);
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED VERDE est?? apagado
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Verifique se o LED VERDE est?? ACESO
  delay(200);
}

///////////////////////////////////////// Falha na grava????o na EEPROM   ///////////////////////////////////
// Pisca o LED vermelho 3 vezes para indicar uma falha na grava????o na EEPROM
void failedWrite()
{
  digitalWrite(blueLed, LED_OFF);   // Verifique se o LED azul est?? apagado
  digitalWrite(redLed, LED_OFF);  // Verifique se o LED VERMELHO est?? apagado
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED VERDE est?? apagado
  delay(200);
  digitalWrite(redLed, LED_ON);   // Verifique se o LED vermelho est?? ACESO
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Verifique se o LED vermelho est?? apagado
  delay(200);
  digitalWrite(redLed, LED_ON);   // Verifique se o LED vermelho est?? aceso
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Verifique se o LED vermelho est?? apagado
  delay(200);
  digitalWrite(redLed, LED_ON);   // Verifique se o LED vermelho est?? aceso
  delay(200);
}

///////////////////////////////////////// Sucesso remover UID da EEPROM  ///////////////////////////////////
// Pisca o LED azul 3 vezes para indicar uma exclus??o bem-sucedida na EEPROM
void successDelete()
{
  digitalWrite(blueLed, LED_OFF);   // Verifique se o LED azul est?? apagado
  digitalWrite(redLed, LED_OFF);    // Verifique se o LED vermelho est?? apagado
  digitalWrite(greenLed, LED_OFF);  // Verifique se o LED verde est?? apagado
  delay(200);
  digitalWrite(blueLed, LED_ON);  // Verifique se o LED azul est?? aceso
  delay(200);
  digitalWrite(blueLed, LED_OFF);   // Verifique se o LED azul est?? apagado
  delay(200);
  digitalWrite(blueLed, LED_ON);  // Verifique se o LED azul est?? aceso
  delay(200);
  digitalWrite(blueLed, LED_OFF);   // Verifique se o LED azul est?? apagado
  delay(200);
  digitalWrite(blueLed, LED_ON);  // Verifique se o LED azul est?? aceso
  delay(200);
}

////////////////////// Verifique o cart??o lido----SE ?? masterCard   ///////////////////////////////////

//----------- Verifique se o ID passado ?? o cart??o de programa????o principal
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
