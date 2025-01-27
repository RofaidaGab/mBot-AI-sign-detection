import os
import threading
import time
import cv2
import numpy as np
import tensorflow.lite as tflite
import socket

ARDUINO_IP = "192.168.64.90"
ARDUINO_PORT = 8080

# Initialize TFLite model
interpreter = tflite.Interpreter(model_path="E:/UNI/IntroToCoEProject/mbotComm/traffic_sign_model_final.tflite")
interpreter.allocate_tensors()
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

class_labels = ["C-Road", "C-Road End", "Start", "Stop"]
last_command_sent = None
arduino_socket = None

def connect_to_arduino():
    global arduino_socket
    try:
        arduino_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        arduino_socket.settimeout(1)
        arduino_socket.connect((ARDUINO_IP, ARDUINO_PORT))
        return True
    except:
        arduino_socket = None
        return False

def send_command_to_arduino(command):
    global arduino_socket
    try:
        if arduino_socket is None:
            if not connect_to_arduino():
                return
        arduino_socket.sendall(command.encode())
        print(f"Sent: {command}")
        return True
    except:
        arduino_socket = None
        return False

def preprocess_frame(frame):
    frame_resized = cv2.resize(frame, (128, 128))
    frame_rgb = cv2.cvtColor(frame_resized, cv2.COLOR_BGR2RGB)
    return np.expand_dims(frame_rgb, axis=0).astype(np.float32) / 255.0

def predict(frame):
    input_data = preprocess_frame(frame)
    interpreter.set_tensor(input_details[0]['index'], input_data)
    interpreter.invoke()
    predictions = interpreter.get_tensor(output_details[0]['index'])[0]
    predicted_class = np.argmax(predictions)
    return class_labels[predicted_class], predictions[predicted_class]

def fetch_frame_from_esp32():
    try:
        cap = cv2.VideoCapture(f'http://{ARDUINO_IP}:81/stream')
        if not cap.isOpened():
            return None
        ret, frame = cap.read()
        cap.release()
        return frame if ret else None
    except Exception as e:
        return None

def stream_images_from_esp32():
    global last_command_sent
    cv2.namedWindow('ESP32 Stream', cv2.WINDOW_AUTOSIZE)
    
    while True:
        frame = fetch_frame_from_esp32()
        if frame is None:
            time.sleep(0.1)
            continue
        
        try:
            sign, confidence = predict(frame)
            
            cv2.putText(frame, f"{sign}: {confidence:.2f}", 
                      (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, 
                      (0, 255, 0), 2)
            cv2.imshow('ESP32 Stream', frame)
            
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
                
            if confidence < 0.6:
                continue
                
            command = None
            if sign == "Stop" and confidence >= 0.85:
                command = "stop"
            elif sign == "Start" and confidence >= 0.99:
                command = "start"
            elif sign == "C-Road" and confidence >= 0.8:
                command = "c-road"
            elif sign == "C-Road End" and confidence >= 0.8:
                command = "c-road-end"
                
            if command and command != last_command_sent:
                if send_command_to_arduino(command):
                    last_command_sent = command
                    print(f"Detected: {sign} ({confidence:.2f})")
                    
        except Exception as e:
            print(f"Error: {e}")
            time.sleep(0.1)
            
    cv2.destroyAllWindows()

if __name__ == "__main__":
    stream_images_from_esp32()
