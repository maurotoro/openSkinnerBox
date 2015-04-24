/*
OpenSkinnerBox
 	Arduino implementation of a Skinner Box with:
 		one lever,
 		a light over that lever,
 		a solenoid pinch valvle to give liquid rewards
 		a lickometer to count the number of licks over the dispensator
 
 	running one of 4 experimental settings:
 		conditioning:
 				Every lever press gives one reward
 				runs until closed
 		fixed ratio:
 				Give a reward every N lever presses
 				runs for 30 minutes
 		Progressive Ratio:
 				Give a reward every N*trial lever presses
 				runs for 30 minutes
 		Resistance to Extintion:
 				Give a reward in one of 5 probabilities (0, 25, 50, 75, 100)
 				during the first 20 trials, just count the rest
 				runs for 30 minutes
 	Every time a reward it's given the lickometer state it's sampled at 33 Hz (every 30 ms
 	check the state of the lickometer) for 3 seconds. Or, the lickings on the reward dispenser are counted, and the timestamp of first lick and end of sensing are send by serial port.
 
 	The data thas its printed in the serial port goes at a baudrate of 9600, and it's form
 	atted in this way:
 	  
 	1	<Experiment Name>     <Start of Experiment>	<Experiment Comments>
 	2	block  trial   leverPressCount	leverPress	leverRelease	reward	rewStart	rewEnd  LickoMeterData
 	3       1       1       14249   14729   0       0       0	
 	...
 	N	"End_of_Expe"	
 
 	This script it's given to the interweb under CRAPL license  a copy of which should be in the project folder
 soyunkope 2013.
 */

//libraries included, should be on your arduino library folder
#include <Streaming.h>	
#include <StopWatch.h>


/*
the pin declaration:
 To digital pin 13 of arduino connect the lever wire
 To digital pin 11 of arduino connect the solenoid valvle wire
 To digital pin 10 of arduino connect the right light wire
 To digital pin 9 of arduino connect the central light wire
 To digital pin 7 of arduino connect the lickometer wire
 */
const int ttlDE=13; 
const int dispRew=11;
const int lzDE=10;
const int lzCE=9;
const int licko=7;

// Definition of some experimental variables
unsigned int bloque=1;		//# block
unsigned long trial=0;		//# trial
unsigned long pressCount=1;	//# lever presses counter
unsigned long palancaPress;	// lever press timestamp
unsigned long palancaRelease;	// lever release timestamp
unsigned long reward=0;		// rewarded or not trial
unsigned long rewStart;		// reward start timestamp
unsigned long rewEnd;		// reward end timestamp
unsigned long trialEnd;		// trial end timestamp
int ost=300;                    // OpenSolenoid Time, how long to give reward

// Lickometer Data variables and arrays
const int samples=100;			//Maximun number of samples, "empirically" chosen
int  val=0;                           //variable to save the number of licks during reward
//volatile unsigned long val[samples];    //array to save the lickometer state in every sample
volatile unsigned long tval[samples];	//array to save the lickometer read timestamp
unsigned int value=0;			//variable to save the lickometer state 
unsigned long tvalue=0;			//variable to save the lickometer read timestamp
int senseCount=0;			//variable to count the number of read
unsigned long lickoStart;		//variable to save the lickometer routine start 

//variables to communicate with serial port
int expe=-1;                    //experiment to run
int tipo=-1;                    //varible of experiment to run, not applies in acconditioning 
int pal=0;			//value of the lever (0 pressed; 1 released)	
int ratio;			//Ratio of Progressive or Fixed Ratio experiments
const long dura= 30*60000;	//Duration of experiments in ms

//Create the stopwatchs for the experiments and sensing routines
StopWatch blockT;
StopWatch senseT;

// Arrays of rewards for Resistance to Extintion experiment
int trial00[]={
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  // Trial 0% reward
int trial25[]={
  1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  // Trial 25% reward
int trial50[]={
  1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0};	// Trial 50% reward
int trial75[]={
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0};	// Trial 75% reward
int trial100[]={
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};	// Trial 100% reward

// A function to randomize the rewards arrays for R2E, runs once every reboot
void randomizeLST(int list[])
{	
  randomSeed(analogRead(3));
  for (int a=0; a<20; a++){
    byte r=random(a,19); 
    byte temp=list[a];
    list[a]=list[r];
    list[r]=temp;
  }
}

/*
A function that samples the lickometer pin 100 times every sample ms 
 	Turns the lever light off, then the reward light on
 	Starts the stopwatch for sensing, and takes a timestamp
 	while the sample counter it's less that 100
 		Reads the lickometer pin and saves it's value and read timestamp
 	When the sample counter gets to 100, prints all values 
 	Turns reward light off and lever light on

void sense(int sample){
 digitalWrite(lzDE,LOW);
 digitalWrite(lzCE,HIGH);
 senseT.start();
 lickoStart=senseT.elapsed();
 while (senseCount < samples)
 {
 if (senseT.elapsed()!=0 && senseT.elapsed()%sample == 0)
 {        
 val[senseCount]=digitalRead(licko);
 tval[senseCount]=senseT.elapsed()-lickoStart;
 senseCount++;
 delay(1);
 }
 if (senseCount == samples)
 {
 senseCount=0;
 for (int i = 0; i<samples; i++)
 {
 Serial <<"\t" << val[i];
 }
 Serial << "\t";
 for (int j=0; j<samples; j++)
 {
 Serial <<"\t"<< tval[j];
 }
 digitalWrite(lzCE,LOW);
 digitalWrite(lzDE,HIGH);  
 break;
 }
 }
 }//*/
/* 
 Sense function that only counts the number of licks, for 3 seconds
 quite non working, yet
*/
void sense(int sample){
  digitalWrite(lzDE,LOW);
  digitalWrite(lzCE,HIGH);
  senseT.start();
  int flami=0;     //first lick mark
  int tflami=0;    //first lick timestamp
  while (true){
    if (senseT.elapsed()>=3000){
        digitalWrite(lzCE,LOW);
        digitalWrite(lzDE,HIGH);
        Serial << "\t" << val << "\t" << tflami << "\t" << senseT.elapsed();
        senseT.stop();
        senseT.reset();
        val=0;
        break;
    }

    if (digitalRead(licko)!=1){
      val++;
      int lami=0;
      if (flami==0)
      {
      tflami=senseT.elapsed();
      flami=1;
      }
      while (lami==0)
      {
        delay(1);
        lami=(digitalRead(licko));
        if (senseT.elapsed()>=3000){break;}
      }
    }
  }
}//*/

/*
 Arduino's setup, the serial port its opened at 9600 bauds all light and reward solenoid pins are declared output,
 and lever and lickometer are declared input and set to high impedance. Finally, the pobability arrays are randomized
 */
void setup()
{
  Serial.begin(9600);		
  pinMode(lzDE,OUTPUT);
  pinMode(lzCE, OUTPUT);
  pinMode(dispRew, OUTPUT);
  pinMode(ttlDE, INPUT);
  pinMode(licko,INPUT);
  digitalWrite(ttlDE, HIGH);
  digitalWrite(licko, HIGH);

  randomizeLST(trial25);
  randomizeLST(trial50);
  randomizeLST(trial75);
}

/*
Conditioning Experiment:
 	Start the block stopwatch
 	Print the data columns names
 	Light the Lever Light and start checking the lever,
 		if it gets pressed, get the timestamp and wait until it's realesed
 	take the release timestamp, give a reward, take the start and end timestamps, 
 	print the block, trial, lever press, lever release, reward state, reward start and reward end
 	Sense the lickometer for 3 seconds and print that data, en the line and start the next trial
 It never ends!!!
 */
void acco()
{
  blockT.start();
  trial=0;
  reward=1;
  Serial << "bloque\t" << "trial\t" << "PalancaPress\t" << "PalancaRelease\t" << "rewardState\t" << "rewStart\t" << "rewEnd\t"<< "LickoMeterData\t"<<endl;
  digitalWrite(lzDE,HIGH); 
  while (trial+1>0)
  { 
    if (digitalRead(ttlDE)!=1)
    {
      palancaPress=blockT.elapsed();
      pal=0;
      while (pal==0)
      {
        delay(1);
        pal=(digitalRead(ttlDE));     
      }
      palancaRelease=blockT.elapsed();
      digitalWrite(dispRew, HIGH);
      rewStart=blockT.elapsed();
      delay(ost);
      digitalWrite(dispRew, LOW);
      rewEnd=blockT.elapsed();  
      Serial << bloque <<"\t" << trial+1  << "\t" << palancaPress << "\t" << palancaRelease << "\t" << reward << "\t" << rewStart << "\t" << rewEnd << "\t";
      sense(30);
      trialEnd=blockT.elapsed();
      Serial << endl;
      trial++;
    } 
  }
}

/*
Resistance to Extintion (R2E) Experiment
 	Start the block stopwatch
 	Print the data columns names
 	while the time elapsed it's less that 30 minutes
 		Check if this trials it lesser that the 20th and gives or not reward
 		Wait for lever press and release and take those timestamps
 		if applies give reward and sense the lickometer
 		print the data 
 */
void restExt(int lista[])
{
  trial=0;
  blockT.reset();
  delay(1);
  blockT.start();
  Serial << "bloque\t" << "trial\t" << "PalancaPress\t" << "PalancaRelease\t" << "rewardState\t" << "rewStart\t" << "rewEnd\t"<< "LickoMeterData\t"<<endl;
  while (blockT.elapsed() < dura)
  {
    if (trial <20)
    {
      reward=lista[trial];
    }
    else if (trial > 19)
    {
      reward =0;
    }
    digitalWrite(lzDE,HIGH);  
    if (digitalRead(ttlDE)!=1)
    {
      palancaPress=blockT.elapsed();
      pal=0;
      while (pal==0)
      {
        delay(1);
        pal=(digitalRead(ttlDE));     
      }
      palancaRelease=blockT.elapsed();
      if (reward==1)
      {
        digitalWrite(dispRew, HIGH);
        rewStart=blockT.elapsed();
        delay(ost);
        digitalWrite(dispRew, LOW);
        rewEnd=blockT.elapsed();
      }
      else if (reward==0)
      {
        rewStart=blockT.elapsed();

        rewEnd=blockT.elapsed();      
      }
      Serial << bloque <<"\t" << trial+1  << "\t" << palancaPress << "\t" << palancaRelease << "\t" << reward << "\t" << rewStart << "\t" << rewEnd << "\t";
      sense(30);
      Serial << endl;
      trial++;
    }  
    if (blockT.elapsed() >= dura)
    {
      Serial <<"End of Expe"<<endl;
      digitalWrite(lzDE,LOW);
      blockT.stop(); 
      break;
    }
  } 
}

/*
Fixed Ratio Schedule of Reinforcements FRSR Experiment
 	Start the block stopwatch
 	Print the data columns names
 	while the time elapsed it's less that 30 minutes
 		Check if the rat has pressed ratio times, if so give reward, else dont
 */
void fixRat(int ratio)
{ 
  blockT.reset();
  trial = 0;
  Serial << "bloque\t" << "trial\t" << "pressCount\t" << "palancaPress\t" << "palancaRelease\t" << "reward\t" << "rewStart\t" << "rewEnd\t"<< "LickoMeterData" << endl;
  delay(1);
  blockT.start();
  while (blockT.elapsed() < dura ) 
  {
    digitalWrite(lzDE, HIGH);
    if ((digitalRead(ttlDE) != 1) && (pressCount == ratio))
    { 
      reward=1;
      palancaPress=blockT.elapsed();      
      pal=0;
      while (pal==0)
      {
        delay(1);
        pal=(digitalRead(ttlDE));     
      }
      palancaRelease=blockT.elapsed();
      digitalWrite(dispRew, HIGH);
      rewStart=blockT.elapsed();
      delay(ost);
      digitalWrite(dispRew, LOW);
      rewEnd=blockT.elapsed();
      Serial << bloque <<"\t" << trial+1  << "\t"<< pressCount << "\t" << palancaPress << "\t" << palancaRelease << "\t" << reward << "\t" << rewStart << "\t" << rewEnd << "\t";
      sense(30);
      Serial << endl;
      trial++;
      pressCount=1;
    }
    else if ((digitalRead(ttlDE) != 1) && (pressCount != ratio))
    {
      reward=0;
      palancaPress=blockT.elapsed();
      pal=0;
      while (pal==0)
      {
        delay(1);
        pal=(digitalRead(ttlDE));     
      }
      palancaRelease=blockT.elapsed();
      rewStart=0;
      rewEnd=0;
      Serial << bloque << "\t" << trial+1 << "\t" << pressCount << "\t" << palancaPress << "\t" << palancaRelease << "\t" << reward << "\t" << rewStart << "\t" << rewEnd << "\t" << endl;
      pressCount++;
    }  	
    if (blockT.elapsed() >= dura)
    {
      Serial <<"End of Expe"<<endl;
      digitalWrite(lzDE,LOW); 
      blockT.stop();
      break;      
    }
  }
}

/*
Progressive Ratio Schedule of Reinforcements
 	Start the block stopwatch
 	Print the data columns names
 	while the time elapsed it's less that 30 minutes
 		Check if the rat has pressed Trial*Ratio times, if so give reward, else dont
 */
void progRat(int ratio)
{ 
  blockT.reset();
  trial = 0;
  Serial << "bloque\t" << "trial\t" << "pressCount\t" << "palancaPress\t" << "palancaRelease\t" << "reward\t" << "rewStart\t" << "rewEnd\t" << "LickoMeterData"<<endl;
  delay(1);
  blockT.start();
  while (blockT.elapsed() < dura ) 
  {
    digitalWrite(lzDE, HIGH);
    if ((digitalRead(ttlDE) != 1) && (pressCount == ratio*(trial+1)))
    { 
      reward=1;
      palancaPress=blockT.elapsed();      
      pal=0;
      while (pal==0)
      {
        delay(1);
        pal=(digitalRead(ttlDE));     
      }
      palancaRelease=blockT.elapsed();
      digitalWrite(dispRew, HIGH);
      rewStart=blockT.elapsed();
      delay(ost);
      digitalWrite(dispRew, LOW);
      rewEnd=blockT.elapsed();
      Serial << bloque <<"\t" << trial+1  << "\t" << pressCount << "\t" << palancaPress << "\t" << palancaRelease << "\t" << reward << "\t" << rewStart << "\t" << rewEnd << "\t";
      sense(30);
      Serial << endl;
      trial++;
      pressCount=1;
    }
    else if ((digitalRead(ttlDE) != 1) && (pressCount != ratio*(trial+1)))
    {
      reward=0;
      palancaPress=blockT.elapsed();
      pal=0;
      while (pal==0)
      {
        delay(1);
        pal=(digitalRead(ttlDE));     
      }
      palancaRelease=blockT.elapsed();
      rewStart=0;
      rewEnd=0;
      Serial << bloque << "\t" << trial+1 << "\t" << pressCount << "\t" << palancaPress << "\t" << palancaRelease << "\t" << reward << "\t" << rewStart << "\t" << rewEnd << "\t" << endl;
      pressCount++;
    }  	
    if (blockT.elapsed() >= dura)
    {
      Serial <<"End of Expe"<<endl;
      digitalWrite(lzDE,LOW); 
      blockT.stop();
      break;      
    }
  }
}


/*
Arduino Loop
 Eternally check if something has been sended in the serial port, and if there's something that could be translated into a experiment
 run that routine.
 */
void loop()
{
  if (Serial.available()>0)
  {
    expe = Serial.parseInt();
    tipo = Serial.parseInt();
  }
  if (expe != -1)
  {
    if (expe ==0)
    {
      Serial << "Acondicionamiento Start:\t" << millis() << "\t100% probability rew"<< endl;
      acco();
    }
    else if ( expe == 1)
    {
      if (tipo == 1)
      {
        Serial << "Resistance to Extintion Start:\t" << millis() << "\t0% probabiliy Rew" <<endl;
        restExt(trial00); 
        expe=-1;
      }

      else if (tipo == 2)
      {
        Serial << "Resistance to Extintion Start:\t" << millis() << "\t25% probabiliy Rew" <<endl;
        restExt(trial25); 
        expe=-1;

      }
      else if (tipo == 3)
      {
        Serial << "Resistance to Extintion Start:\t" << millis() << "\t50% probabiliy Rew" <<endl;
        restExt(trial50); 
        expe=-1;
      }
      else if (tipo == 4)
      {
        Serial << "Resistance to Extintion Start:\t" << millis() << "\t75% probabiliy Rew" <<endl;
        restExt(trial75); 
        expe=-1;
      }
      else if (tipo == 5)
      {
        Serial << "Resistance to Extintion Start:\t" << millis() << "\t100% probabiliy Rew" <<endl;
        restExt(trial100); 
        expe=-1;
      }
    }
    else if (expe == 2)
    {
      if (tipo !=-1)
      {
        ratio=tipo;
        Serial << "Fixed Ratio\t" << millis() << "\t" <<"Ratio\t" << ratio << endl;
        fixRat(ratio); 
        expe=-1;
      }
      else
      {
        Serial << "Agrega un Ratio para comenzar el experimento"<<endl;
        Serial<< "End of Expe" << endl;
      }
    }
    else if (expe == 3)
    {
      if (tipo != -1)
      {
        ratio=tipo;
        Serial << "Progressive Ratio\t" << millis() << "\t" <<"Ratio\t" << ratio << endl;
        progRat(ratio); 
        expe=-1;
      }
      else
      {
        Serial << "Agrega un Ratio para comenzar el experimento"<<endl;
        Serial<< "End of Expe" << endl;
      }
    }
  }
}


