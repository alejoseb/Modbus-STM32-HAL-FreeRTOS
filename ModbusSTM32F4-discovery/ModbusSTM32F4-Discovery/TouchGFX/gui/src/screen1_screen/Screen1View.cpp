#include <gui/screen1_screen/Screen1View.hpp>
#include "string.h"
Screen1View::Screen1View()
{

}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::functionLedToggle()
{
	presenter->LedToggleRequested(ToggleLed.getState()); //tell the presenter to update the model with the state of toggleLed

}


void Screen1View::functionBtDown()
{
	presenter->RegisterUpDown(-1);
}


void Screen1View::functionBtUp()
{
	presenter->RegisterUpDown(1);
}

void Screen1View::registerUpdate(int value)
{
	memset(&textArea1Buffer,0, TEXTAREA1_SIZE);
	Unicode::snprintf(textArea1Buffer, sizeof(textArea1Buffer), "%d",value);
	textArea1.invalidate();

}
