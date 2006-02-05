// -*- tab-width: 4 -*-
// MOD Loader for TRAXMOD Player
// Copyright (c) 2005, K9spud LLC.
// http://www.k9spud.com/
#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <iomanip>

#include "mod.h"
#include "row.h"

using namespace std;

#define PAL  7093789.2
#define NTSC 7159090.5
#define AMIGASPD NTSC / 2

const double mod::C2SPD[16] = { 
	(AMIGASPD / 428),
	(AMIGASPD / 425),
	(AMIGASPD / 422),
	(AMIGASPD / 419),
	(AMIGASPD / 416),
	(AMIGASPD / 413),
	(AMIGASPD / 410),
	(AMIGASPD / 407),
	(AMIGASPD / 453),
	(AMIGASPD / 450),
	(AMIGASPD / 447),
	(AMIGASPD / 444),
	(AMIGASPD / 441),
	(AMIGASPD / 437),
	(AMIGASPD / 434),
	(AMIGASPD / 431) };


mod::mod(char* sFileName)
{
  ifstream fIn;

  fIn.open(sFileName);

  // get length of file:
  fIn.seekg (0, ios::end);
  int iLength = fIn.tellg();
  fIn.seekg(0, ios::beg);


  iNumChannels = 4;
  NumSamples = 31;

  fIn.read(Title, 20);

  cout.setf(ios::hex);
  cout.setf(ios::uppercase);
  cout.width(2);
  cout.fill('0');

  cout << "Title: [" << Title << "]" << endl;

  // Read sample headers
  fIn.read((char *) &Samples[0], 31 * sizeof(SampleType));

  // Read number of Orders
  fIn.read((char *)&NumOrders, 1);
  fIn.seekg(1, ios::cur);

  // Read order table
  fIn.read((char *)OrderTable, 128);

  // Read magic string marker
  fIn.read(Format, 4);

  // Calculate the number of patterns based on the highest pattern number
  // listed in the order table.  
  NumPatterns = 0;
  for(int i = 0; i < NumOrders; ++i)
	{
	  int j = (unsigned int) OrderTable[i];
	  cout << "Order: " << i << " [" << j << "]" << endl;
	  if(OrderTable[i] >= NumPatterns)
		{
		  NumPatterns = OrderTable[i] + 1;
		}
	}

  cout << "Number of Patterns: " << (int)NumPatterns << endl;
  cout << "Number of Orders: " << (int)NumOrders << endl;

  Patterns = new unsigned char[(NumPatterns + 1) * 1024];
  oPat = new Pattern[NumPatterns];

  fIn.read((char *) Patterns, 1024 * NumPatterns);

  // Load the sample data
  for(int i = 0; i < NumSamples; ++i)
	{
	  unsigned int iLength = 
		2 * ( ((Samples[i].Length & 0xFF) << 8) | 
			  ((Samples[i].Length & 0xFF00) >> 8) );

	  if(iLength == 0)
		{
		  SampleData[i] = NULL;
		  continue;
		}

	  SampleData[i] = new char[iLength];

	  fIn.read(SampleData[i], iLength);
	}

  // Render the music instructions

  for(int iPat = 0; iPat < NumPatterns; ++iPat)
	{
	  oPat[iPat] = ProcessPattern(iPat);
	} // end pattern loop

  for(int iPat = 0; iPat < NumPatterns; ++iPat)
	{
	  unsigned char *ptr = Patterns + (1024 * iPat);

	  cout << "Pattern " << dec << iPat << endl;

	  for(int iRow = 0; iRow < 64; ++iRow)
		{
		  for(int iChan = 0; iChan < 4; ++iChan)
			{
			  
			  int iSample = (ptr[0] & 0xF0) | ((ptr[2] & 0xF0) >> 4);
			  int iPeriod = ((ptr[0] & 0x0F) << 8) | ptr[1];
			  int iEffect = ((ptr[2] & 0x0F) << 8) | ptr[3];
			  ptr += 4;

			  if(iPeriod == 0)
				{
				  cout << "--- ";
				}
			  else
				{
				  cout << hex << setw(3) << iPeriod << " ";
				}

			  if(iSample == 0)
				{
				  cout << "-- ";
				}
			  else
				{
				  cout << dec << setw(2) << iSample << " ";
				}

			  
			  if(iEffect == 0)
				{
				  cout << "--- | ";
				}
			  else
				{
				  cout << hex << setw(3) << iEffect << " | ";
				}
			}

		  cout << oPat[iPat][iRow] << endl;
		}

	  cout << dec << oPat[iPat].length() << " bytes\n" << endl;
	}
  
  writeSamples();
  writeMusic();
}

mod::~mod()
{

}

void mod::writeMusic(void)
{
  ofstream fOut("output/orderdata.h");
  fOut << "// Copyright (c) 2005, K9spud LLC." << endl;
  fOut << "// All rights reserved." << endl;
  fOut << endl;
  fOut << "#ifndef ORDERDATA_H" << endl;
  fOut << "#define ORDERDATA_H\n" << endl;
  fOut << "#define NUMORDERS " << (unsigned int) NumOrders << endl << endl;

  for(int i = 0; i < NumPatterns; ++i)
	{
	  fOut << "extern void Pattern" << i << "(void);" << endl;
	}
  fOut << endl;

  fOut << "inline unsigned int getPattern(unsigned int iOrder)" << endl;
  fOut << "{" << endl;
  fOut << "  switch(iOrder)" << endl;
  fOut << "  {" << endl;
  fOut << "    default:" << endl;
  for(int i = 0; i < NumOrders; ++i)
	{
	  unsigned int iPattern = OrderTable[i];

	  fOut << "    case " << i << ":" << endl;
	  fOut << "      return (unsigned int)(&Pattern" << iPattern << ") + ((unsigned int)(&Pattern" << iPattern << ") >> 1);" << endl;
	  fOut << endl;
	}
  fOut << "  }" << endl;
  fOut << "}\n" << endl;

  fOut << "#endif // ORDERDATA_H\n" << endl;

  fOut.close();


  fOut.open("output/patterndata.s");
  fOut << ";; Copyright (c) 2005, K9spud LLC." << endl;
  fOut << ";; All rights reserved." << endl;
  fOut << endl;

  fOut << ".text" << endl;

  for(int iPat = 0; iPat < NumPatterns; ++iPat)
	{
	  fOut << ".global _Pattern" << dec << iPat << endl;
	  fOut << "_Pattern" << dec << iPat << ":" << endl;
	  
	  fOut.setf(ios::hex);
	  fOut.setf(ios::uppercase);
	  fOut.width(2);
	  fOut.fill('0');

	  int iLength = oPat[iPat].length();
	  unsigned char* data = new unsigned char[iLength + 1];
	  oPat[iPat].getData(data);

	  for(int iData = 0; iData < iLength;)
		{
		  fOut << ".pbyte ";
		  for(int i = 0; i < 14; ++i)
			{
			  if(i)
				fOut << ",";
			  
			  unsigned int iTmp = data[iData++];

			  fOut << "0x" << hex << setw(2) << iTmp;
			  
			  if(iData >= iLength)
				break;
			}
		  fOut << endl;		  
		}
	  fOut << endl;
	}

  fOut.close();
}

void mod::writeSamples(void)
{
  ofstream fOut;
  char sFileName[1000];

  // First process all of the samples and see if there is any excess
  // sample loop end data that can be trimmed.
  for(int i = 0; i < NumSamples; ++i)
  {
    unsigned int iLength = 
  	2 * ( ((Samples[i].Length & 0xFF) << 8) | 
  		  ((Samples[i].Length & 0xFF00) >> 8) );
    unsigned int iLoopStart = 
	2 * ( ((Samples[i].LoopStart & 0xFF) << 8) | 
		  ((Samples[i].LoopStart & 0xFF00) >> 8) );
    unsigned int iLoopLength = 
 	2 * ( ((Samples[i].LoopLength & 0xFF) << 8) | 
		  ((Samples[i].LoopLength & 0xFF00) >> 8) );

    if(iLength == 0)
	{
	  // ABORT: no data
	  continue;
	}

	// Does this sample loop?
	if(iLoopLength > 2)
	  {
		// Yes
		if(iLength > (iLoopStart + iLoopLength))
		  {
			// Trim
			cout << "Trimming sample " << dec << (i+1) << " from " << iLength;
			iLength = iLoopStart + iLoopLength;
			cout << " down to " << iLength << endl;
			iLength = iLength >> 1;
			Samples[i].Length = ((iLength & 0xFF) << 8) | ((iLength & 0xFF00) >> 8);
		  }
	  }

	/*
    sprintf(sFileName, "output/sample%d.raw", i+1);
    fOut.open(sFileName);
    fOut.write(SampleData[i], iLength);
    fOut.close();
	*/
  }

  // Write the sample data C functions
  sprintf(sFileName, "output/samples.c");
  fOut.open(sFileName);
  fOut << "// Copyright (c) 2005, K9spud LLC." << endl;
  fOut << "// All rights reserved." << endl;
  fOut << endl;
  fOut << "#include \"mixer.h\"" << endl;
  fOut << endl;
  for(int i = 0; i < NumSamples; ++i)
  {
    unsigned int iLength = 
	  2 * ( ((Samples[i].Length & 0xFF) << 8) | 
			((Samples[i].Length & 0xFF00) >> 8) );

	if(iLength == 0)
	  continue;

	fOut << "extern void Sample" << (i+1) << "(void);" << endl;
  }
  fOut << endl;

  fOut << "void SetSample(ChannelType* pChan, unsigned int iSample)" << endl;
  fOut << "{" << endl;
  fOut << "  switch(iSample)" << endl;
  fOut << "  {" << endl;
  fOut << "    case 0:" << endl;
  fOut << "      return; // do nothing" << endl;
  fOut << endl;
  fOut << "    default:" << endl;
  for(int i = 0; i < NumSamples; ++i)
  {
    unsigned int iLength = 
	  2 * ( ((Samples[i].Length & 0xFF) << 8) | 
			((Samples[i].Length & 0xFF00) >> 8) );
	if(iLength == 0)
	  {
		fOut << "    case " << (i+1) << ":" << endl;		
	  }
  }
  fOut << "      pChan->bActive = 0;" << endl;
  fOut << "      break;\n" << endl;

  for(int i = 0; i < NumSamples; ++i)
  {
    unsigned int iLength = 
	  2 * ( ((Samples[i].Length & 0xFF) << 8) | 
			((Samples[i].Length & 0xFF00) >> 8) );
    unsigned int iLoopStart = 
	  2 * ( ((Samples[i].LoopStart & 0xFF) << 8) | 
			((Samples[i].LoopStart & 0xFF00) >> 8) );
    unsigned int iLoopLength = 
	  2 * ( ((Samples[i].LoopLength & 0xFF) << 8) | 
			((Samples[i].LoopLength & 0xFF00) >> 8) );
	
    if(iLength == 0)
	  {
		// ABORT: no data
		continue;
	  }

	fOut << "    case " << (i+1) << ":" << endl;
	fOut << "      // " << Samples[i].Name << endl;
	fOut << "      pChan->bActive = 1;" << endl;
	fOut << "      pChan->iLoc = (unsigned int)(&Sample" << (i+1) << ") + ((unsigned int)(&Sample" << (i+1) << ") >> 1);" << endl;

	// Does this sample loop?
	if(iLoopLength > 2)
	  {
		fOut << "      pChan->bLoop = 1;" << endl;
		fOut << "      pChan->iLoopStart = pChan->iLoc + " << iLoopStart << ";" << endl;
		fOut << "      pChan->iEnd = pChan->iLoc + " << (iLoopStart + iLoopLength) << ";" << endl;
	  }
	else
	  {
		fOut << "      pChan->bLoop = 0;" << endl;
		fOut << "      pChan->iEnd = pChan->iLoc + " << (iLength) << ";" << endl;
	  }

	fOut << "      break;" << endl;
    fOut << endl;
  }

  fOut << "  }" << endl;
  fOut << "  pChan->iSample = iSample;\n" << endl;
  fOut << "}" << endl;
  fOut.close();


  // Now write the compilable ASM hex dump of the sample data
  sprintf(sFileName, "output/sampledata.s");

  fOut.open(sFileName);
  fOut << ";; Copyright (c) 2005, K9spud LLC." << endl;
  fOut << ";; All rights reserved." << endl;
  fOut << endl;
  fOut << ".text" << endl;

  for(int i = 0; i < NumSamples; ++i)
  {
    unsigned int iLength = 
	  2 * ( ((Samples[i].Length & 0xFF) << 8) | 
			((Samples[i].Length & 0xFF00) >> 8) );
    unsigned int iLoopStart = 
	  2 * ( ((Samples[i].LoopStart & 0xFF) << 8) | 
			((Samples[i].LoopStart & 0xFF00) >> 8) );
    unsigned int iLoopLength = 
	  2 * ( ((Samples[i].LoopLength & 0xFF) << 8) | 
			((Samples[i].LoopLength & 0xFF00) >> 8) );
	
    if(iLength == 0)
	  {
		// ABORT: no data
		continue;
	  }

    fOut << ";; Sample     : " << (i+1) << ". " << Samples[i].Name << endl;
    fOut << ";; Length     : " << iLength << endl;
	if(iLoopLength > 2)
	  {
		fOut << ";; Loop Start : " << iLoopStart << endl;
		fOut << ";; Loop End   : " << (iLoopStart + iLoopLength) << endl;
	  }
    fOut << endl;
  }

  for(int i = 0; i < NumSamples; ++i)
  {
    unsigned int iLength = 
  	2 * ( ((Samples[i].Length & 0xFF) << 8) | 
  		  ((Samples[i].Length & 0xFF00) >> 8) );
    unsigned int iLoopStart = 
	2 * ( ((Samples[i].LoopStart & 0xFF) << 8) | 
		  ((Samples[i].LoopStart & 0xFF00) >> 8) );
    unsigned int iLoopLength = 
 	2 * ( ((Samples[i].LoopLength & 0xFF) << 8) | 
		  ((Samples[i].LoopLength & 0xFF00) >> 8) );

    if(iLength == 0)
	{
	  // ABORT: no data
	  continue;
	}

    fOut << ".global _Sample" << dec << (i+1) << endl;
    fOut << "_Sample" << dec << (i+1) << ":" << endl;
  
    fOut.setf(ios::hex);
    fOut.setf(ios::uppercase);
    fOut.width(2);
    fOut.fill('0');
    
    for(unsigned int iLoc = 0; iLoc < iLength;)
    {
	  fOut << ".pbyte ";
	  for(int j = 0; j < 14; ++j)
		{
		  if(j)
			fOut << ",";
		  
		  unsigned int iVal;
		  iVal = (unsigned char)SampleData[i][iLoc++];
		  
		  fOut << "0x" << hex << setw(2) << iVal;
		  if(iLoc >= iLength)
			{
			  break;
			}
		}
	  fOut << endl;
	}

	fOut << endl;
  }

  fOut << flush;
  fOut.close();
}

Pattern mod::ProcessPattern(int iPat)
{
  Pattern oPat;
  int iWait = 0;
  
  for(int iRow = 0; iRow < 64; ++iRow)
	{
	  Row oRow = ProcessRow(iPat, iRow);
	  
	  if(!oRow.IsEmpty() && iRow != 0)
		{
		  // This row contains data, therefore, we need to emit
		  // a row wait command in front of this row so that
		  // it will start on the right time.
		  oRow.addFront(cmdWait(iWait));
		  iWait = 0;
		}
	  
	  if(oRow.patternBreak())
		{
		  // This row emitted a pattern break, therefore we are done
		  // with this pattern;
		  // If there is any remaining waiting to be done, emit it.
		  if(iWait)
			{
			  oRow.add(cmdWait(iWait));
			  iWait = 0;
			}

		  oPat[iRow] = oRow;
		  return oPat;
		}
	  
	  int iRowWait = calcWait(iPat, iRow);
	  if(iRowWait)
		{
		  iWait += iRowWait;
		  
		  // Emit wait commands to bring us back into range
		  while(iWait >= 0xF)
			{				  
			  oRow.add(cmdWait(0xF));
			  iWait -= 0x0F;
			}
		}
	  
	  oPat[iRow] = oRow;
	} // end row loop
  
  // If there is any remaining waiting to be done, emit it.
  if(iWait)
	{
	  oPat[63].add(cmdWait(iWait));
	  iWait = 0;
	}
  
  // mark end of pattern
  oPat[63].addRear(cmdPatternBreak(0));
  
  return oPat;
}

Row mod::ProcessRow(int iPat, int iRow)
{
	Row oRow;
	bool bPatternBreak = false;
	unsigned int iBreakToRow = 0;
	
	unsigned char *ptr = Patterns + (1024 * iPat) + (4 * iNumChannels * iRow);
	for(int iChan = 0; iChan < iNumChannels; ++iChan)
	{
	  int iSample = (ptr[0] & 0xF0) | ((ptr[2] & 0xF0) >> 4);
	  int iPeriod = ((ptr[0] & 0x0F) << 8) | ptr[1];
	  int iEffect = ((ptr[2] & 0x0F) << 8) | ptr[3];
	  ptr += 4;

	  if((iEffect & 0xF00) == 0xD00)
		{
		  // Wait until the end of the row to emit pattern breaks.
		  bPatternBreak = true;
		  iBreakToRow = iEffect & 0xFF;
		  iEffect = 0;
		}
		   
	  if(iSample == 0 && iEffect == 0 && iPeriod == 0)
	    {
		  // nothing to do on this channel, skip it.
		  continue;
	    }


	  if((iEffect & 0xF00) == 0x300)
		{
		  // This row has a Porta to Note effect.
		  if(iPeriod != 0)
			{
			  oRow.add(cmdPortaNote(iPeriod, iEffect & 0xFF, iSample, iChan));
			  iPeriod = 0;
			  iEffect = 0;
			  iSample = 0;
			}		  
		}


	  // Is there a new note?
	  if(iPeriod != 0 || iSample != 0)
	  {
	    iPeriod = 
	      (int)((C2SPD[0] * iPeriod) / C2SPD[Samples[iSample-1].Finetune]);

	    // Do we need to set the volume based off the sample 
	    // volume? (or is there a volume effect about to
	    // override anyway?)
	    int iNoteVol;
	    if((iEffect & 0xF00) != 0xC00)
	    {
		  iNoteVol = Samples[iSample-1].Volume;
	    }
	    else
	    {
		  iNoteVol = iEffect & 0x0FF;
		  iEffect = 0;
	    }

		oRow.add(cmdNote(iPeriod, iSample, iNoteVol, iChan));
	  }

	  if(iEffect != 0)
	  {

		// We don't handle pattern delay effects here.
		if((iEffect & 0xFF0) != 0xEE0)
		  {
			oRow.add(cmdEffect(iEffect, iChan));
		  }
	  }
	}


	if(bPatternBreak)
	  {
		oRow.addRear(cmdPatternBreak(iBreakToRow));
	  }

	return oRow;
}


/**
 * If the specified row contains data, returns 0.
 * If the specified row contains no data, returns 1.
 * If the specified row contains Pattern Delay commands (EEx), returns the last
 * pattern delay command in the row.
 */
int mod::calcWait(int iPat, int iRow)
{
  bool bEmptyRow = true;
  int iPatDelay = 0;

	unsigned char *ptr = Patterns + (1024 * iPat) + (4 * iNumChannels * iRow);
  for(int iChan = 0; iChan < iNumChannels; ++iChan)
	{
	  int iPeriod = ((ptr[0] & 0x0F) << 8) | ptr[1];
	  int iSample = (ptr[0] & 0xF0) | ((ptr[2] & 0xF0) >> 4);
	  int iEffect = ((ptr[2] & 0x0F) << 8) | ptr[3];
	  ptr += 4;

	  if(iPeriod != 0 || iSample != 0 || iEffect != 0)
		{
		  bEmptyRow = false;
		}

	  if((iEffect & 0xFF0) == 0xEE0) 
		{
		  iPatDelay = (iEffect & 0x00F);
		}
	}

  if(bEmptyRow)
	{
	  return 1;
	}

  return iPatDelay;
}