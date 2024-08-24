#include "shape.h"

Shape::Shape(vec2 pos): pos(pos){}
Shape::Shape(double pos_x, double pos_y): pos{pos_x, pos_y}{}
Shape::Shape(int pos_x, int pos_y): pos{(double)pos_x, (double)pos_y}{}

Shape::~Shape(){
    if(texture != nullptr){
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

vec2 Shape::getPos(){return pos;}
vec2 Shape::getVel(){return vel;}
vec2 Shape::getAcc(){return acc;}
double Shape::getPosX(){return pos.x;}
double Shape::getPosY(){return pos.y;}
double Shape::getVelX(){return vel.x;}
double Shape::getVelY(){return vel.y;}
double Shape::getAccX(){return acc.x;}
double Shape::getAccY(){return acc.y;}
SDL_Color Shape::getFillColor(){return fillColor;}
SDL_Color Shape::getOutlineColor(){return outlineColor;}
bool Shape::isFilled(){return isFill;}
bool Shape::isOutlined(){return hasOutline;}

void Shape::setPos(vec2 pos){this->pos = pos;}
void Shape::setVel(vec2 vel){this->vel = vel;}
void Shape::setAcc(vec2 acc){this->acc = acc;}
void Shape::setPos(double pos_x, double pos_y){pos.x = pos_x; pos.y = pos_y;}
void Shape::setVel(double vel_x, double vel_y){vel.x = vel_x; vel.y = vel_y;}
void Shape::setAcc(double acc_x, double acc_y){acc.x = acc_x; acc.y = acc_y;}
void Shape::setFillColor(SDL_Color color){
    fillColor = color;
    // if(texture != nullptr){    /* TODO: is it required */
    //     SDL_SetTextureColorMod(texture,color.r,color.g,color.b);
    //     SDL_SetTextureAlphaMod(texture,color.a);
    // }    
}
void Shape::setOutlineColor(SDL_Color color){outlineColor = color;}
void Shape::setFillColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a){
    fillColor = {r,g,b,a};
    // if(texture != nullptr){    /* TODO: is it required */
    //     SDL_SetTextureColorMod(texture,r,g,b);
    //     SDL_SetTextureAlphaMod(texture,a);
    // }     
}
void Shape::setOutlineColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a){outlineColor = {r,g,b,a};}
void Shape::enableFill(){isFill = true;}
void Shape::disableFill(){isFill = false;}
void Shape::enableOutline(){hasOutline = true;}
void Shape::disableOutline(){hasOutline = false;}
void Shape::update(){
    pos = pos+vel; vel = vel+acc;
}
// virtual void Shape::draw(SDL_Renderer* renderer) = 0;
void Shape::drawTrails(SDL_Renderer* renderer, deque<vec2> &trailsPos){
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
void Shape::setTexture(SDL_Texture* texture){
    if(this->texture != nullptr) SDL_DestroyTexture(this->texture);
    this->texture = texture;
    /* TODO copy texture somehow*/
}


Rect::Rect(vec2 pos, double width, double height): Shape(pos){boundBox.x = pos.x - width/2; boundBox.y = pos.y - height/2; boundBox.w = width; boundBox.h = height;}
Rect::Rect(double pos_x, double pos_y, double width, double height): Shape(pos_x, pos_y){boundBox.x = pos_x - width/2; boundBox.y = pos_y - height/2; boundBox.w = width; boundBox.h = height;}
Rect::Rect(int pos_x, int pos_y, double width, double height): Shape(pos_x, pos_y){boundBox.x = pos_x - width/2; boundBox.y = pos_y - height/2; boundBox.w = width; boundBox.h = height;}
Rect::Rect(int pos_x, int pos_y, int width, int height): Shape(pos_x, pos_y){boundBox.x = pos_x - (double)width/2; boundBox.y = pos_y - (double)height/2; boundBox.w = (double)width; boundBox.h = (double)height;}

double Rect::getWidth(){return boundBox.w;}
double Rect::getHeight(){return boundBox.h;}

void Rect::setPos(vec2 pos){this->pos = pos; boundBox.x = pos.x - boundBox.w/2; boundBox.y = pos.y - boundBox.h/2;}
void Rect::setPos(double pos_x, double pos_y){pos.x = pos_x; pos.y = pos_y; boundBox.x = pos_x - boundBox.w/2; boundBox.y = pos_y - boundBox.h/2;}
void Rect::setWidth(double width){boundBox.w = width;}
void Rect::setHeight(double height){boundBox.h = height;}
void Rect::setWidth(int width){boundBox.w = (double)width;}
void Rect::setHeight(int height){boundBox.h = (double)height;}

void Rect::update(){
    /* TODO: fix this */
    // if(pos.x + boundBox.w/2 + vel.x > SCREEN_WIDTH || pos.x - boundBox.w/2 + vel.x < 0) vel.x = -vel.x;
    // else vel.x += acc.x;
    // if(pos.y + boundBox.h/2 + vel.y > SCREEN_HEIGHT || pos.y - boundBox.h/2 + vel.y < 0) vel.y = -vel.y;
    // else vel.y += acc.y;
    vel.x += acc.x; vel.y += acc.y;
    pos.x += vel.x; pos.y += vel.y;
    boundBox.x = pos.x - boundBox.w/2; boundBox.y = pos.y - boundBox.h/2;
}
void Rect::draw(SDL_Renderer* renderer){
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

void Rect::drawRotatedFillRectangle(SDL_Renderer* renderer, SDL_FPoint center, double width, double height, double angle, SDL_Color fill_color){
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


void Ellipse::updateBoundBox(){
    boundBox.x = pos.x - radius.x;
    boundBox.y = pos.y - radius.y;
    boundBox.w = 2*radius.x;
    boundBox.h = 2*radius.y;
}

// void Ellipse::createFillTexture(){    /* TODO: check and fix or eliminate if redundant */
//     if(texture != nullptr){
//         SDL_DestroyTexture(texture);
//         texture = nullptr;
//     }
//     auto texture_width = 2*CIRCLE_TEX_RADIUS, texture_height = 2*CIRCLE_TEX_RADIUS;
//     auto pixelsCount = (texture_width)*(texture_height);
//     auto pixels = new Uint32[pixelsCount];
    
//     auto color = SDL_MapRGBA(SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888), 255, 255, 255, 255);
//     auto transparent_mask = SDL_MapRGBA(SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888), 0, 0, 0, 0);
//     for(auto i = 0; i < pixelsCount; ++i) {
//         pixels[i] = color;
//     }
//     for(auto x = -CIRCLE_TEX_RADIUS; x <= -1; ++x){
//         for(auto y = -CIRCLE_TEX_RADIUS; y <= -1; ++y){
//             if(x*x + y*y <= CIRCLE_TEX_RADIUS*CIRCLE_TEX_RADIUS) break;
//             int i = x+CIRCLE_TEX_RADIUS, j = y+CIRCLE_TEX_RADIUS;
//             pixels[(i*(texture_width)) + j] = transparent_mask;
//             pixels[(i*(texture_width)) + (texture_height-1-j)] = transparent_mask;
//             pixels[((texture_width-1-i)*(texture_width)) + (j)] = transparent_mask;
//             pixels[((texture_width-1-i)*(texture_width)) + (texture_height-1-j)] = transparent_mask;               
//         }
//     }
//     texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, texture_width, texture_height);
//     SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
//     SDL_UpdateTexture(texture, nullptr, pixels, texture_width*sizeof(Uint32));
//     delete[] pixels;
// }

Ellipse::Ellipse(vec2 pos, double radius_x, double radius_y): Shape(pos){setRadii(radius_x, radius_y);}
Ellipse::Ellipse(double pos_x, double pos_y, double radius_x, double radius_y): Shape(pos_x, pos_y){setRadii(radius_x, radius_y);}
Ellipse::Ellipse(int pos_x, int pos_y, double radius_x, double radius_y): Shape(pos_x, pos_y){setRadii(radius_x, radius_y);}
Ellipse::Ellipse(vec2 pos, vec2 radius): Shape(pos){setRadii(radius);}
Ellipse::Ellipse(double pos_x, double pos_y, vec2 radius): Shape(pos_x, pos_y){setRadii(radius);}
Ellipse::Ellipse(int pos_x, int pos_y, vec2 radius): Shape(pos_x, pos_y){setRadii(radius);}

void Ellipse::initializeTextures(SDL_Texture* circle_solid_texture, SDL_Texture* circle_outline_texture){
    Ellipse::fillTexture = circle_solid_texture;
    Ellipse::outlineTexture = circle_outline_texture;
}

vec2 Ellipse::getRadii(){return radius;}
double Ellipse::getRadiusX(){return radius.x;}
double Ellipse::getRadiusY(){return radius.y;}

void Ellipse::setPos(vec2 pos){
    this->pos = pos;
    updateBoundBox();   
}
void Ellipse::setPos(double pos_x, double pos_y){
    pos.x = pos_x; pos.y = pos_y;
    updateBoundBox();
}
void Ellipse::setRadii(vec2 radius){
    this->radius = radius;
    updateBoundBox();
}
void Ellipse::setRadii(double radius_x, double radius_y){
    this->radius = {radius_x, radius_y};
    updateBoundBox();
}
void Ellipse::setRadii(int radius_x, int radius_y){
    this->radius = {(double)radius_x, (double)radius_y};
    updateBoundBox();
}
void Ellipse::setRadius(double radius){
    this->radius = {radius, radius};
    updateBoundBox();
}
void Ellipse::setRadius(int radius){
    this->radius = {(double)radius, (double)radius};
    updateBoundBox();
}

void Ellipse::update(){
    /* TODO: fix this */
    // if(pos.x + radius.x + vel.x > SCREEN_WIDTH || pos.x - radius.x + vel.x < 0){
    //     vel.x = -vel.x;
    //     // if (Mix_PlayChannel(-1, collide_sound, 0) == -1) {
    //     //     cerr << "Mix_PlayChannel: " <<  Mix_GetError() << endl;
    //     // }
    // }
    // else vel.x += acc.x;
    // if(pos.y + radius.y + vel.y > SCREEN_HEIGHT || pos.y - radius.y + vel.y < 0){
    //     vel.y = -vel.y;
    //     // if (Mix_PlayChannel(-1, collide_sound, 0) == -1) {
    //     //     cerr << "Mix_PlayChannel: " <<  Mix_GetError() << endl;
    //     // }
    // }   
    // else vel.y += acc.y;
    vel.x += acc.x; vel.y += acc.y;
    pos.x += vel.x; pos.y += vel.y;
    updateBoundBox();
}

/* TODO: fix this function */
// void Ellipse::updateConstrained(vec2 center, double len){
//     vel.x += acc.x; vel.y += acc.y;
//     if(distance(center,pos) <= len && distance(center,pos+vel) > len){
//         double frac = solveQuadratic(sqnorm(vel),2*dot(vel,pos-center),sqnorm(pos-center)-len*len).second;
//         if(frac == INFINITY || frac < 0) return;
//         pos.x += frac*vel.x; pos.y += frac*vel.y;
//         vel = vel - (dot(vel,pos-center)*(pos-center))/sqnorm(pos-center);
//         vel = vel - (dot(vel,pos-center)*(pos-center))/sqnorm(pos-center);
//         vel = vel - (dot(vel,pos-center)*(pos-center))/sqnorm(pos-center);
//     }
//     else pos.x += vel.x; pos.y += vel.y;
//     updateBoundBox();
// }

void Ellipse::drawEllipse(SDL_Renderer* renderer, SDL_Color outline_color, vec2 pos, vec2 radius){    /* fix it for ellipse class or remove if not needed */
    SDL_SetRenderDrawColor(renderer, outline_color.r, outline_color.g, outline_color.b, outline_color.a);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    for(double i=0;i<180;i+=50.0/max(radius.x, radius.y)){
        double angle = M_PI*i/180;
        double x = pos.x + radius.x*cos(angle);
        double y = pos.y + radius.y*sin(angle);
        SDL_RenderDrawPointF(renderer, x, y);
        SDL_RenderDrawPointF(renderer, 2*pos.x - x, 2*pos.y - y);
    }
}

void Ellipse::drawEllipseSolid(SDL_Renderer* renderer, SDL_Colour fill_color, SDL_FRect boundBox){
    // if(fillTexture == nullptr) createFillTexture();     /*TODO*/
    SDL_SetTextureColorMod(fillTexture,fill_color.r,fill_color.g,fill_color.b);
    SDL_SetTextureAlphaMod(fillTexture,fill_color.a);
    SDL_BlendMode prev_blendmode;
    SDL_GetRenderDrawBlendMode(renderer, &prev_blendmode);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_MOD);
    SDL_RenderCopyF(renderer,fillTexture,nullptr,&boundBox);        
    SDL_SetRenderDrawBlendMode(renderer,prev_blendmode);
}

void Ellipse::drawEllipseOutline(SDL_Renderer* renderer, SDL_Colour outline_color, SDL_FRect boundBox){
    // if(texture == nullptr) createOutlineTexture();     /*TODO*/
    SDL_SetTextureColorMod(outlineTexture,outline_color.r,outline_color.g,outline_color.b);
    SDL_SetTextureAlphaMod(outlineTexture,outline_color.a);
    SDL_BlendMode prev_blendmode;
    SDL_GetRenderDrawBlendMode(renderer, &prev_blendmode);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_MOD);
    SDL_RenderCopyF(renderer,outlineTexture,nullptr,&boundBox);        
    SDL_SetRenderDrawBlendMode(renderer,prev_blendmode);
}

void Ellipse::draw(SDL_Renderer* renderer){
    if(isFill) drawEllipseSolid(renderer, fillColor, boundBox);
    if(hasOutline) drawEllipse(renderer, outlineColor, pos, radius);
    // if(hasOutline) drawEllipseOutline(renderer);
}

SDL_Texture* Ellipse::fillTexture{nullptr};
SDL_Texture* Ellipse::outlineTexture{nullptr};

int main(int argc, char** argv){

    return 0;
}