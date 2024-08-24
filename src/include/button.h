#ifndef BUTTON_H
#define BUTTON_H
    
#include <SDL2/SDL.h>
#include <set>
#include "vec2.h"

enum class ButtonTypesEnum{
    SELECT_BUTTON,
    TOGGLE_BUTTON,
    NUM_BUTTON_TYPES
};

class Button{
private:
    SDL_FRect clickRegion;
    bool state{false};
    bool isActive{true};    // bool storing whether button is currently responsive or disabled
    ButtonTypesEnum buttonType{ButtonTypesEnum::SELECT_BUTTON};
    static std::set<Button*> button_list;

public:
    Button(SDL_FRect clickRegion){
        button_list.insert(this);
        this->clickRegion = clickRegion;
    }

    Button(vec2 pos, double width = 10, double height = 10){
        button_list.insert(this);
        clickRegion.x = pos.x - width/2;
        clickRegion.y = pos.y - height/2;
        clickRegion.w = width;
        clickRegion.h = height;
    }

    Button(double pos_x = 0.0, double pos_y = 0.0, double width = 10.0, double height = 10.0){
        button_list.insert(this);
        clickRegion.x = pos_x - width/2;
        clickRegion.y = pos_y - height/2;
        clickRegion.w = width;
        clickRegion.h = height;
    }

    Button(int pos_x, int pos_y, double width = 10.0, double height = 10.0){
        button_list.insert(this);
        clickRegion.x = pos_x - width/2;
        clickRegion.y = pos_y - height/2;
        clickRegion.w = width;
        clickRegion.h = height;
    }

    Button(int pos_x, int pos_y, int width, int height){
        button_list.insert(this);
        clickRegion.x = pos_x - (double)width/2;
        clickRegion.y = pos_y - (double)height/2;
        clickRegion.w = (double)width;
        clickRegion.h = (double)height;
    }

    ~Button(){
        button_list.erase(this);
    }

    bool getState(){return state;}

    vec2 getPos(){return {clickRegion.x, clickRegion.y};}
    double getPosX(){return clickRegion.x;}
    double getPosY(){return clickRegion.y;}

    vec2 getSize(){return {clickRegion.w, clickRegion.h};}
    double getWidth(){return clickRegion.w;}
    double getHeight(){return clickRegion.h;}

    void setPos(vec2 pos){clickRegion.x = pos.x; clickRegion.y = pos.y;}
    void setWidth(double width){clickRegion.w = width;}
    void setHeight(double height){clickRegion.h = height;}
    void setWidth(int width){clickRegion.w = (double)width;}
    void setHeight(int height){clickRegion.h = (double)height;}

    void enable(){isActive = true;}
    void disable(){isActive = false;}

    void setState(bool state){this->state = state;}

    void setButtonType(ButtonTypesEnum button_type){buttonType = button_type;}

    void updateState(SDL_Event &event){
        if(!isActive) return;
        switch(buttonType){
            case ButtonTypesEnum::SELECT_BUTTON:
                if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
                    if(event.button.x >= clickRegion.x && event.button.x < clickRegion.x + clickRegion.w && event.button.y >= clickRegion.y && event.button.y < clickRegion.y +clickRegion.h) state = true;
                }
                break;
            
            case ButtonTypesEnum::TOGGLE_BUTTON:
                if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
                    if(event.button.x >= clickRegion.x && event.button.x < clickRegion.x + clickRegion.w && event.button.y >= clickRegion.y && event.button.y < clickRegion.y +clickRegion.h) state = !state;
                }
                break;

            default:
                break;
        }
             
    }

    static void updateAllStates(SDL_Event &event){
        for(auto &button: button_list) button->updateState(event);
    }

};

#endif