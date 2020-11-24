#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

Screen1Presenter::Screen1Presenter(Screen1View& v)
    : view(v)
{

}

void Screen1Presenter::activate()
{

}

void Screen1Presenter::deactivate()
{

}

void Screen1Presenter::LedToggleRequested(bool value)
{
	model->LedToggleRequested(value);
}

void Screen1Presenter::RegisterUpDown(int value)
{
	model->RegisterUpDown(value);
}


void Screen1Presenter::registerUpdate(int value)
{
  view.registerUpdate(value);
}
