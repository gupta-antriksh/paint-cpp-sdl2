#include "button.h"

Button::Button(SDL_FRect clickRegion){
    button_list.insert(this);
    this->clickRegion = clickRegion;
}

Button::Button(vec2 pos, double width, double height){
    button_list.insert(this);
    clickRegion.x = pos.x - width/2;
    clickRegion.y = pos.y - height/2;
    clickRegion.w = width;
    clickRegion.h = height;
}

Button::Button(double pos_x, double pos_y, double width, double height){
    button_list.insert(this);
    clickRegion.x = pos_x - width/2;
    clickRegion.y = pos_y - height/2;
    clickRegion.w = width;
    clickRegion.h = height;
}

Button::Button(int pos_x, int pos_y, double width, double height){
    button_list.insert(this);
    clickRegion.x = pos_x - width/2;
    clickRegion.y = pos_y - height/2;
    clickRegion.w = width;
    clickRegion.h = height;
}

Button::Button(int pos_x, int pos_y, int width, int height){
    button_list.insert(this);
    clickRegion.x = pos_x - (double)width/2;
    clickRegion.y = pos_y - (double)height/2;
    clickRegion.w = (double)width;
    clickRegion.h = (double)height;
}

Button::~Button(){
    button_list.erase(this);
}

bool Button::getState(){return clickState;}

vec2 Button::getPos(){return {clickRegion.x, clickRegion.y};}
double Button::getPosX(){return clickRegion.x;}
double Button::getPosY(){return clickRegion.y;}

vec2 Button::getSize(){return {clickRegion.w, clickRegion.h};}
double Button::getWidth(){return clickRegion.w;}
double Button::getHeight(){return clickRegion.h;}

// SDL_FRect Button::&getClickRegionRect(){return clickRegion;}


void Button::setPos(vec2 pos){clickRegion.x = pos.x; clickRegion.y = pos.y;}
void Button::setWidth(double width){clickRegion.w = width;}
void Button::setHeight(double height){clickRegion.h = height;}
void Button::setWidth(int width){clickRegion.w = (double)width;}
void Button::setHeight(int height){clickRegion.h = (double)height;}

void Button::enable(){isActive = true;}
void Button::disable(){isActive = false;}

void Button::updateState(SDL_Event &event){
    if(!isActive) return;
    if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
        if(event.button.x >= clickRegion.x && event.button.x < clickRegion.x + clickRegion.w && event.button.y >= clickRegion.y && event.button.y < clickRegion.y +clickRegion.h) clickState = true;
    }     
}

void Button::updateAllStates(SDL_Event &event){
    if(event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return;
    for(auto &button: button_list) button->updateState(event);
}

void Button::resetState(){clickState = false;}

set<Button*> Button::button_list;

int main(int argc, char** ) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Quit();

    return 0;
}
