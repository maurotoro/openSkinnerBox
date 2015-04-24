Arduino Interface to DIY SkinnerBox:
* 1 TTL Lever
* 1 Solenoid Pinch Valve
* 2 Lights
* 1 Lickometer, based in Vajnerova 2002, look in references

All files in the repository are distributed under CRAPL Licence, a copy of wich is included in the respective folder.


The Arduino File implements 4 different experiments:

* 0 Conditioning
* 1 Resistance to Extintion
* 2 Fixed Ration Schedule of Reinforcement
* 3 Progressive Ratio Schedule of Reinforcement


they can be called via commandline with the python script
or via the serial Monitor in Arduino IDE. to call any of them:

Experiment Line	
* "0"	  Conditioning: infinite looop, any lever press is rewarded
* "1 1"	Resistance to extintion 0% reward probability: 30 mins of no reward
* "1 2" Resistance to extintion 25% reward probability: 30 mins, 25% of the first 20 trials are rewarded 
* "1 3"	Resistance to extintion 50% reward probability: 30 mins, 50% of the first 20 trials are rewarded
* "1 4"	Resistance to extintion 75% reward probability: 30 mins, 75% of the first 20 trials are rewarded
* "1 5"	Resistance to extintion 100% reward probability: 30 mins, all of them are rewarded
* "2 *n*"	Fixed Ratio: every *n* lever presses a reward is given
* "3 *n*"	Progresive Ratio: the lever presses needed to give a reward increases by *n* every trial, *n*x1,*n*x2,...


The python script allows to start a experiment and save the data to a file,
To start a experiment in this way you need to have installed pySerial, and python...
to call it you should use the following line:
pyduinoSKNNRBX '/dev/ARDUINO' 'Experiment Line' RatNumber

Any commentaries or help would be really helpfull, and must be sent to my personal email:
mauro.toro.e at gmail.com

Keep in mind that all of this is part of my Ms thesis, so really any help will help!! And, it could blow any of your equipment...

Suerte!

SoyUnKoPe 2013

PS: The references added are probably not legal, if that's bad please tell me and I will rm them. 
