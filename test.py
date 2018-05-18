from ctypes import *
import time

todlib = cdll.LoadLibrary('./tod.so')
todlib.Open()


todlib.SetMotor(0, 100)
#todlib.SetMotor(1, -100)
time.sleep(1)

todlib.SetMotor(0, 0)
#todlib.SetMotor(1, 0)

todlib.Close()
