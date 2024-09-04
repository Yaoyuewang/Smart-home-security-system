#! /hajpi/bin/env python
# /etc/init.d/messenger.py
### BEGIN INIT INFO
# Provides:
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start daemon at boot time
# Description:       Enable service provided by daemon.
### END INIT INFO

import sys
import os
import RPi.GPIO as GPIO
import pygame
import time

os.system("pulseaudio --start")

pygame.mixer.init(frequency=44100, size=-16, channels=2)
channels = [pygame.mixer.Channel(i) for i in range(pygame.mixer.get_num_channels())]

# GPIO Set Up
GPIO.setmode(GPIO.BCM)
#GPIO.setup(16, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)
GPIO.setup(25, GPIO.IN, pull_up_down = GPIO.PUD_UP)
GPIO.setup(22, GPIO.IN, pull_up_down = GPIO.PUD_UP)

def play_audio(num):
    sounds = {
        1: pygame.mixer.Sound('Downloads/alarm.wav'),
        2: pygame.mixer.Sound('Downloads/blender.mp3'),
        3: pygame.mixer.Sound('Downloads/cheers.mp3'),
        4: pygame.mixer.Sound('Downloads/nanovna.mp3'),
        5: pygame.mixer.Sound('Downloads/cookingstart.mp3'),
        6: pygame.mixer.Sound('Downloads/frying.mp3'),
        7: pygame.mixer.Sound('Downloads/music.mp3'), 
        8: pygame.mixer.Sound('Downloads/CNN.mp3'),
        9: pygame.mixer.Sound('Downloads/dog.mp3'),
        10: pygame.mixer.Sound('Downloads/vacuum.mp3')
    }

    channel = channels[0]

    sound = sounds.get(num)
    if sound:
        if channel.get_busy():
            channel.stop()
        channel.play(sound)


def debounce_input(pin, threshold=1, delay=0.05):
    count = 0
    for i in range(threshold):
        if GPIO.input(pin) == GPIO.LOW:
            count += 1
        else:
            return False
    return True

if __name__ == "__main__":
    counter = 0
    counter2 = 0
    sent = False

    print("Your system has been started!!!")
    #os.system("paplay Downloads/nanovna.mp3")
    #time.sleep(5)
    play_audio(4)
    time.sleep(3)

    os.system("sendmail -t shmonitoring459@gmail.com < start.txt")

    # Constant Check for ATMega Input
    while(True):
        if debounce_input(25):
            if not sent:
                os.system("sendmail -t shmonitoring459@gmail.com < door.txt")
                sent = True
            while(True):
                play_audio(1)
                time.sleep(5)
        elif debounce_input(22):
            if counter % 3 == 0:
                if counter != 0:
                    counter2+=1
                if counter2 % 2 == 1:
                    play_audio(2)
                    #play_audio2(5)
                    time.sleep(4)
                    play_audio(8)
                    time.sleep(10)
                    #play_audio2(6)
                else:
                    play_audio(5)
                    time.sleep(4)
                    play_audio(6)
                    time.sleep(10)
            elif counter % 3 == 1:
                if counter2 % 2 == 1:
                    play_audio(9)
                    time.sleep(4)
                else:
                    play_audio(10)
                    time.sleep(4)
            elif counter % 3 == 2:
                play_audio(7)
                time.sleep(15)
                #play_audio2(3)
        else:
            channels[0].stop
            counter+=1

