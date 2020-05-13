# I don't know Python
# I don't care for Python

import time
import datetime
import serial

# Create array/object of the system time
sysTime = [72, 0, 77, 0, 83, 0]     #72="H"  77="M"  83="S"
                                    #Format is HxMxSx... Hours Minutes Seconds

# Open Serial port 
# 
ser1 = serial.Serial("/dev/ttyACM0", 9600)
#ser2 = serial.Serial("/dev/ttyACM1", 9600)

while 1:
    dt = datetime.datetime.now()    # get current system date and time from RPi
    sysTime[1] = dt.hour            # set Hour data
    sysTime[3] = dt.minute          # set Minutes
    sysTime[5] = dt.second          # set Seconds

#write data to Serial port
    ser1.write(sysTime)
#    ser2.write(sysTime)

# 1 second Send interval
    time.sleep(1)
