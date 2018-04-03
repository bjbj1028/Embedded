import time
import RPi.GPIO as GPIO
import pygame.mixer
from sys import exit

GPIO.setmode(GPIO.BCM)
GPIO.setup(21, GPIO.OUT)
GPIO.setup(24, GPIO.IN)
GPIO.setup(17, GPIO.OUT)

trig = 13
echo = 19
GPIO.setup(trig, GPIO.OUT)
GPIO.setup(echo, GPIO.IN)

motor = GPIO.PWM(17,50)
motor.start(7.5)

pygame.mixer.init(48000, -16, 1, 1024)
soundA = pygame.mixer.Sound("center.wav")
soundChannelA = pygame.mixer.Channel(1)
print "Soundboard Ready."

try :
       while True :
        inputIO = GPIO.input(24)
        GPIO.output(trig, False)
        time.sleep(0.5)
        GPIO.output(trig, True)
        time.sleep(0.00001)
        GPIO.output(trig, False)

        while GPIO.input(echo) == 0 :
           pulse_start = time.time()
        while GPIO.input(echo) == 1 :
           pulse_end = time.time()

        pulse_duration = pulse_end - pulse_start
        distance = pulse_duration*17000
        distance = round(distance, 2)
        if inputIO == True:
         if distance < 5 :
           GPIO.output(21, GPIO.HIGH)
           motor.ChangeDutyCycle(12.5)
           time.sleep(2)
           motor.ChangeDutyCycle(2.5)
           soundChannelA.play(soundA)
        else:
           GPIO.output(21, GPIO.LOW)
except KeyboardInterrupt :
    GPIO.cleanup()
