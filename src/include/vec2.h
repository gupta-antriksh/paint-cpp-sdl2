#ifndef VEC2_H
#define VEC2_H

#include <math.h>

typedef struct vec2{
    double x, y;
    vec2(double x_val = 0.0, double y_val = 0.0): x(x_val), y(y_val){};
    vec2(int x_val, int y_val): x((double)x_val), y((double)y_val){};
} vec2;

vec2 operator+(const vec2 &a, const vec2 &b){return {a.x+b.x,a.y+b.y};}
vec2 operator-(const vec2 &a, const vec2 &b){return {a.x-b.x,a.y-b.y};}
vec2 operator*(const double &a, const vec2 &b){return {a*b.x,a*b.y};}
vec2 operator/(const vec2 &a, const double &b){return {a.x/b,a.y/b};}

double distance(vec2 v1, vec2 v2){
    return sqrt((v1.x - v2.x)*(v1.x - v2.x) + (v1.y - v2.y)*(v1.y - v2.y));
}

double norm(vec2 v){
    return sqrt(v.x*v.x + v.y*v.y);
}

double sqnorm(vec2 v){
    return v.x*v.x + v.y*v.y;
}

double dot(vec2 v1, vec2 v2){
    return v1.x*v2.x + v1.y*v2.y;
}

#endif