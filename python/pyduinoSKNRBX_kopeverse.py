#!/usr/bin/python2.7
import sys
import serial
import time
import datetime

'''''
tiene que llamarse con 3 arg: arturito, experimento, nRata
Hay que hacer que mande por serial el expe a realizar
Experimento es un string con 1 o dos valores EJ: 0 ; '1,1'
Expeirmento CODA:
0 accond
        lzDE, ttlDE, 100% rew, no time limit
1,1 restExt(trial00)
        lzDE, ttlDE, RestExt 0% rew, 30 minutos
1,2 restExt(trial25)
        lzDE,ttlDE, RestExt 25% rew, 30 minutos
1,3 restExt(trial50)
        lzDE,ttlDE, RestExt 50% rew, 30 minutos
1,4 restExt(trial75)
        lzDE,ttlDE, RestExt7 75% rew, 30 minutos
1,5 restExt(trial100)
        lzDE,ttlDE, RestExt 100% rew, 30 minutos
2,n fixRat(n)
        lzDE,ttlDE,FixedRatio ratio=n, 30 minutos
3,n progRat(n)
        lzDE,ttlDE, ProgRatio ratio=n, 30 minutos
'''''
PORT = str(sys.argv[1]) # arturito
expe = str(sys.argv[2])
nRata = str(sys.argv[3])
date = time.asctime()
today = str(datetime.date.today()) 

with  open("/Personal/Deberes/U/Magister/A2012S02/Tesis/Data/"+nRata+"."+today+".log","a+") as f:
    f.write(date+" "+nRata+"\n"+"\n")

ser = serial.Serial(PORT,9600)
time.sleep(2)
ser.write(expe)

def writeData(value): #save value string to a log file nRata+today.log
    with  open("/Personal/Deberes/U/Magister/A2012S02/Tesis/Data/"+nRata+"."+today+".log","a+") as f:
        f.write(value)
        f.write("\n")
	f.close()
        print value


while True:
 try:
  if ser.inWaiting() > 0: # if data present in serial
        value = ser.readline() # Read the data from serial 
        if value == 'End of Expe\r\n':
	  ser.setDTR( level=False)
	  ser.flushInput()
	  ser.setDTR( level=True)
	  ser.close()
          break
        else:
          value = value.strip('\r\n') #take the eol
          writeData(value)

 except KeyboardInterrupt:
        print '\n Salida Forzada'
        ser.setDTR( level=False)
        ser.flushInput()
        ser.setDTR( level=True)
        ser.close()
        break

