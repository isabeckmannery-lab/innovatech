// Motor de passo 17HS4401S com TB6600 - Versão Python Compatível
const int STEP_PIN = 11;   // PUL+
const int DIR_PIN = 5;    // DIR+  
const int EN_PIN = 8;     // ENA+

const int MICROSTEPS = 1;
const int STEPS_PER_REVOLUTION = 800 * MICROSTEPS;
const int STEPS_PER_90_DEGREES = STEPS_PER_REVOLUTION / 4;
const int STEP_DELAY_US = 2000;

int currentPosition = 0;  // Posição atual (0-3)
String inputString = "";  // Buffer para dados seriais
bool stringComplete = false;

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);  // Ativa driver
  
  Serial.begin(9600);
  Serial.println("Sistema iniciado - Aguardando comandos do Python");
  Serial.println("Pronto para receber: 0, 1, 2, 3");
  
  // Move para posição inicial (0 graus)
  moveToPosition(0);
}

void loop() {
  // Processa comandos recebidos via serial
  if (stringComplete) {
    inputString.trim(); // Remove espaços e quebras de linha
    
    Serial.print("Recebido: '");
    Serial.print(inputString);
    Serial.println("'");
    
    // Verifica se é um comando de posição válido (0-3)
    if (inputString.length() == 1 && inputString >= "0" && inputString <= "3") {
      int targetPosition = inputString.toInt();
      moveToPosition(targetPosition);
    } 
    // Comando de teste
    else if (inputString == "t") {
      testMotor();
    }
    // Comando de status
    else if (inputString == "s") {
      printStatus();
    }
    else {
      Serial.println("Comando inválido. Use: 0, 1, 2, 3, t (teste), s (status)");
    }
    
    // Limpa buffer para próximo comando
    inputString = "";
    stringComplete = false;
  }
}

void moveToPosition(int targetPosition) {
  // Calcula diferença de posição (0-3)
  int diff = targetPosition - currentPosition;
  
  // Ajusta para caminho mais curto
  if (diff > 2) diff -= 4;
  if (diff < -2) diff += 4;
  
  if (diff == 0) {
    Serial.println("Já na posição " + String(targetPosition));
    return;
  }
  
  // Determina direção e quantidade de passos
  bool direction = diff > 0;
  int steps = abs(diff) * STEPS_PER_90_DEGREES;
  
  Serial.print("Movendo para posição ");
  Serial.print(targetPosition);
  Serial.print(" (");
  Serial.print(targetPosition * 90);
  Serial.print("°) - ");
  Serial.print(steps);
  Serial.println(" passos");
  
  // Executa movimento
  moveSteps(steps, direction);
  
  // Atualiza posição atual
  currentPosition = targetPosition;
  
  Serial.print("Movimento concluído. Posição atual: ");
  Serial.println(currentPosition);
  
  // Feedback para Python
  Serial.println("OK"); // Confirmação para o Python
}

void moveSteps(int steps, bool direction) {
  digitalWrite(DIR_PIN, direction ? HIGH : LOW);
  delay(10); // Pequena pausa para estabilização da direção
  
  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_DELAY_US / 2);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(STEP_DELAY_US / 2);
  }
}

void testMotor() {
  Serial.println(">>> INICIANDO TESTE DO MOTOR <<<");
  
  // Testa todas as posições
  for (int pos = 0; pos <= 3; pos++) {
    Serial.print("Testando posição ");
    Serial.println(pos);
    moveToPosition(pos);
    delay(1000); // Pausa de 1 segundo entre movimentos
  }
  
  // Volta para posição 0
  moveToPosition(0);
  Serial.println(">>> TESTE CONCLUÍDO <<<");
}

void printStatus() {
  Serial.println("=== STATUS DO SISTEMA ===");
  Serial.print("Posição atual: ");
  Serial.print(currentPosition);
  Serial.print(" (");
  Serial.print(currentPosition * 90);
  Serial.println("°)");
  Serial.print("STEP_PIN: ");
  Serial.println(digitalRead(STEP_PIN));
  Serial.print("DIR_PIN: ");
  Serial.println(digitalRead(DIR_PIN));
  Serial.print("EN_PIN: ");
  Serial.println(digitalRead(EN_PIN));
  Serial.println("========================");
}

// Função chamada automaticamente quando dados chegam pela serial
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    
    if (inChar == '\n') {
      stringComplete = true;
    } else if (inChar != '\r') { // Ignora carriage return
      inputString += inChar;
    }
  }
}