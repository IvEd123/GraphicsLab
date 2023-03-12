#include <stdio.h>
#include <math.h>

#include <iostream>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#define max(a, b) (a) > (b) ? a : b


/*int counter;
char str[200];
int str_len;*/

/*
|==============================================================|
|============== Geometry functions and structs ================|
|==============================================================|
*/

typedef struct{
    float x, y;
} vec;

vec     add   ( vec v1,  vec v2  ){ return {v1.x + v2.x, v1.y + v2.y}; }
vec     sub   ( vec v1,  vec v2  ){ return {v1.x - v2.x, v1.y - v2.y}; }
vec     mult  ( vec v,   float k ){ return {v.x * k, v.y * k}; }
float   dot   ( vec v1,  vec v2  ){ return  v1.x * v2.x + v1.y * v2.y; }
float   lensq ( vec v            ){ return  dot(v, v); }
float   len   ( vec v            ){ return  sqrt(lensq(v));}
float   getcos( vec v1,  vec v2  ){ return  dot(v1, v2) / sqrt(lensq(v1) * lensq(v2));}

#define maxv(v1, v2) (lensq)

typedef struct{
    vec p0, p1;
} line;

void move(line* l_p, vec v){
    l_p->p0 = add(l_p->p0, v);
    l_p->p1 = add(l_p->p1, v);
}

void print(vec v){
    std::cout << "(" << v.x << "; " << v.y << ")";
}

void scale(line* l_p, float v){
    const vec middle = mult(add(l_p->p1, l_p->p0), 0.5);

    l_p->p0 = sub (l_p->p0, middle);
    l_p->p0 = mult(l_p->p0, v);
    l_p->p0 = add (l_p->p0, middle);

    l_p->p1 = sub (l_p->p1, middle);
    l_p->p1 = mult(l_p->p1, v);
    l_p->p1 = add (l_p->p1, middle);
}

void rotate_tiny_angle(line* l_p, float angle){
    const vec middle = mult(add(l_p->p1, l_p->p0), 0.5);

    vec* v0 = &l_p->p0;
    vec* v1 = &l_p->p1;

    const float _sin = angle;
    const float _cos = (1 - angle * angle);

    *v0 = sub(*v0, middle);
    vec temp_v = *v0;
    v0->x = temp_v.x * _cos - temp_v.y * _sin;
    v0->y = temp_v.x * _sin + temp_v.y * _cos;
    *v0 = add(*v0, middle);

    *v1 = sub(*v1, middle);
    temp_v = *v1;
    v1->x = temp_v.x * _cos - temp_v.y * _sin;
    v1->y = temp_v.x * _sin + temp_v.y * _cos;
    *v1 = add(*v1, middle);
}

void rotate(line* l_p, float angle){

    const vec middle = mult(add(l_p->p1, l_p->p0), 0.5);

    vec* v0 = &l_p->p0;
    vec* v1 = &l_p->p1;

    const float _cos = cos(angle);
    const float _sin = sin(angle);

    *v0 = sub(*v0, middle);
    vec temp_v = *v0;
    v0->x = temp_v.x * _cos - temp_v.y * _sin;
    v0->y = temp_v.x * _sin + temp_v.y * _cos;
    *v0 = add(*v0, middle);

    *v1 = sub(*v1, middle);
    temp_v = *v1;
    v1->x = temp_v.x * _cos - temp_v.y * _sin;
    v1->y = temp_v.x * _sin + temp_v.y * _cos;
    *v1 = add(*v1, middle);

}

/*
|==============================================================|
|============= X system functrions and structs ================|
|==============================================================|
*/

struct {
    Display *dis;
    int screen;
    Window win;
    GC gc;
    int x11_fd;
    fd_set in_fds;
} X_info;

struct {
    XEvent event;
    KeySym key;
    char text[255];
    struct timeval tv;
} X_event;

#define X_INFO X_info.dis, X_info.win, X_info.gc

void init_x() {
	unsigned long black,white;
	X_info.dis=XOpenDisplay((char *)0);
   	X_info.screen=DefaultScreen(X_info.dis);
	black=BlackPixel(X_info.dis, X_info.screen),
	white=WhitePixel(X_info.dis, X_info.screen);  

   	X_info.win=XCreateSimpleWindow(
        X_info.dis,
        DefaultRootWindow(X_info.dis),
        0,0, 400, 600, 5, white, black
    );

	XSetStandardProperties(X_info.dis, X_info.win,"Lab1", "lab1",None,NULL,0,NULL);

	XSelectInput(X_info.dis, X_info.win, ExposureMask|ButtonPressMask|KeyPressMask);

    X_info.gc=XCreateGC(X_info.dis, X_info.win, 0,0);        

	XSetBackground(X_info.dis, X_info.gc, black);
	XSetForeground(X_info.dis, X_info.gc, white);

	XClearWindow(X_info.dis, X_info.win);
	XMapRaised(X_info.dis, X_info.win);
    X_info.x11_fd = ConnectionNumber(X_info.dis);
}

void close_x() {
	XFreeGC(X_info.dis, X_info.gc);
	XDestroyWindow(X_info.dis, X_info.win);
	XCloseDisplay(X_info.dis);	
	exit(1);				
}

#define TARGET_FPS 30.f

typedef enum {
    DEFAULT,
    CHANGE_LINE,
    MOVE_UP,
    MOVE_RIGHT,
    MOVE_DOWN,
    MOVE_LEFT,
    BIGGER,
    SMALLER,
    ROTATE_CLOCKWISE,
    ROTATE_COUNTERCLOCKWISE
} line_control;

line_control checkEvents(){
    line_control ctrl = DEFAULT;
    X_event.tv.tv_sec = 0;
    X_event.tv.tv_usec = (1000.0 / TARGET_FPS) * 1000;

    select(X_info.x11_fd+1, &X_info.in_fds, 0, 0, &X_event.tv);

    while(XPending(X_info.dis))
        XNextEvent(X_info.dis, &X_event.event);
    if( X_event.event.type == KeyPress &&
        XLookupString(&X_event.event.xkey, X_event.text, 255, &X_event.key, 0) == 1
        ){
        if(X_event.text[0] == 27){
            close_x();
        }
        switch (X_event.text[0])
        {
        case 'w':
            ctrl = MOVE_UP;
            break;
        case 'a':
            ctrl = MOVE_LEFT;
            break;
        case 's':
            ctrl = MOVE_DOWN;
            break;
        case 'd':
            ctrl = MOVE_RIGHT;
            break;
        case 'q':
            ctrl = ROTATE_COUNTERCLOCKWISE;
            break;
        case 'e':
            ctrl = ROTATE_CLOCKWISE;
            break;
        case 'r':
            ctrl = BIGGER;
            break;
        case 'f':
            ctrl = SMALLER;
            break;
        case ' ':
            ctrl = CHANGE_LINE;
            break;
        default:
            break;
        }
        /*if(X_event.text[0] == 8){
            str[str_len] = 0;
            if(str_len > 0)
                str_len--;
        }
        else{
            str[str_len] = X_event.text[0];
            if(str_len < 200)
                str_len++;
        }*/



    }   

    /*if( X_event.event.type == ButtonPress){
        printf("You pressed a button at (%i,%i)\n",
        X_event.event.xbutton.x,X_event.event.xbutton.y);
    }*/
    X_event.event.type = 0;
    return ctrl;
}

/*
|==============================================================|
|============== Graphics functions and structs ================|
|==============================================================|
*/

typedef struct {
    int x;
    int y;
} vector;

void drawLine_dda(int x1, int y1, int x2, int y2){
    int delx = abs(x2 - x1);
    int dely = abs(y2 - y1);
    float length = max(delx, dely);

    if(length == 0){
        XDrawPoint(X_INFO, x1, y1);
        return;
    }

    float dx = (x2 - x1) / length;
    float dy = (y2 - y1) / length;
    float x = x1, 
          y = y1;

    length++;

    while(length--){
        x += dx;
        y += dy;
        XDrawPoint(X_INFO, x, y);
    }
    return;
}

#define ABS(x) (x) * ((((x) >= 0) << 1) - 1)


void drawLine_brez(int x0, int y0, int x1, int y1){

    int d[2] = {x1 - x0, y1 - y0};
    d[0] = ABS(d[0]);
    d[1] = ABS(d[1]);

    d[0] += d[0] == 0;
    d[1] += d[1] == 0;

    const int s[2] = {((x0 < x1)<<1)-1 , ((y0 < y1)<<1)-1};

    const int reverse = d[1] > d[0];

    int k = (d[!reverse] << 6) / d[reverse];

    int _s = s[reverse];

    int coords[2] = {x0, y0};
    coords[!reverse] = coords[!reverse] << 6;
    k *= s[!reverse];

    for(int i = 0; i <= d[reverse]; i++){
        coords[reverse] += _s;
        coords[!reverse] += k;
        XDrawPoint(X_INFO, coords[0]>>(6*reverse), coords[1]>>(6*!reverse));
    }


}   


line lines[2] = { 
    {{90, 100}, {120, 150}}, 
    {{30, 15}, {70, 200}}
};

void UpdateLine(line_control ctrl){
    static int line_choosen = 0;
    switch (ctrl){
    case CHANGE_LINE:
        line_choosen = !line_choosen;
        break;
    case MOVE_UP:
        move(&lines[line_choosen], {0, -10});
        break;
    case MOVE_RIGHT:
        move(&lines[line_choosen], {10, 0});
        break;
    case MOVE_DOWN:
        move(&lines[line_choosen], {0, 10});
        break;
    case MOVE_LEFT:
        move(&lines[line_choosen], {-10, 0});
        break;
    case BIGGER:
        scale(&lines[line_choosen], 1.1);
        break;
    case SMALLER:
        scale(&lines[line_choosen], 0.9);
        break;
    case ROTATE_CLOCKWISE:
        rotate(&lines[line_choosen], 0.174533);
        break;
    case ROTATE_COUNTERCLOCKWISE:
        rotate(&lines[line_choosen], -0.174533);
        break;
    default:
        break;
    }
}

void Update(){
    drawLine_brez(lines[0].p0.x, lines[0].p0.y, lines[0].p1.x, lines[0].p1.y);
    vec text1_pos = add(mult(add(lines[0].p0, lines[0].p1), 0.5), {0, -10});
    XDrawString(X_INFO, text1_pos.x, text1_pos.y, "Line 1", 6);
    drawLine_dda(lines[1].p0.x, lines[1].p0.y, lines[1].p1.x, lines[1].p1.y);
    vec text2_pos = add(mult(add(lines[1].p0, lines[1].p1), 0.5), {0, -10});
    XDrawString(X_INFO, text2_pos.x, text2_pos.y, "Line 2", 6);
}

int main(){
    init_x();

    while(1){
        line_control ctrl = checkEvents();
        XClearWindow(X_info.dis, X_info.win);   
        UpdateLine(ctrl);
        Update();
    }


    close_x();

    return 0;
}