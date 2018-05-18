from ctypes import *
import time
import select

import threading
import sys, termios, tty, os

ch = None

def getch():
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)

    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

class KeyboardThread(threading.Thread):
    def run(self):
        global ch
        global done
        while True:
            ch = getch()
            if ch == 'q':
                break

todlib = cdll.LoadLibrary('./tod.so')
todlib.Open()

lmul = 0.9
rmul = 1.0

def SetLeft(power):
    todlib.SetMotor(1, -int(power * lmul))

def SetRight(power):
    todlib.SetMotor(0, int(power * rmul))

def Forward():
    SetLeft(80)
    SetRight(80)

def Backward():
    SetLeft(-60)
    SetRight(-60)

def RotCW():
    SetLeft(80)
    SetRight(-80)

def RotCCW():
    SetLeft(-80)
    SetRight(80)

def Stop():
    #print('Stop')
    SetLeft(0)
    SetRight(0)

kt = KeyboardThread()
kt.start()

counter = 6

try:
    while True:
      if ch == 'w':
          Forward()
      elif ch == 's':
          Backward()
      elif ch == 'a':
          RotCCW()
      elif ch == 'd':
          RotCW()
      elif ch == 'q':
          break
      
      if ch is None and counter == 0:
          Stop()
      elif not (ch is None):
          counter = 3
          ch = None
      else:
          counter = counter - 1

      time.sleep(0.05)
except KeyboardInterrupt:
    pass
finally:
    pass
    #todlib.Close()

kt.join()
todlib.Close()
