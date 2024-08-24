#ifndef SHAPE_H
#define SHAPE_H

#include <SDL2/SDL.h>
#include "vec2.h"
#include <deque>

class Shape{
protected:
    SDL_FRect boundBox;
    bool isFill = false;
    bool hasOutline = true;
    SDL_Color fillColor{255,255,255,255}, outlineColor{255,255,255,255};
    SDL_Texture* texture{nullptr};
    vec2 pos, vel, acc;

public:
    Shape(vec2 pos): pos(pos){}
    Shape(double pos_x = 0.0, double pos_y = 0.0): pos{pos_x, pos_y}{}
    Shape(int pos_x, int pos_y): pos{static_cast<double>(pos_x), static_cast<double>(pos_y)}{}

    ~Shape(){
        if(texture != nullptr){
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }

   inline vec2 getPos(){return pos;}
   inline vec2 getVel(){return vel;}
   inline vec2 getAcc(){return acc;}
   inline double getPosX(){return pos.x;}
   inline double getPosY(){return pos.y;}
   inline double getVelX(){return vel.x;}
   inline double getVelY(){return vel.y;}
   inline double getAccX(){return acc.x;}
   inline double getAccY(){return acc.y;}
   inline SDL_Color getFillColor(){return fillColor;}
   inline SDL_Color getOutlineColor(){return outlineColor;}
   inline bool isFilled(){return isFill;}
   inline bool isOutlined(){return hasOutline;}

    virtual void setPos(vec2 pos){this->pos = pos;}
    void setVel(vec2 vel){this->vel = vel;}
    void setAcc(vec2 acc){this->acc = acc;}
    virtual void setPos(double pos_x, double pos_y){pos.x = pos_x; pos.y = pos_y;}
    void setVel(double vel_x, double vel_y){vel.x = vel_x; vel.y = vel_y;}
    void setAcc(double acc_x, double acc_y){acc.x = acc_x; acc.y = acc_y;}
    void setFillColor(SDL_Color color){
        fillColor = color;
    }
    void setOutlineColor(SDL_Color color){outlineColor = color;}
    void setFillColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a){
        fillColor = {r,g,b,a};
    }
    void setOutlineColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a){outlineColor = {r,g,b,a};}
    void enableFill(){isFill = true;}
    void disableFill(){isFill = false;}
    void enableOutline(){hasOutline = true;}
    void disableOutline(){hasOutline = false;}
    virtual void update(){
        pos = pos+vel; vel = vel+acc;
    }
    virtual void draw(SDL_Renderer* renderer) = 0;
    void drawTrails(SDL_Renderer* renderer, std::deque<vec2> &trailsPos){
        auto prev_pos = pos;
        auto prev_fill_alpha = fillColor.a;
        auto prev_outline_alpha = outlineColor.a;
        auto fill_step = prev_fill_alpha/(2*(trailsPos.size()+1));
        auto outline_step = prev_outline_alpha/(2*(trailsPos.size()+1));
        auto fill_alpha = fill_step;
        auto outline_alpha = outline_step;
        
        for(auto i=0;i<trailsPos.size();++i){
            setPos(trailsPos[i]);
            fill_alpha += fill_step;
            outline_alpha += outline_step;
            setFillColor(fillColor.r,fillColor.g,fillColor.b,fill_alpha);
            setOutlineColor(outlineColor.r,outlineColor.g,outlineColor.b,outline_alpha);
            draw(renderer);
        }
        setPos(prev_pos);
        setFillColor(fillColor.r,fillColor.g,fillColor.b,prev_fill_alpha);
        setOutlineColor(outlineColor.r,outlineColor.g,outlineColor.b,prev_outline_alpha);
    }
    void setTexture(SDL_Texture* texture){
        if(this->texture != nullptr) SDL_DestroyTexture(this->texture);
        this->texture = texture;
        /* TODO copy texture somehow*/
    }
};

class Rect: public Shape{
public:
    Rect(vec2 pos, double width = 10, double height = 10): Shape(pos){boundBox.x = pos.x - width/2; boundBox.y = pos.y - height/2; boundBox.w = width; boundBox.h = height;}
    Rect(double pos_x = 0.0, double pos_y = 0.0, double width = 10.0, double height = 10.0): Shape(pos_x, pos_y){boundBox.x = pos_x - width/2; boundBox.y = pos_y - height/2; boundBox.w = width; boundBox.h = height;}
    Rect(int pos_x, int pos_y, double width = 10.0, double height = 10.0): Shape(pos_x, pos_y){boundBox.x = pos_x - width/2; boundBox.y = pos_y - height/2; boundBox.w = width; boundBox.h = height;}
    Rect(int pos_x, int pos_y, int width, int height): Shape(pos_x, pos_y){boundBox.x = pos_x - (static_cast<double>(width))/2; boundBox.y = pos_y - (static_cast<double>(height))/2; boundBox.w = static_cast<double>(width); boundBox.h = static_cast<double>(height);}

    double getWidth(){return boundBox.w;}
    double getHeight(){return boundBox.h;}

    void setPos(vec2 pos) override{this->pos = pos; boundBox.x = pos.x - boundBox.w/2; boundBox.y = pos.y - boundBox.h/2;}
    void setPos(double pos_x, double pos_y) override{pos.x = pos_x; pos.y = pos_y; boundBox.x = pos_x - boundBox.w/2; boundBox.y = pos_y - boundBox.h/2;}
    void setWidth(double width){boundBox.w = width;}
    void setHeight(double height){boundBox.h = height;}
    void setWidth(int width){boundBox.w = static_cast<double>(width);}
    void setHeight(int height){boundBox.h = static_cast<double>(height);}
    
    void update() override{
        vel.x += acc.x; vel.y += acc.y;
        pos.x += vel.x; pos.y += vel.y;
        boundBox.x = pos.x - boundBox.w/2; boundBox.y = pos.y - boundBox.h/2;
    }
    void draw(SDL_Renderer* renderer) override{
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
        if(texture != nullptr){
            SDL_SetTextureColorMod(texture,fillColor.r,fillColor.g,fillColor.b);
            SDL_SetTextureAlphaMod(texture,fillColor.a);
            SDL_RenderCopyF(renderer,texture,nullptr,&boundBox);
        }
        else{
            SDL_Color prev_color;
            SDL_GetRenderDrawColor(renderer, &prev_color.r, &prev_color.g, &prev_color.b, &prev_color.a);    // store previous draw color
            if(isFill){            
                SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
                SDL_RenderFillRectF(renderer, &boundBox);                      
            }
            if(hasOutline){
                SDL_SetRenderDrawColor(renderer, outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
                SDL_RenderDrawRectF(renderer, &boundBox);
            }
            SDL_SetRenderDrawColor(renderer, prev_color.r, prev_color.g, prev_color.b, prev_color.a);    // restore previous draw color
        }
    }

    static inline void drawRotatedFillRectangle(SDL_Renderer* renderer, SDL_FPoint center, double width, double height, double angle, SDL_Color fill_color){
        SDL_FRect rect;
        rect.x = center.x - width/2;
        rect.y = center.y - height/2;
        rect.w = width;
        rect.h = height;
        SDL_Texture* render_target = SDL_GetRenderTarget(renderer);
        SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawColor(renderer, fill_color.r, fill_color.g, fill_color.b, fill_color.a);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, render_target);
        SDL_RenderCopyExF(renderer, texture, nullptr, &rect, angle, nullptr, SDL_FLIP_NONE);
        return;
    }
};

class Ellipse: public Shape{
private:
    vec2 radius;
    static SDL_Texture* fillTexture;
    static SDL_Texture* outlineTexture;

    void updateBoundBox(){
        boundBox.x = pos.x - radius.x;
        boundBox.y = pos.y - radius.y;
        boundBox.w = 2*radius.x;
        boundBox.h = 2*radius.y;
    }    

public:
    Ellipse(vec2 pos, double radius_x = 5.0, double radius_y = 5.0): Shape(pos){setRadii(radius_x, radius_y);}
    Ellipse(double pos_x = 0.0, double pos_y = 0.0, double radius_x = 5.0, double radius_y = 5.0): Shape(pos_x, pos_y){setRadii(radius_x, radius_y);}
    Ellipse(int pos_x, int pos_y, double radius_x = 5.0, double radius_y = 5.0): Shape(pos_x, pos_y){setRadii(radius_x, radius_y);}
    Ellipse(vec2 pos, vec2 radius): Shape(pos){setRadii(radius);}
    Ellipse(double pos_x, double pos_y, vec2 radius): Shape(pos_x, pos_y){setRadii(radius);}
    Ellipse(int pos_x, int pos_y, vec2 radius): Shape(pos_x, pos_y){setRadii(radius);}

    static void initializeTextures(SDL_Texture* circle_solid_texture, SDL_Texture* circle_outline_texture){
        Ellipse::fillTexture = circle_solid_texture;
        Ellipse::outlineTexture = circle_outline_texture;
    }

    vec2 getRadii(){return radius;}
    double getRadiusX(){return radius.x;}
    double getRadiusY(){return radius.y;}

    void setPos(vec2 pos) override{
        this->pos = pos;
        updateBoundBox();   
    }
    void setPos(double pos_x, double pos_y) override{
        pos.x = pos_x; pos.y = pos_y;
        updateBoundBox();
    }
    void setRadii(vec2 radius){
        this->radius = radius;
        updateBoundBox();
    }
    void setRadii(double radius_x, double radius_y){
        this->radius = {radius_x, radius_y};
        updateBoundBox();
    }
    void setRadii(int radius_x, int radius_y){
        this->radius = {static_cast<double>(radius_x), static_cast<double>(radius_y)};
        updateBoundBox();
    }
    void setRadius(double radius){
        this->radius = {radius, radius};
        updateBoundBox();
    }
    void setRadius(int radius){
        this->radius = {static_cast<double>(radius), static_cast<double>(radius)};
        updateBoundBox();
    }

    void update() override{
        vel.x += acc.x; vel.y += acc.y;
        pos.x += vel.x; pos.y += vel.y;
        updateBoundBox();
    }

    static void drawEllipse(SDL_Renderer* renderer, SDL_Color outline_color, vec2 pos, vec2 radius){    /* fix it for ellipse class or remove if not needed */
        SDL_SetRenderDrawColor(renderer, outline_color.r, outline_color.g, outline_color.b, outline_color.a);
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
        for(double i=0;i<180;i+=50.0/std::max(radius.x, radius.y)){
            double angle = M_PI*i/180;
            double x = pos.x + radius.x*cos(angle);
            double y = pos.y + radius.y*sin(angle);
            SDL_RenderDrawPointF(renderer, x, y);
            SDL_RenderDrawPointF(renderer, 2*pos.x - x, 2*pos.y - y);
        }
    }

    static void drawEllipseSolid(SDL_Renderer* renderer, SDL_Colour fill_color, SDL_FRect boundBox){
        SDL_SetTextureColorMod(fillTexture,fill_color.r,fill_color.g,fill_color.b);
        SDL_SetTextureAlphaMod(fillTexture,fill_color.a);
        SDL_BlendMode prev_blendmode;
        SDL_GetRenderDrawBlendMode(renderer, &prev_blendmode);
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_MOD);
        SDL_RenderCopyF(renderer,fillTexture,nullptr,&boundBox);        
        SDL_SetRenderDrawBlendMode(renderer,prev_blendmode);
    }

    static void drawEllipseOutline(SDL_Renderer* renderer, SDL_Colour outline_color, SDL_FRect boundBox){
        SDL_SetTextureColorMod(outlineTexture,outline_color.r,outline_color.g,outline_color.b);
        SDL_SetTextureAlphaMod(outlineTexture,outline_color.a);
        SDL_BlendMode prev_blendmode;
        SDL_GetRenderDrawBlendMode(renderer, &prev_blendmode);
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_MOD);
        SDL_RenderCopyF(renderer,outlineTexture,nullptr,&boundBox);        
        SDL_SetRenderDrawBlendMode(renderer,prev_blendmode);
    }

    void draw(SDL_Renderer* renderer) override{
        if(isFill) drawEllipseSolid(renderer, fillColor, boundBox);
        if(hasOutline) drawEllipse(renderer, outlineColor, pos, radius);
    }
};

#endif