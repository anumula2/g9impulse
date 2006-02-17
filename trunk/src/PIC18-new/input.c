#include <system.h>
#include "input.h"

#define INPUT_UP_BUTTON 3
#define INPUT_LEFT_BUTTON 1
#define INPUT_DOWN_BUTTON 2
#define INPUT_RIGHT_BUTTON 0
#define INPUT_A_BUTTON 7
#define INPUT_B_BUTTON 6
#define INPUT_START_BUTTON 4
#define INPUT_SELECT_BUTTON 5

#define INPUT_PRESSED_EVENT_FLAG 0b10000000
#define INPUT_KEY_MASK 0b01111111

InputEvent theInputBuffer;

bool theNewInputFlag = false;

void interrupt()
{
	char tempEvent;
	char key;
	bool pressed;
	
	if (pir1 & 0b00100000)
	{
    	tempEvent = rcreg;
   	
       	key = tempEvent & INPUT_KEY_MASK;
       	pressed = tempEvent & INPUT_PRESSED_EVENT_FLAG;
       	
       	switch (key)
       	{
       	    case INPUT_A_BUTTON:
                theInputBuffer.buttonAPressed = pressed;
                break;
            
            case INPUT_B_BUTTON:
                theInputBuffer.buttonBPressed = pressed;
                break;
            
            case INPUT_START_BUTTON:
                theInputBuffer.startPressed = pressed;
                break;
                
            case INPUT_SELECT_BUTTON:
                theInputBuffer.selectPressed = pressed;
                break;
                
            case INPUT_UP_BUTTON:
                theInputBuffer.upPressed = pressed;
                break;
            
            case INPUT_DOWN_BUTTON:
                theInputBuffer.downPressed = pressed;
                break;
                
            case INPUT_LEFT_BUTTON:
                theInputBuffer.leftPressed = pressed;
                break;
                
            case INPUT_RIGHT_BUTTON:
                theInputBuffer.rightPressed = pressed;
                break;
        }
            
        theInputBuffer.anyInput = theInputBuffer.anyInput || pressed;
    	
		theNewInputFlag = true;
	}
}

InputEvent* getInputEvent()
{
	if (theNewInputFlag)
	{
    	theNewInputFlag = false;
    }
    else
    {
        theInputBuffer.buttonAPressed   = false;
        theInputBuffer.buttonBPressed   = false;
        theInputBuffer.startPressed     = false;
        theInputBuffer.selectPressed    = false;
        theInputBuffer.upPressed        = false;
        theInputBuffer.downPressed      = false;
        theInputBuffer.leftPressed      = false;
        theInputBuffer.rightPressed     = false;
            
        theInputBuffer.anyInput = false;
    }

	return &theInputBuffer;
}	

//initialize serial port for continuous receive
void serialInit()
{
	set_bit(trisc, 7);
	set_bit(trisc, 6);
	spbrg = 80;
	clear_bit(txsta, BRGH); 
	clear_bit(baudcon, BRG16);
	clear_bit(txsta, SYNC);
	set_bit(rcsta, SPEN);
	set_bit(txsta, TXEN);
	set_bit(rcsta, CREN);
	//enable interrupts
	set_bit(pie1, RCIE);
	set_bit(intcon, PEIE);
	set_bit(intcon, GIE);
}