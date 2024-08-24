#include <iostream>
#include <vector>
#include <queue>
#include <deque>
#include <string>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "vec2.h"
#include "shape.h"
#include "button.h"
#include "tinyfiledialogs.h"
using namespace std;
 
const int SCREEN_WIDTH = 1280; 
const int SCREEN_HEIGHT = 720;
const char* WINDOW_TITLE = "Paint using C++ and SDL2";
const int TARGET_FPS = 240;
const int FRAME_DELAY_MS = (1000/TARGET_FPS);
const double ERASER_SIDE_LEN = 15;
const double g = 0.5;

enum class ColorsEnum: int{
    BLACK,
    GREY,
    DARK_RED,
    RED,
    ORANGE,
    YELLOW,
    GREEN,
    TURQUOISE,
    INDIGO,
    PURPLE,
    WHITE,
    LIGHT_GREY,
    BROWN,
    ROSE,
    GOLD,
    LIGHT_YELLOW,
    LIME,
    LIGHT_TURQUOISE,
    BLUE_GREY,
    LAVENDER,
    NUM_COLORS
};

enum class ToolsEnum: int{
    ERASER,
    BUCKETFILL,
    LINE,
    SCRIBBLE,
    RECT,
    ELLIPSE,
    FILL_COLOR_SELECTOR,
    OUTLINE_COLOR_SELECTOR,
    FILL_TOGGLER,
    OUTLINE_TOGGLER,
    TRANSPARENCY_TOGGLER,
    NUM_TOOLS
};

typedef struct Buttons{
    vector<Button*> tool_buttons;
    vector<Button*> color_buttons;
} Buttons;

typedef struct Key{
    bool shift_pressed{false};    // shift for line snapping, squares and circles
    bool ctrl_pressed{false};
} Key;

typedef struct Mouse{
    vec2 initial_pos;
    vec2 curr_pos;
    // vec2 modified_pos;
} Mouse;

typedef struct Color{
    SDL_Color fill_color;
    SDL_Color outline_color;
    bool is_filled{true};
    bool is_outlined{true};
    bool is_transparent{false};
    bool is_fill_color_selected{true};    // false implies outline_color is selected
    vector<SDL_Color> colors;
} Color;

typedef struct Texture{
    SDL_Texture* canvas_texture{nullptr};
    SDL_Texture* canvas_overlay_texture{nullptr};
    SDL_Texture* toolbox_texture{nullptr};
    SDL_Texture* toolbox_overlay_texture{nullptr};
} Texture;

typedef struct Object{
    Rect draw_rect;
    Ellipse draw_ellipse;
    SDL_FRect eraser_rect;
} Object;

typedef struct History{
    deque<SDL_Texture*> draw_history;
    int curr_history_idx{0};
    int max_valid_history_idx{0};
} History;

typedef struct Cursor{
    SDL_Cursor* draw_cursor{nullptr};
} Cursor;

typedef struct Context{
    Buttons buttons;
    Key key;
    Mouse mouse;
    Color color;
    Texture texture;
    Object object;
    History history;
    Cursor cursor;
    bool is_drawing{false};
    ToolsEnum selected_tool = ToolsEnum::LINE;
    char filename[256] = "";
} Context;

SDL_Window* window{nullptr};
SDL_Renderer* renderer{nullptr};

SDL_Texture* ellipse_solid_texture;
SDL_Texture* ellipse_outline_texture;

pair<double,double> solveQuadratic(double a, double b, double c){
    double D = b*b - 4*a*c;
    if(D < 0) return {INFINITY,INFINITY};
    return {(-b-sqrt(D))/(2*a), (-b+sqrt(D))/(2*a)};
}

SDL_Texture* copyTexture(SDL_Texture* texture, SDL_Renderer* renderer){
    int texture_width, texture_height;
    Uint32 format;
    SDL_BlendMode texture_blend_mode;
    Uint8 texture_alpha_mod;
    if(SDL_QueryTexture(texture, &format, nullptr, &texture_width, &texture_height) < 0) return nullptr;
    SDL_GetTextureBlendMode(texture, &texture_blend_mode);
    SDL_GetTextureAlphaMod(texture, &texture_alpha_mod);
    SDL_Texture* newTexture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_TARGET, texture_width, texture_height);
    SDL_SetTextureBlendMode(newTexture, texture_blend_mode);
    SDL_SetTextureAlphaMod(newTexture, texture_alpha_mod);
    SDL_Texture* prev_rendering_target = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, newTexture);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_SetRenderTarget(renderer, prev_rendering_target);
    return newTexture;    
}

// initializing static variables
SDL_Texture* Ellipse::fillTexture{nullptr};
SDL_Texture* Ellipse::outlineTexture{nullptr};
set<Button*> Button::button_list{};

bool init(){
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) return false;

    window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if (window == nullptr) return false;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr){
        SDL_DestroyWindow(window);
        window = nullptr;
        return false;
    }

    return true;
}

void initEllipseTextures(){
    SDL_Surface* circle_solid_surface = IMG_Load("textures/circle_solid.bmp");    
    ellipse_solid_texture = SDL_CreateTextureFromSurface(renderer,circle_solid_surface); 
    SDL_FreeSurface(circle_solid_surface);
    circle_solid_surface = nullptr;

    SDL_Surface* circle_outline_surface = IMG_Load("textures/circle_outline5.bmp");
    ellipse_outline_texture = SDL_CreateTextureFromSurface(renderer,circle_outline_surface);
    SDL_FreeSurface(circle_outline_surface);
    circle_outline_surface = nullptr;

    Ellipse::initializeTextures(ellipse_solid_texture, ellipse_outline_texture);
}

void initializeColors(vector<SDL_Color> &colors){
    colors.resize(static_cast<int>(ColorsEnum::NUM_COLORS));
    colors[static_cast<int>(ColorsEnum::BLACK)] = {0, 0, 0, 255};
    colors[static_cast<int>(ColorsEnum::GREY)] = {127, 127, 127, 255};
    colors[static_cast<int>(ColorsEnum::DARK_RED)] = {136, 0, 21, 255};
    colors[static_cast<int>(ColorsEnum::RED)] = {237, 28, 36, 255};
    colors[static_cast<int>(ColorsEnum::ORANGE)] = {255, 127, 39, 255};
    colors[static_cast<int>(ColorsEnum::YELLOW)] = {255, 242, 0, 255};
    colors[static_cast<int>(ColorsEnum::GREEN)] = {34, 177, 76, 255};
    colors[static_cast<int>(ColorsEnum::TURQUOISE)] = {0, 162, 232, 255};
    colors[static_cast<int>(ColorsEnum::INDIGO)] = {63, 72, 204, 255};
    colors[static_cast<int>(ColorsEnum::PURPLE)] = {163, 73, 164, 255};
    colors[static_cast<int>(ColorsEnum::WHITE)] = {255, 255, 255, 255};
    colors[static_cast<int>(ColorsEnum::LIGHT_GREY)] = {195, 195, 195, 255};
    colors[static_cast<int>(ColorsEnum::BROWN)] = {185, 122, 87, 255};
    colors[static_cast<int>(ColorsEnum::ROSE)] = {255, 174, 201, 255};
    colors[static_cast<int>(ColorsEnum::GOLD)] = {255, 201, 14, 255};
    colors[static_cast<int>(ColorsEnum::LIGHT_YELLOW)] = {239, 228, 176, 255};
    colors[static_cast<int>(ColorsEnum::LIME)] = {181, 230, 29, 255};
    colors[static_cast<int>(ColorsEnum::LIGHT_TURQUOISE)] = {153, 217, 234, 255};
    colors[static_cast<int>(ColorsEnum::BLUE_GREY)] = {112, 146, 190, 255};
    colors[static_cast<int>(ColorsEnum::LAVENDER)] = {200, 191, 231, 255};
    return;
}

void initializeToolboxAndButtons(Context &context, SDL_Renderer* renderer, SDL_FRect &toolbox_bounds_rect){
    toolbox_bounds_rect.x = toolbox_bounds_rect.y = 0;
    toolbox_bounds_rect.w = SCREEN_WIDTH;
    toolbox_bounds_rect.h = SCREEN_WIDTH/10;
    context.texture.toolbox_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, toolbox_bounds_rect.w, toolbox_bounds_rect.h);
    context.texture.toolbox_overlay_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, toolbox_bounds_rect.w, toolbox_bounds_rect.h);
    SDL_SetTextureBlendMode(context.texture.toolbox_texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(context.texture.toolbox_overlay_texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, context.texture.toolbox_texture);
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderClear(renderer);

    // initialize and draw color buttons
    double color_square_side_len = 20.0;
    double color_square_gap_x = 7.0;
    double color_square_gap_y = 7.0;
    double color_square_offset_x = 3*toolbox_bounds_rect.w/5;
    double color_square_offset_y = 40.0;
    SDL_FRect color_rect;
    color_rect.x = toolbox_bounds_rect.x + color_square_offset_x;
    color_rect.y = toolbox_bounds_rect.y + color_square_offset_y;
    color_rect.w = color_rect.h = color_square_side_len;
    for(int i=0; i < static_cast<int>(ColorsEnum::NUM_COLORS); ++i){
        if(i == static_cast<int>(ColorsEnum::NUM_COLORS)/2){
            color_rect.x = toolbox_bounds_rect.x + color_square_offset_x;
            color_rect.y += color_rect.h + color_square_gap_y;
        }        
        SDL_SetRenderDrawColor(renderer, context.color.colors[i].r, context.color.colors[i].g, context.color.colors[i].b, context.color.colors[i].a);
        SDL_RenderFillRectF(renderer, &color_rect);
        SDL_SetRenderDrawColor(renderer, context.color.colors[static_cast<int>(ColorsEnum::WHITE)].r, context.color.colors[static_cast<int>(ColorsEnum::WHITE)].g, context.color.colors[static_cast<int>(ColorsEnum::WHITE)].b, context.color.colors[static_cast<int>(ColorsEnum::WHITE)].a);
        SDL_RenderDrawRectF(renderer, &color_rect);    
        context.buttons.color_buttons[i] = new Button(color_rect);
        color_rect.x += color_rect.w + color_square_gap_x;
    }

    // initialize and draw tool buttons
    double tool_square_side_len = 35.0;
    double tool_square_gap_x = 7.0;
    double tool_square_gap_y = 7.0;
    double tool_square_offset_x = toolbox_bounds_rect.w/5;
    double tool_square_offset_y = 50.0;
    double tool_square_padding = 5.0;
    SDL_FRect tool_outline_rect, tool_fill_rect;
    tool_outline_rect.x = toolbox_bounds_rect.x + tool_square_offset_x;
    tool_outline_rect.y = toolbox_bounds_rect.y + tool_square_offset_y;
    tool_outline_rect.w = tool_outline_rect.h = tool_square_side_len;
    tool_fill_rect.x = tool_outline_rect.x + tool_square_padding;
    tool_fill_rect.y = tool_outline_rect.y + tool_square_padding;
    tool_fill_rect.w = tool_outline_rect.w - 2*tool_square_padding;
    tool_fill_rect.h = tool_outline_rect.h - 2*tool_square_padding;
    
    for(int i=0; i < static_cast<int>(ToolsEnum::FILL_COLOR_SELECTOR); ++i){
        // if(i == static_cast<int>(ToolsEnum::NUM_TOOLS)/2){
        //     temp_rect.x = toolbox_bounds_rect.x + tool_square_offset_x;
        //     temp_rect.y += temp_rect.h + tool_square_gap_y;
        // }
        SDL_Surface* surface;

        if(i == static_cast<int>(ToolsEnum::ERASER)) surface = IMG_Load("textures/eraser.bmp");
        else if(i == static_cast<int>(ToolsEnum::BUCKETFILL)) surface = IMG_Load("textures/bucketfill.bmp");
        else if(i == static_cast<int>(ToolsEnum::LINE)) surface = IMG_Load("textures/line.bmp");
        else if(i == static_cast<int>(ToolsEnum::SCRIBBLE)) surface = IMG_Load("textures/scribble.bmp");
        else if(i == static_cast<int>(ToolsEnum::RECT)) surface = IMG_Load("textures/rectangle.bmp");
        else if(i == static_cast<int>(ToolsEnum::ELLIPSE)) surface = IMG_Load("textures/ellipse.bmp");

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_RenderCopyF(renderer, texture, nullptr, &tool_fill_rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
        context.buttons.tool_buttons[i] = new Button(tool_outline_rect);
        tool_outline_rect.x += tool_outline_rect.w + tool_square_gap_x;
        tool_fill_rect.x += tool_outline_rect.w + tool_square_gap_x;
        if(i == static_cast<int>(ToolsEnum::BUCKETFILL)){
            tool_outline_rect.x += 50;
            tool_fill_rect.x += 50;
        }
    }

    tool_square_gap_y = 7.0;
    tool_square_offset_x = toolbox_bounds_rect.w/2 + 70;
    tool_square_offset_y = 25.0;
    tool_square_padding = 5.0;
    tool_outline_rect.x = toolbox_bounds_rect.x + tool_square_offset_x;
    tool_outline_rect.y = toolbox_bounds_rect.y + tool_square_offset_y;
    tool_outline_rect.w = tool_outline_rect.h = tool_square_side_len;
    tool_fill_rect.x = tool_outline_rect.x + tool_square_padding;
    tool_fill_rect.y = tool_outline_rect.y + tool_square_padding;
    tool_fill_rect.w = tool_outline_rect.w - 2*tool_square_padding;
    tool_fill_rect.h = tool_outline_rect.h - 2*tool_square_padding;
    for(int i = static_cast<int>(ToolsEnum::FILL_COLOR_SELECTOR); i <= static_cast<int>(ToolsEnum::OUTLINE_COLOR_SELECTOR); ++i){
        SDL_SetRenderDrawColor(renderer, context.color.colors[static_cast<int>(ColorsEnum::WHITE)].r, context.color.colors[static_cast<int>(ColorsEnum::WHITE)].g, context.color.colors[static_cast<int>(ColorsEnum::WHITE)].b, context.color.colors[static_cast<int>(ColorsEnum::WHITE)].a);
        SDL_RenderFillRectF(renderer, &tool_fill_rect);
        context.buttons.tool_buttons[i] = new Button(tool_outline_rect);
        tool_outline_rect.y += tool_outline_rect.h + tool_square_gap_y;
        tool_fill_rect.y += tool_outline_rect.h + tool_square_gap_y;
    }

    tool_square_gap_y = 7.0;
    tool_square_offset_x = toolbox_bounds_rect.w/2 - 30;
    tool_square_offset_y = 35.0;
    tool_square_padding = 5.0;
    tool_square_gap_y = 27;
    tool_outline_rect.x = toolbox_bounds_rect.x + tool_square_offset_x;
    tool_outline_rect.y = toolbox_bounds_rect.y + tool_square_offset_y;
    tool_outline_rect.w = tool_outline_rect.h = 15;
    tool_fill_rect.x = tool_outline_rect.x + tool_square_padding;
    tool_fill_rect.y = tool_outline_rect.y + tool_square_padding;
    tool_fill_rect.w = tool_outline_rect.w - 2*tool_square_padding;
    tool_fill_rect.h = tool_outline_rect.h - 2*tool_square_padding;
    for(int i = static_cast<int>(ToolsEnum::FILL_TOGGLER); i <= static_cast<int>(ToolsEnum::OUTLINE_TOGGLER); ++i){
        SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
        // SDL_RenderFillRectF(renderer, &tool_outline_rect);
        Ellipse::drawEllipseSolid(renderer, context.color.colors[static_cast<int>((ColorsEnum::WHITE))], tool_outline_rect);
        context.buttons.tool_buttons[i] = new Button(tool_outline_rect);
        context.buttons.tool_buttons[i]->setButtonType(ButtonTypesEnum::TOGGLE_BUTTON);
        context.buttons.tool_buttons[i]->setState(true);

        SDL_Surface* text_surface = (i == static_cast<int>(ToolsEnum::FILL_TOGGLER) ? IMG_Load("textures/Fill_text.bmp") : IMG_Load("textures/Outline_text.bmp"));
        SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
        SDL_SetTextureBlendMode(text_texture, SDL_BLENDMODE_BLEND);
        SDL_FRect dest_rect;
        dest_rect.w = ((double)text_surface->w);
        dest_rect.h = ((double)text_surface->h);
        dest_rect.x = tool_fill_rect.x + tool_fill_rect.w + 10;
        dest_rect.y = tool_fill_rect.y - dest_rect.h/2 + 2;
        SDL_SetTextureScaleMode(text_texture, SDL_ScaleModeLinear);
        SDL_RenderCopyF(renderer, text_texture, nullptr, &dest_rect);
        SDL_DestroyTexture(text_texture);
        SDL_FreeSurface(text_surface);
        
        tool_outline_rect.y += tool_outline_rect.h + tool_square_gap_y;
        tool_fill_rect.y += tool_outline_rect.h + tool_square_gap_y;
    }

    tool_square_offset_x = 4*toolbox_bounds_rect.w/5 + 30;
    tool_square_offset_y = 55.0;
    tool_square_padding = 5.0;
    tool_square_gap_y = 27;
    tool_outline_rect.x = toolbox_bounds_rect.x + tool_square_offset_x;
    tool_outline_rect.y = toolbox_bounds_rect.y + tool_square_offset_y;
    tool_outline_rect.w = tool_outline_rect.h = 15;
    tool_fill_rect.x = tool_outline_rect.x + tool_square_padding;
    tool_fill_rect.y = tool_outline_rect.y + tool_square_padding;
    tool_fill_rect.w = tool_outline_rect.w - 2*tool_square_padding;
    tool_fill_rect.h = tool_outline_rect.h - 2*tool_square_padding;
    // SDL_RenderFillRectF(renderer, &tool_outline_rect);
    Ellipse::drawEllipseSolid(renderer, context.color.colors[static_cast<int>((ColorsEnum::WHITE))], tool_outline_rect);
    context.buttons.tool_buttons[static_cast<int>(ToolsEnum::TRANSPARENCY_TOGGLER)] = new Button(tool_outline_rect);
    context.buttons.tool_buttons[static_cast<int>(ToolsEnum::TRANSPARENCY_TOGGLER)]->setButtonType(ButtonTypesEnum::TOGGLE_BUTTON);
    context.buttons.tool_buttons[static_cast<int>(ToolsEnum::TRANSPARENCY_TOGGLER)]->setState(false);

    SDL_Surface* text_surface = IMG_Load("textures/Transparent_Fill_text.bmp");
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    SDL_SetTextureBlendMode(text_texture, SDL_BLENDMODE_BLEND);
    SDL_FRect dest_rect;
    dest_rect.w = ((double)text_surface->w);
    dest_rect.h = ((double)text_surface->h);
    dest_rect.x = tool_fill_rect.x + tool_fill_rect.w + 10;
    dest_rect.y = tool_fill_rect.y - dest_rect.h/2 + 2;
    SDL_SetTextureScaleMode(text_texture, SDL_ScaleModeLinear);
    SDL_RenderCopyF(renderer, text_texture, nullptr, &dest_rect);
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);

    return;
}

void initializeCursors(Context &context){
    SDL_Surface* surface = IMG_Load("textures/draw_cursor.bmp");
    context.cursor.draw_cursor = SDL_CreateColorCursor(surface, 11, 11);
    SDL_FreeSurface(surface);
}

void handleColorButtons(Context &context){
    for(int i=0; i<context.buttons.color_buttons.size(); ++i){
        if(context.buttons.color_buttons[i]->getState() == true){
            if(context.color.is_fill_color_selected) context.color.fill_color = context.color.colors[i];
            else context.color.outline_color = context.color.colors[i];
            context.buttons.color_buttons[i]->setState(false);
            // break;
        }
    }
    if(context.color.is_transparent) context.color.fill_color.a = 128;
    else context.color.fill_color.a = 255;

    return;
}

void handleToolButtons(Context &context){
    for(int i=0; i <= static_cast<int>(ToolsEnum::OUTLINE_COLOR_SELECTOR); ++i){
        if(context.buttons.tool_buttons[i]->getState() == true){
            switch(static_cast<ToolsEnum>(i)){
                case ToolsEnum::FILL_COLOR_SELECTOR:
                    context.color.is_fill_color_selected = true;
                    break;

                case ToolsEnum::OUTLINE_COLOR_SELECTOR:
                    context.color.is_fill_color_selected = false;
                    break;

                default:
                    context.selected_tool = static_cast<ToolsEnum>(i);
            }
            context.buttons.tool_buttons[i]->setState(false);
            // break;
        }
    }

    for(int i=static_cast<int>(ToolsEnum::FILL_TOGGLER); i < static_cast<int>(ToolsEnum::NUM_TOOLS); ++i){
        switch(static_cast<ToolsEnum>(i)){
            case ToolsEnum::FILL_TOGGLER:
                context.color.is_filled = context.buttons.tool_buttons[i]->getState();
                break;
            
            case ToolsEnum::OUTLINE_TOGGLER:
                context.color.is_outlined = context.buttons.tool_buttons[i]->getState();
                break;

            case ToolsEnum::TRANSPARENCY_TOGGLER:
                context.color.is_transparent = context.buttons.tool_buttons[i]->getState();
                break;

            default:
                break;
        }
    }


    return;
}

void updateToolBoxOverlay(Context &context, SDL_Renderer* renderer){    
    SDL_SetRenderTarget(renderer, context.texture.toolbox_overlay_texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_Color selection_outline_color = {102, 217, 226, 255};
    SDL_Color selection_fill_color = {0, 0, 0, 255};
    SDL_SetRenderDrawColor(renderer, selection_outline_color.r, selection_outline_color.g, selection_outline_color.b, selection_outline_color.a);
    
    SDL_FRect tool_rect;
    tool_rect.x = context.buttons.tool_buttons[static_cast<int>(context.selected_tool)]->getPosX();
    tool_rect.y = context.buttons.tool_buttons[static_cast<int>(context.selected_tool)]->getPosY();
    tool_rect.w = context.buttons.tool_buttons[static_cast<int>(context.selected_tool)]->getWidth();
    tool_rect.h = context.buttons.tool_buttons[static_cast<int>(context.selected_tool)]->getHeight();
    SDL_RenderDrawRectF(renderer, &tool_rect);
    
    double tool_rect_padding = 5.0;
    SDL_FRect fill_color_selector_outline_rect;
    SDL_FRect fill_color_selector_fill_rect;
    fill_color_selector_outline_rect.x = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::FILL_COLOR_SELECTOR)]->getPosX();
    fill_color_selector_outline_rect.y = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::FILL_COLOR_SELECTOR)]->getPosY();
    fill_color_selector_outline_rect.w = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::FILL_COLOR_SELECTOR)]->getWidth();
    fill_color_selector_outline_rect.h = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::FILL_COLOR_SELECTOR)]->getHeight();
    fill_color_selector_fill_rect.x = fill_color_selector_outline_rect.x + tool_rect_padding;
    fill_color_selector_fill_rect.y = fill_color_selector_outline_rect.y + tool_rect_padding;
    fill_color_selector_fill_rect.w = fill_color_selector_outline_rect.w - 2*tool_rect_padding;
    fill_color_selector_fill_rect.h = fill_color_selector_outline_rect.h - 2*tool_rect_padding;
    SDL_SetRenderDrawColor(renderer, context.color.fill_color.r, context.color.fill_color.g, context.color.fill_color.b, 255-(255-context.color.fill_color.a)/2);
    SDL_RenderFillRectF(renderer, &fill_color_selector_fill_rect);
    SDL_SetRenderDrawColor(renderer, 128, 128, 126, 255);
    SDL_RenderDrawRectF(renderer, &fill_color_selector_fill_rect);
    if(context.color.is_fill_color_selected){
        SDL_SetRenderDrawColor(renderer, selection_outline_color.r, selection_outline_color.g, selection_outline_color.b, selection_outline_color.a);
        SDL_RenderDrawRectF(renderer, &fill_color_selector_outline_rect);
    }

    SDL_FRect outline_color_selector_outline_rect;
    SDL_FRect outline_color_selector_fill_rect;
    outline_color_selector_outline_rect.x = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::OUTLINE_COLOR_SELECTOR)]->getPosX();
    outline_color_selector_outline_rect.y = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::OUTLINE_COLOR_SELECTOR)]->getPosY();
    outline_color_selector_outline_rect.w = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::OUTLINE_COLOR_SELECTOR)]->getWidth();
    outline_color_selector_outline_rect.h = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::OUTLINE_COLOR_SELECTOR)]->getHeight();
    outline_color_selector_fill_rect.x = outline_color_selector_outline_rect.x + tool_rect_padding;
    outline_color_selector_fill_rect.y = outline_color_selector_outline_rect.y + tool_rect_padding;
    outline_color_selector_fill_rect.w = outline_color_selector_outline_rect.w - 2*tool_rect_padding;
    outline_color_selector_fill_rect.h = outline_color_selector_outline_rect.h - 2*tool_rect_padding;
    SDL_SetRenderDrawColor(renderer, context.color.outline_color.r, context.color.outline_color.g, context.color.outline_color.b, context.color.outline_color.a);
    SDL_RenderFillRectF(renderer, &outline_color_selector_fill_rect);
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_RenderDrawRectF(renderer, &outline_color_selector_fill_rect);
    if(!context.color.is_fill_color_selected){
        SDL_SetRenderDrawColor(renderer, selection_outline_color.r, selection_outline_color.g, selection_outline_color.b, selection_outline_color.a);
        SDL_RenderDrawRectF(renderer, &outline_color_selector_outline_rect);
    }

    SDL_FRect temp_rect;
    tool_rect_padding = 3;
    if(context.color.is_filled){
        temp_rect.x = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::FILL_TOGGLER)]->getPosX() + tool_rect_padding;
        temp_rect.y = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::FILL_TOGGLER)]->getPosY() + tool_rect_padding;
        temp_rect.w = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::FILL_TOGGLER)]->getWidth() - 2*tool_rect_padding;
        temp_rect.h = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::FILL_TOGGLER)]->getHeight() - 2*tool_rect_padding;
        SDL_SetRenderDrawColor(renderer, selection_fill_color.r, selection_fill_color.g, selection_fill_color.b, selection_fill_color.a);
        // SDL_RenderFillRectF(renderer, &temp_rect);
        Ellipse::drawEllipseSolid(renderer, selection_fill_color, temp_rect);
    }

    if(context.color.is_outlined){
        temp_rect.x = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::OUTLINE_TOGGLER)]->getPosX() + tool_rect_padding;
        temp_rect.y = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::OUTLINE_TOGGLER)]->getPosY() + tool_rect_padding;
        temp_rect.w = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::OUTLINE_TOGGLER)]->getWidth() - 2*tool_rect_padding;
        temp_rect.h = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::OUTLINE_TOGGLER)]->getHeight() - 2*tool_rect_padding;
        SDL_SetRenderDrawColor(renderer, selection_fill_color.r, selection_fill_color.g, selection_fill_color.b, selection_fill_color.a);
        // SDL_RenderFillRectF(renderer, &temp_rect);
        Ellipse::drawEllipseSolid(renderer, selection_fill_color, temp_rect);
    }

    if(context.color.is_transparent){
        temp_rect.x = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::TRANSPARENCY_TOGGLER)]->getPosX() + tool_rect_padding;
        temp_rect.y = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::TRANSPARENCY_TOGGLER)]->getPosY() + tool_rect_padding;
        temp_rect.w = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::TRANSPARENCY_TOGGLER)]->getWidth() - 2*tool_rect_padding;
        temp_rect.h = context.buttons.tool_buttons[static_cast<int>(ToolsEnum::TRANSPARENCY_TOGGLER)]->getHeight() - 2*tool_rect_padding;
        SDL_SetRenderDrawColor(renderer, selection_fill_color.r, selection_fill_color.g, selection_fill_color.b, selection_fill_color.a);
        // SDL_RenderFillRectF(renderer, &temp_rect);
        Ellipse::drawEllipseSolid(renderer, selection_fill_color, temp_rect);
    }
}

void saveHistory(Context &context, SDL_Renderer* renderer){
    if(context.history.curr_history_idx == context.history.draw_history.size()-1){
        context.history.draw_history.push_back(copyTexture(context.texture.canvas_texture, renderer));
        ++context.history.curr_history_idx;
    }
    else{
        SDL_DestroyTexture(context.history.draw_history[context.history.curr_history_idx+1]);
        context.history.draw_history[++context.history.curr_history_idx] = copyTexture(context.texture.canvas_texture, renderer);
    }
    context.history.max_valid_history_idx = context.history.curr_history_idx;
    return;
}

inline void handleUndo(Context &context, SDL_Renderer* renderer){
    if(context.history.curr_history_idx > 0) context.texture.canvas_texture = copyTexture(context.history.draw_history[--context.history.curr_history_idx], renderer);
    return;
}

inline void handleRedo(Context &context, SDL_Renderer* renderer){
    if(context.history.curr_history_idx < context.history.max_valid_history_idx) context.texture.canvas_texture = copyTexture(context.history.draw_history[++context.history.curr_history_idx], renderer);
    return;
}

void saveTexture(SDL_Renderer* renderer, SDL_Texture* texture){
    if(texture == nullptr) return;
    int texture_width, texture_height;
    SDL_BlendMode texture_blend_mode;
    SDL_QueryTexture(texture, nullptr, nullptr, &texture_width, &texture_height);
    SDL_Texture* prev_render_target = SDL_GetRenderTarget(renderer);
    SDL_GetTextureBlendMode(texture, &texture_blend_mode);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, texture_width, texture_height, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    SDL_SetSurfaceBlendMode(surface, texture_blend_mode);
    SDL_SetRenderTarget(renderer, texture);
    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);
    SDL_SetRenderTarget(renderer, prev_render_target);

    const char *filetypes[] = { "*.png" };
    const char *filename = tinyfd_saveFileDialog(
        "Save Image",          // Dialog title
        "image.png",           // Default filename
        1,                     // Number of file types
        filetypes,             // File types array
        NULL                   // Optional description for the file types
    );

    if (filename) {
        string filename_str(filename);
        if(filename_str.size() < 5 || filename_str.substr(filename_str.size()-4, 4) != ".png") filename_str += ".png";
        IMG_SavePNG(surface, filename_str.c_str()); 
    }

    SDL_FreeSurface(surface);
}

void bucketFill(SDL_Renderer* renderer, SDL_Texture* texture, SDL_Color fill_color, SDL_Point start_point){
    if(texture == nullptr) return;
    int texture_width, texture_height;
    SDL_BlendMode texture_blend_mode;
    SDL_QueryTexture(texture, nullptr, nullptr, &texture_width, &texture_height);
    SDL_GetTextureBlendMode(texture, &texture_blend_mode);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, texture_width, texture_height, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    SDL_SetSurfaceBlendMode(surface, texture_blend_mode);
    SDL_SetRenderTarget(renderer, texture);
    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);
    Uint32 start_pixel_color = *((Uint32*)surface->pixels + start_point.y*surface->w + start_point.x);
    Uint32 fill_pixel_color = SDL_MapRGBA(SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888), fill_color.r, fill_color.g, fill_color.b, fill_color.a);
    if(start_pixel_color == fill_pixel_color) return;
    queue<SDL_Point> q;
    SDL_Point test_point;
    q.push(start_point);
    *((Uint32*)surface->pixels + start_point.y*surface->w + start_point.x) = fill_pixel_color;
    while(!q.empty()){
        start_point = q.front(); q.pop();
        test_point = {start_point.x - 1, start_point.y};
        if(test_point.x >=0 && test_point.y >=0 && test_point.x < texture_width && test_point.y < texture_height && *((Uint32*)surface->pixels + test_point.y*surface->w + test_point.x) == start_pixel_color){
            *((Uint32*)surface->pixels + test_point.y*surface->w + test_point.x) = fill_pixel_color;
            q.push(test_point);
        }

        test_point = {start_point.x + 1, start_point.y};
        if(test_point.x >=0 && test_point.y >=0 && test_point.x < texture_width && test_point.y < texture_height && *((Uint32*)surface->pixels + test_point.y*surface->w + test_point.x) == start_pixel_color){
            *((Uint32*)surface->pixels + test_point.y*surface->w + test_point.x) = fill_pixel_color;
            q.push(test_point);
        }
        
        test_point = {start_point.x, start_point.y - 1};
        if(test_point.x >=0 && test_point.y >=0 && test_point.x < texture_width && test_point.y < texture_height && *((Uint32*)surface->pixels + test_point.y*surface->w + test_point.x) == start_pixel_color){
            *((Uint32*)surface->pixels + test_point.y*surface->w + test_point.x) = fill_pixel_color;
            q.push(test_point);
        }
        
        test_point = {start_point.x, start_point.y + 1};
        if(test_point.x >=0 && test_point.y >=0 && test_point.x < texture_width && test_point.y < texture_height && *((Uint32*)surface->pixels + test_point.y*surface->w + test_point.x) == start_pixel_color){
            *((Uint32*)surface->pixels + test_point.y*surface->w + test_point.x) = fill_pixel_color;
            q.push(test_point);
        }
    }

    SDL_Texture* temp_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetRenderTarget(renderer, texture);
    SDL_RenderCopy(renderer, temp_texture, nullptr, nullptr);
    SDL_DestroyTexture(temp_texture);
}

int main(int argc, char** argv){

    // Initialization
    if(!init()){
        cerr << "Initialization failed: " << SDL_GetError();
        return -1;
    }

    initEllipseTextures();

    Context context;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    context.texture.canvas_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
    context.texture.canvas_overlay_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetTextureBlendMode(context.texture.canvas_texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(context.texture.canvas_overlay_texture, SDL_BLENDMODE_BLEND);


    initializeColors(context.color.colors);

    SDL_FRect toolbox_bounds_rect;
    context.buttons.color_buttons.resize(static_cast<int>(ColorsEnum::NUM_COLORS));
    context.buttons.tool_buttons.resize(static_cast<int>(ToolsEnum::NUM_TOOLS));
    initializeToolboxAndButtons(context, renderer, toolbox_bounds_rect);

    initializeCursors(context);
    SDL_SetCursor(context.cursor.draw_cursor);

    SDL_SetRenderTarget(renderer, context.texture.canvas_texture);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    Uint64 start_time, elapsed_time;
    
    context.color.fill_color = context.color.colors[static_cast<int>(ColorsEnum::WHITE)];
    context.color.outline_color = context.color.colors[static_cast<int>(ColorsEnum::BLACK)];
     
    context.object.eraser_rect.w = context.object.eraser_rect.h = ERASER_SIDE_LEN;
    vec2 modified_mouse_pos;
    context.history.draw_history.push_back(copyTexture(context.texture.canvas_texture, renderer));

    updateToolBoxOverlay(context, renderer);

    bool running = true;

    while(running){
        start_time = SDL_GetTicks64();

        SDL_Event event;
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
                    running = false;
                    break;
                
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                        // initializeToolboxAndButtons(renderer, toolbox_texture, toolbox_overlay_texture, toolbox_bounds_rect, color_buttons, tool_buttons);
                    }
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if(event.button.button == SDL_BUTTON_LEFT){
                        context.mouse.initial_pos = {event.button.x,event.button.y};
                        context.mouse.curr_pos = context.mouse.initial_pos;

                        if(context.mouse.initial_pos.y < toolbox_bounds_rect.y + toolbox_bounds_rect.h){
                            Button::updateAllStates(event);    // update button states based on clicks
                            handleToolButtons(context);
                            handleColorButtons(context);
                            updateToolBoxOverlay(context, renderer);
                        }
                        else{
                            if(context.selected_tool == ToolsEnum::RECT){
                                context.is_drawing = true;
                                context.object.draw_rect.setWidth(0);
                                context.object.draw_rect.setHeight(0);
                                context.object.draw_rect.setPos(context.mouse.initial_pos);
                                context.object.draw_rect.setFillColor(context.color.fill_color);
                                context.object.draw_rect.setOutlineColor(context.color.outline_color);
                                if(context.color.is_filled) context.object.draw_rect.enableFill();
                                else context.object.draw_rect.disableFill();
                                if(context.color.is_outlined) context.object.draw_rect.enableOutline();
                                else context.object.draw_rect.disableOutline();
                                SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                                SDL_SetRenderDrawColor(renderer, context.color.fill_color.r, context.color.fill_color.g, context.color.fill_color.b, 0);
                                SDL_RenderClear(renderer); 
                                context.object.draw_rect.draw(renderer);
                            }

                            else if(context.selected_tool == ToolsEnum::ELLIPSE){
                                context.is_drawing = true;
                                context.object.draw_ellipse.setRadius(0);
                                context.object.draw_ellipse.setPos(context.mouse.initial_pos);
                                context.object.draw_ellipse.setFillColor(context.color.fill_color);
                                context.object.draw_ellipse.setOutlineColor(context.color.outline_color);
                                if(context.color.is_filled) context.object.draw_ellipse.enableFill();
                                else context.object.draw_ellipse.disableFill();
                                if(context.color.is_outlined) context.object.draw_ellipse.enableOutline();
                                else context.object.draw_ellipse.disableOutline();
                                SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                                SDL_SetRenderDrawColor(renderer, context.color.fill_color.r, context.color.fill_color.g, context.color.fill_color.b, 0);
                                SDL_RenderClear(renderer); 
                                context.object.draw_ellipse.draw(renderer);
                            }

                            else if(context.selected_tool == ToolsEnum::LINE){
                                context.is_drawing = true;
                                SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                                SDL_RenderClear(renderer);
                                SDL_SetRenderDrawColor(renderer, context.color.outline_color.r, context.color.outline_color.g, context.color.outline_color.b, context.color.outline_color.a);
                                SDL_RenderDrawPoint(renderer, context.mouse.initial_pos.x, context.mouse.initial_pos.y);
                            }

                            else if(context.selected_tool == ToolsEnum::SCRIBBLE){
                                context.is_drawing = true;
                                SDL_SetRenderTarget(renderer, context.texture.canvas_texture);
                                SDL_SetRenderDrawColor(renderer, context.color.outline_color.r, context.color.outline_color.g, context.color.outline_color.b, context.color.outline_color.a);
                                SDL_RenderDrawPoint(renderer, context.mouse.initial_pos.x, context.mouse.initial_pos.y);
                            }
                            
                            else if(context.selected_tool == ToolsEnum::ERASER){
                                context.is_drawing = true;
                                context.object.eraser_rect.x = context.mouse.initial_pos.x - ERASER_SIDE_LEN/2;
                                context.object.eraser_rect.y = context.mouse.initial_pos.y - ERASER_SIDE_LEN/2;
                                SDL_SetRenderTarget(renderer, context.texture.canvas_texture);
                                Ellipse::drawEllipseSolid(renderer, {255, 255, 255, 255}, context.object.eraser_rect);
                            }

                            else if(context.selected_tool == ToolsEnum::BUCKETFILL){
                                bucketFill(renderer, context.texture.canvas_texture, context.color.fill_color, {event.button.x, event.button.y});
                                saveHistory(context, renderer);
                            }
                        }
                    }

                    else if(event.button.button == SDL_BUTTON_RIGHT){
                        // bucketFill(renderer, canvas_texture, {0,0,255,255}, {event.button.x, event.button.y});
                    }

                    break;

                case SDL_MOUSEMOTION:
                    context.mouse.curr_pos = {event.button.x,event.button.y};

                    if(context.selected_tool == ToolsEnum::RECT){
                        if(context.is_drawing){
                            if(context.key.shift_pressed){
                                int square_side_len = min(abs(context.mouse.curr_pos.x-context.mouse.initial_pos.x),abs(context.mouse.curr_pos.y-context.mouse.initial_pos.y));
                                modified_mouse_pos.x = context.mouse.initial_pos.x + (context.mouse.curr_pos.x > context.mouse.initial_pos.x ? 1 : -1)*square_side_len;
                                modified_mouse_pos.y = context.mouse.initial_pos.y + (context.mouse.curr_pos.y > context.mouse.initial_pos.y ? 1 : -1)*square_side_len;
                            }
                            else modified_mouse_pos = context.mouse.curr_pos;
                            context.object.draw_rect.setWidth(abs(modified_mouse_pos.x-context.mouse.initial_pos.x));
                            context.object.draw_rect.setHeight(abs(modified_mouse_pos.y-context.mouse.initial_pos.y));
                            context.object.draw_rect.setPos((context.mouse.initial_pos.x+modified_mouse_pos.x)/2, (context.mouse.initial_pos.y+modified_mouse_pos.y)/2);
                            SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                            SDL_SetRenderDrawColor(renderer, context.color.fill_color.r, context.color.fill_color.g, context.color.fill_color.b, 0);
                            SDL_RenderClear(renderer);
                            context.object.draw_rect.draw(renderer);
                        }
                    }
                        
                    else if(context.selected_tool == ToolsEnum::ELLIPSE){
                        if(context.is_drawing){
                            if(context.key.shift_pressed){
                                int square_side_len = min(abs(context.mouse.curr_pos.x-context.mouse.initial_pos.x),abs(context.mouse.curr_pos.y-context.mouse.initial_pos.y));
                                modified_mouse_pos.x = context.mouse.initial_pos.x + (context.mouse.curr_pos.x > context.mouse.initial_pos.x ? 1 : -1)*square_side_len;
                                modified_mouse_pos.y = context.mouse.initial_pos.y + (context.mouse.curr_pos.y > context.mouse.initial_pos.y ? 1 : -1)*square_side_len;
                            }
                            else modified_mouse_pos = context.mouse.curr_pos;
                            context.object.draw_ellipse.setRadii(abs(modified_mouse_pos.x-context.mouse.initial_pos.x)/2,abs(modified_mouse_pos.y-context.mouse.initial_pos.y)/2);
                            context.object.draw_ellipse.setPos((context.mouse.initial_pos.x+modified_mouse_pos.x)/2, (context.mouse.initial_pos.y+modified_mouse_pos.y)/2);
                            SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                            SDL_SetRenderDrawColor(renderer, context.color.fill_color.r, context.color.fill_color.g, context.color.fill_color.b, 0);
                            SDL_RenderClear(renderer);
                            context.object.draw_ellipse.draw(renderer);
                        }
                    }

                    else if(context.selected_tool == ToolsEnum::LINE){
                        if(context.is_drawing){
                            if(context.key.shift_pressed){
                                double d_x = abs(context.mouse.curr_pos.x - context.mouse.initial_pos.x);
                                double d_y = abs(context.mouse.curr_pos.y - context.mouse.initial_pos.y);
                                if(d_x < 0.4*d_y){
                                    modified_mouse_pos.x = context.mouse.initial_pos.x;
                                    modified_mouse_pos.y = context.mouse.curr_pos.y;
                                }
                                else if(d_y < 0.4*d_x){
                                    modified_mouse_pos.x = context.mouse.curr_pos.x;
                                    modified_mouse_pos.y = context.mouse.initial_pos.y;
                                }
                                else{
                                    modified_mouse_pos.x = context.mouse.initial_pos.x + min(d_x, d_y)*(context.mouse.curr_pos.x - context.mouse.initial_pos.x)/d_x;
                                    modified_mouse_pos.y = context.mouse.initial_pos.y + min(d_x, d_y)*(context.mouse.curr_pos.y - context.mouse.initial_pos.y)/d_y;
                                }
                            }
                            else modified_mouse_pos = context.mouse.curr_pos;
                            SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                            SDL_RenderClear(renderer);
                            SDL_SetRenderDrawColor(renderer, context.color.outline_color.r, context.color.outline_color.g, context.color.outline_color.b, context.color.outline_color.a);
                            SDL_RenderDrawLineF(renderer, context.mouse.initial_pos.x, context.mouse.initial_pos.y, modified_mouse_pos.x, modified_mouse_pos.y);
                        }
                    }

                    else if(context.selected_tool == ToolsEnum::SCRIBBLE){
                        if(context.is_drawing){
                            SDL_SetRenderTarget(renderer, context.texture.canvas_texture);
                            SDL_SetRenderDrawColor(renderer, context.color.outline_color.r, context.color.outline_color.g, context.color.outline_color.b, context.color.outline_color.a);
                            SDL_RenderDrawLineF(renderer, context.mouse.initial_pos.x, context.mouse.initial_pos.y, context.mouse.curr_pos.x, context.mouse.curr_pos.y);
                            context.mouse.initial_pos = context.mouse.curr_pos;
                        }
                    }

                    else if(context.selected_tool == ToolsEnum::ERASER){                        
                        if(context.is_drawing){
                            context.object.eraser_rect.x = context.mouse.curr_pos.x - ERASER_SIDE_LEN/2;
                            context.object.eraser_rect.y = context.mouse.curr_pos.y - ERASER_SIDE_LEN/2;
                            SDL_SetRenderTarget(renderer, context.texture.canvas_texture);
                            Rect::drawRotatedFillRectangle(renderer, {(float)(context.mouse.initial_pos.x + context.mouse.curr_pos.x)/2, (float)(context.mouse.initial_pos.y + context.mouse.curr_pos.y)/2}, distance(context.mouse.curr_pos, context.mouse.initial_pos), ERASER_SIDE_LEN, 180*atan2(context.mouse.curr_pos.y-context.mouse.initial_pos.y, context.mouse.curr_pos.x-context.mouse.initial_pos.x)/M_PI, {255, 255, 255, 255});
                            SDL_SetRenderTarget(renderer, context.texture.canvas_texture);
                            Ellipse::drawEllipseSolid(renderer, {255, 255, 255, 255}, context.object.eraser_rect);
                            context.mouse.initial_pos = context.mouse.curr_pos;
                        }
                    }
                        
                    break;
                
                case SDL_MOUSEBUTTONUP:
                    if(event.button.button == SDL_BUTTON_LEFT){
                        if(context.selected_tool == ToolsEnum::RECT){
                            if(context.is_drawing){
                                context.is_drawing = false;
                                SDL_SetRenderTarget(renderer, context.texture.canvas_texture);
                                context.object.draw_rect.draw(renderer);
                                SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                                SDL_RenderClear(renderer);
                                saveHistory(context, renderer);
                            }
                        }

                        else if(context.selected_tool == ToolsEnum::ELLIPSE){
                            if(context.is_drawing){
                                context.is_drawing = false;
                                SDL_SetRenderTarget(renderer, context.texture.canvas_texture);
                                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                                context.object.draw_ellipse.draw(renderer);
                                SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                                SDL_RenderClear(renderer);
                                saveHistory(context, renderer);
                            }
                        }

                        else if(context.selected_tool == ToolsEnum::LINE){
                            if(context.is_drawing){
                                context.is_drawing = false;
                                if(context.key.shift_pressed){
                                    double d_x = abs(context.mouse.curr_pos.x - context.mouse.initial_pos.x);
                                    double d_y = abs(context.mouse.curr_pos.y - context.mouse.initial_pos.y);
                                    if(d_x < 0.4*d_y){
                                        modified_mouse_pos.x = context.mouse.initial_pos.x;
                                        modified_mouse_pos.y = context.mouse.curr_pos.y;
                                    }
                                    else if(d_y < 0.4*d_x){
                                        modified_mouse_pos.x = context.mouse.curr_pos.x;
                                        modified_mouse_pos.y = context.mouse.initial_pos.y;
                                    }
                                    else{
                                        modified_mouse_pos.x = context.mouse.initial_pos.x + min(d_x, d_y)*(context.mouse.curr_pos.x - context.mouse.initial_pos.x)/d_x;
                                        modified_mouse_pos.y = context.mouse.initial_pos.y + min(d_x, d_y)*(context.mouse.curr_pos.y - context.mouse.initial_pos.y)/d_y;
                                    }
                                }
                                else modified_mouse_pos = context.mouse.curr_pos;
                                SDL_SetRenderTarget(renderer, context.texture.canvas_texture);
                                SDL_SetRenderDrawColor(renderer, context.color.outline_color.r, context.color.outline_color.g, context.color.outline_color.b, context.color.outline_color.a);
                                SDL_RenderDrawLineF(renderer, context.mouse.initial_pos.x, context.mouse.initial_pos.y, modified_mouse_pos.x, modified_mouse_pos.y);
                                SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                                SDL_RenderClear(renderer);
                                saveHistory(context, renderer);
                            }
                        }

                        else if(context.selected_tool == ToolsEnum::SCRIBBLE){
                            if(context.is_drawing){
                                context.is_drawing = false;
                                saveHistory(context, renderer);
                            }                                
                        }

                        else if(context.selected_tool == ToolsEnum::ERASER){
                            if(context.is_drawing){
                                context.is_drawing = false;
                                context.object.eraser_rect.x = context.mouse.curr_pos.x - ERASER_SIDE_LEN/2;
                                context.object.eraser_rect.y = context.mouse.curr_pos.y - ERASER_SIDE_LEN/2;
                                SDL_SetRenderTarget(renderer, context.texture.canvas_texture);
                                Ellipse::drawEllipseSolid(renderer, {255, 255, 255, 255}, context.object.eraser_rect);
                                saveHistory(context, renderer);
                            }
                        }
                    }
                    break;

                case SDL_KEYDOWN:
                    if(event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT){
                        if(context.key.shift_pressed) break;

                        context.key.shift_pressed = true;
                        
                        if(context.selected_tool == ToolsEnum::RECT){
                            if(context.is_drawing){
                                vec2 modified_mouse_pos;
                                int side_len = (int)min(abs(context.mouse.curr_pos.x-context.mouse.initial_pos.x),abs(context.mouse.curr_pos.y-context.mouse.initial_pos.y));
                                modified_mouse_pos.x = context.mouse.initial_pos.x + (context.mouse.curr_pos.x > context.mouse.initial_pos.x ? 1 : -1)*side_len;
                                modified_mouse_pos.y = context.mouse.initial_pos.y + (context.mouse.curr_pos.y > context.mouse.initial_pos.y ? 1 : -1)*side_len;
                                context.object.draw_rect.setWidth(abs(modified_mouse_pos.x-context.mouse.initial_pos.x));
                                context.object.draw_rect.setHeight(abs(modified_mouse_pos.y-context.mouse.initial_pos.y));
                                context.object.draw_rect.setPos((context.mouse.initial_pos.x+modified_mouse_pos.x)/2, (context.mouse.initial_pos.y+modified_mouse_pos.y)/2);
                                SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                                SDL_SetRenderDrawColor(renderer, context.color.fill_color.r, context.color.fill_color.g, context.color.fill_color.b, 0);
                                SDL_RenderClear(renderer);
                                context.object.draw_rect.draw(renderer);
                            }
                        }
                                
                        else if(context.selected_tool == ToolsEnum::ELLIPSE){
                            if(context.is_drawing){
                                vec2 modified_mouse_pos;
                                int side_len = (int)min(abs(context.mouse.curr_pos.x-context.mouse.initial_pos.x),abs(context.mouse.curr_pos.y-context.mouse.initial_pos.y));
                                modified_mouse_pos.x = context.mouse.initial_pos.x + (context.mouse.curr_pos.x > context.mouse.initial_pos.x ? 1 : -1)*side_len;
                                modified_mouse_pos.y = context.mouse.initial_pos.y + (context.mouse.curr_pos.y > context.mouse.initial_pos.y ? 1 : -1)*side_len;
                                context.object.draw_ellipse.setRadii(abs(modified_mouse_pos.x-context.mouse.initial_pos.x)/2, abs(modified_mouse_pos.y-context.mouse.initial_pos.y)/2);
                                context.object.draw_ellipse.setPos((context.mouse.initial_pos.x+modified_mouse_pos.x)/2, (context.mouse.initial_pos.y+modified_mouse_pos.y)/2);
                                SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                                SDL_SetRenderDrawColor(renderer, context.color.fill_color.r, context.color.fill_color.g, context.color.fill_color.b, 0);
                                SDL_RenderClear(renderer);
                                context.object.draw_ellipse.draw(renderer);
                            }
                        }

                        else if(context.selected_tool == ToolsEnum::LINE){
                            if(context.is_drawing){
                                double d_x = abs(context.mouse.curr_pos.x - context.mouse.initial_pos.x);
                                double d_y = abs(context.mouse.curr_pos.y - context.mouse.initial_pos.y);
                                if(d_x < 0.4*d_y){
                                    modified_mouse_pos.x = context.mouse.initial_pos.x;
                                    modified_mouse_pos.y = context.mouse.curr_pos.y;
                                }
                                else if(d_y < 0.4*d_x){
                                    modified_mouse_pos.x = context.mouse.curr_pos.x;
                                    modified_mouse_pos.y = context.mouse.initial_pos.y;
                                }
                                else{
                                    modified_mouse_pos.x = context.mouse.initial_pos.x + min(d_x, d_y)*(context.mouse.curr_pos.x - context.mouse.initial_pos.x)/d_x;
                                    modified_mouse_pos.y = context.mouse.initial_pos.y + min(d_x, d_y)*(context.mouse.curr_pos.y - context.mouse.initial_pos.y)/d_y;
                                }
                                SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                                SDL_RenderClear(renderer);
                                SDL_SetRenderDrawColor(renderer, context.color.outline_color.r, context.color.outline_color.g, context.color.outline_color.b, context.color.outline_color.a);
                                SDL_RenderDrawLineF(renderer, context.mouse.initial_pos.x, context.mouse.initial_pos.y, modified_mouse_pos.x, modified_mouse_pos.y);
                            }
                        }

                    }

                    else if(event.key.keysym.sym == SDLK_LCTRL || event.key.keysym.sym == SDLK_RCTRL){
                        context.key.ctrl_pressed = true;
                    }

                    // else if(event.key.keysym.sym == SDLK_1){
                    //     if(!context.is_drawing) context.selected_tool = ToolsEnum::LINE;
                    // }

                    // else if(event.key.keysym.sym == SDLK_2){
                    //     if(!context.is_drawing) context.selected_tool = ToolsEnum::SCRIBBLE;
                    // }
                    
                    // else if(event.key.keysym.sym == SDLK_3){
                    //     if(!context.is_drawing) context.selected_tool = ToolsEnum::RECT;
                    // }

                    // else if(event.key.keysym.sym == SDLK_4){
                    //     if(!context.is_drawing) context.selected_tool = ToolsEnum::ELLIPSE;
                    // }

                    // else if(event.key.keysym.sym == SDLK_5){
                    //     if(!context.is_drawing) context.selected_tool = ToolsEnum::ERASER;
                    // }

                    // else if(event.key.keysym.sym == SDLK_6){
                    //     if(!context.is_drawing) context.selected_tool = ToolsEnum::BUCKETFILL;
                    // }

                    else if(event.key.keysym.sym == SDLK_z){
                        if(context.key.ctrl_pressed) handleUndo(context, renderer);
                    }

                    else if(event.key.keysym.sym == SDLK_y){
                        if(context.key.ctrl_pressed) handleRedo(context, renderer);
                    }

                    else if(event.key.keysym.sym == SDLK_s){
                        if(context.key.ctrl_pressed) saveTexture(renderer, context.texture.canvas_texture);
                    }

                    break;

                case SDL_KEYUP:
                    if(event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT){
                        context.key.shift_pressed = false;

                        if(context.selected_tool == ToolsEnum::RECT){
                            if(context.is_drawing){
                                context.object.draw_rect.setWidth(abs(context.mouse.curr_pos.x-context.mouse.initial_pos.x));
                                context.object.draw_rect.setHeight(abs(context.mouse.curr_pos.y-context.mouse.initial_pos.y));
                                context.object.draw_rect.setPos((context.mouse.initial_pos.x+context.mouse.curr_pos.x)/2, (context.mouse.initial_pos.y+context.mouse.curr_pos.y)/2);
                                SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                                SDL_SetRenderDrawColor(renderer, context.color.fill_color.r, context.color.fill_color.g, context.color.fill_color.b, 0);
                                SDL_RenderClear(renderer);
                                context.object.draw_rect.draw(renderer);
                            }
                        }
                                
                        else if(context.selected_tool == ToolsEnum::ELLIPSE){
                            if(context.is_drawing){
                                context.object.draw_ellipse.setRadii(abs(context.mouse.curr_pos.x-context.mouse.initial_pos.x)/2, abs(context.mouse.curr_pos.y-context.mouse.initial_pos.y)/2);
                                context.object.draw_ellipse.setPos((context.mouse.initial_pos.x+context.mouse.curr_pos.x)/2, (context.mouse.initial_pos.y+context.mouse.curr_pos.y)/2);
                                SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                                SDL_SetRenderDrawColor(renderer, context.color.fill_color.r, context.color.fill_color.g, context.color.fill_color.b, 0);
                                SDL_RenderClear(renderer);
                                context.object.draw_ellipse.draw(renderer);
                            }
                        }

                        else if(context.selected_tool == ToolsEnum::LINE){
                            if(context.is_drawing){
                                SDL_SetRenderTarget(renderer, context.texture.canvas_overlay_texture);
                                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                                SDL_RenderClear(renderer);
                                SDL_SetRenderDrawColor(renderer, context.color.outline_color.r, context.color.outline_color.g, context.color.outline_color.b, context.color.outline_color.a);
                                SDL_RenderDrawLineF(renderer, context.mouse.initial_pos.x, context.mouse.initial_pos.y, context.mouse.curr_pos.x, context.mouse.curr_pos.y);
                            }
                        }
                    }

                    else if(event.key.keysym.sym == SDLK_LCTRL || event.key.keysym.sym == SDLK_RCTRL){
                        context.key.ctrl_pressed = false;
                    }

                    break;

                default:
                    break;
            }
        }
        
        SDL_SetRenderTarget(renderer, nullptr);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, context.texture.canvas_texture, nullptr, nullptr);
        SDL_RenderCopy(renderer, context.texture.canvas_overlay_texture, nullptr, nullptr);
        SDL_RenderCopyF(renderer, context.texture.toolbox_texture, nullptr, &toolbox_bounds_rect);
        SDL_RenderCopyF(renderer, context.texture.toolbox_overlay_texture, nullptr, &toolbox_bounds_rect);        
        
        SDL_RenderPresent(renderer);
        

        elapsed_time = SDL_GetTicks64() - start_time;
        if (elapsed_time < FRAME_DELAY_MS){
            SDL_Delay(FRAME_DELAY_MS - elapsed_time);
            // cout << "FPS = " << TARGET_FPS << endl;
        }
        // else cout << "FPS = " << 1000/elapsed_time << endl;
    }
    SDL_DestroyTexture(context.texture.canvas_texture);
    SDL_DestroyTexture(context.texture.canvas_overlay_texture);
    SDL_DestroyTexture(context.texture.toolbox_texture);
    SDL_DestroyTexture(context.texture.toolbox_overlay_texture);
    SDL_DestroyTexture(ellipse_solid_texture);
    SDL_DestroyTexture(ellipse_outline_texture);
    SDL_FreeCursor(context.cursor.draw_cursor);
    SDL_CloseAudio();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}