# Importa bibliotecas necessárias
from tensorflow.keras.models import load_model   # Para carregar o modelo treinado (rede neural)
import numpy as np                                # Para lidar com arrays e imagens
import cv2                                        # Para capturar vídeo e mostrar imagem na tela
import serial                                     # Para comunicação com o Arduino
import time
from collections import deque, Counter            # Para armazenar e contar predições recentes

# Carrega o modelo treinado do Teachable Machine
model = load_model('Keras_model.h5')

# Cria uma matriz (array) para armazenar a imagem normalizada que será analisada pelo modelo
data = np.ndarray(shape=(1, 224, 224, 3), dtype=np.float32)

# Inicia a captura de vídeo da webcam
cap = cv2.VideoCapture(1, cv2.CAP_DSHOW)

# Inicia a conexão serial com o Arduino (ajuste 'COM12' se necessário)
ser = serial.Serial('COM7', 9600)
time.sleep(2)  # Espera 2 segundos para o Arduino estar pronto

# Lista com os nomes das classes treinadas no modelo
classes = ['Rolamento', 'Porca', 'Parafuso', 'Vazia']

# Mapeia cada classe para uma posição do motor de passo (valores de 0 a 3)
stepper_positions = {
    'Rolamento': 0,
    'Porca': 1,
    'Parafuso': 2,
    'Vazia': 3
}

# Define limite mínimo de confiança e tamanho do histórico de predições
CONFIDENCE_THRESHOLD = 0.7
PREDICTION_BUFFER_SIZE = 10

# Cria um histórico circular (FIFO) para armazenar as últimas predições válidas
prediction_history = deque(maxlen=PREDICTION_BUFFER_SIZE)

# Armazena a última posição enviada ao motor para evitar repetições desnecessárias
last_sent_position = None

# Loop principal
while True:
    # Captura uma imagem da webcam
    success, img = cap.read()
    if not success:
        print("Erro na captura da imagem.")
        continue

    # Redimensiona a imagem para 224x224 (o tamanho que o modelo espera)
    img_resized = cv2.resize(img, (224, 224))

    # Normaliza a imagem (transforma os valores dos pixels entre -1 e 1)
    normalized_image_array = ((img_resized.astype(np.float32) / 127.0) - 1)
    data[0] = normalized_image_array

    # Usa o modelo para prever a classe da imagem atual
    prediction = model.predict(data, verbose=0)
    predicted_index = np.argmax(prediction)  # Índice da classe com maior confiança
    confidence = prediction[0][predicted_index]  # Nível de confiança
    predicted_class = classes[predicted_index]   # Nome da classe

    # Só adiciona ao histórico se a confiança for alta o suficiente
    if confidence >= CONFIDENCE_THRESHOLD:
        prediction_history.append(predicted_class)

    # Quando o buffer estiver cheio, analisa qual classe apareceu mais vezes
    if len(prediction_history) == PREDICTION_BUFFER_SIZE:
        most_common_class = Counter(prediction_history).most_common(1)[0][0]
        position_index = stepper_positions[most_common_class]  # Converte classe em posição (0 a 3)

        # Só envia comando se for uma nova posição diferente da anterior
        if position_index != last_sent_position:
            ser.write(f"{position_index}\n".encode())  # Envia para o Arduino via Serial
            last_sent_position = position_index
            print(f"Enviado: Posição {position_index} ({most_common_class})")

    # Mostra na tela a imagem da webcam com o nome da classe e confiança
    display_text = f"{predicted_class} ({confidence:.2f})"
    cv2.putText(img, display_text, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
    cv2.imshow('Reconhecimento de Objetos', img)

    # Se o usuário apertar a tecla 'q', o programa fecha
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Libera os recursos ao final
cap.release()
ser.close()
cv2.destroyAllWindows()
