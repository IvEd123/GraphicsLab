#include <stdio.h>
#include <math.h>
#include <time.h>
#include <iostream>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#define WIDTH 600
#define HEIGHT 600

#define NUM_OF_ASTEROIDS 10
#define MAX_NUM_OF_BULLETS 30
#define BULLET_SPEED 15
#define SPACESHIP_SPEED 15

#define max(a, b) (a) > (b) ? a : b


int counter;

/*
|==============================================================|
|============== Geometry functions and structs ================|
|==============================================================|
*/

typedef struct{
    float x, y;
} vec2;

vec2     add   ( vec2 v1,  vec2 v2  ){ return {v1.x + v2.x, v1.y + v2.y}; }
vec2     sub   ( vec2 v1,  vec2 v2  ){ return {v1.x - v2.x, v1.y - v2.y}; }
vec2     mult  ( vec2 v,   float k ){ return {v.x * k, v.y * k}; }
float   dot   ( vec2 v1,  vec2 v2  ){ return  v1.x * v2.x + v1.y * v2.y; }
float   lensq ( vec2 v            ){ return  dot(v, v); }
float   len   ( vec2 v            ){ return  sqrt(lensq(v));}
float   getcos( vec2 v1,  vec2 v2  ){ return  dot(v1, v2) / sqrt(lensq(v1) * lensq(v2));}

void    rotate(vec2* v, float angle){
    const float _cos = cos(angle);
    const float _sin = sin(angle);
    vec2 temp_v = *v;
    v->x = temp_v.x * _cos - temp_v.y * _sin;
    v->y = temp_v.x * _sin + temp_v.y * _cos;
}

#define maxv(v1, v2) (lensq)

typedef struct{
    vec2 p0, p1;
} line;

void move(line* l_p, vec2 v){
    l_p->p0 = add(l_p->p0, v);
    l_p->p1 = add(l_p->p1, v);
}

void print(vec2 v){
    std::cout << "(" << v.x << "; " << v.y << ")";
}

void scale(line* l_p, float v){
    const vec2 middle = mult(add(l_p->p1, l_p->p0), 0.5);

    l_p->p0 = sub (l_p->p0, middle);
    l_p->p0 = mult(l_p->p0, v);
    l_p->p0 = add (l_p->p0, middle);

    l_p->p1 = sub (l_p->p1, middle);
    l_p->p1 = mult(l_p->p1, v);
    l_p->p1 = add (l_p->p1, middle);
}

void rotate_tiny_angle(line* l_p, float angle){
    const vec2 middle = mult(add(l_p->p1, l_p->p0), 0.5);

    vec2* v0 = &l_p->p0;
    vec2* v1 = &l_p->p1;

    const float _sin = angle;
    const float _cos = (1 - angle * angle);

    *v0 = sub(*v0, middle);
    vec2 temp_v = *v0;
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

    const vec2 middle = mult(add(l_p->p1, l_p->p0), 0.5);

    vec2* v0 = &l_p->p0;
    vec2* v1 = &l_p->p1;

    const float _cos = cos(angle);
    const float _sin = sin(angle);

    *v0 = sub(*v0, middle);
    vec2 temp_v = *v0;
    v0->x = temp_v.x * _cos - temp_v.y * _sin;
    v0->y = temp_v.x * _sin + temp_v.y * _cos;
    *v0 = add(*v0, middle);

    *v1 = sub(*v1, middle);
    temp_v = *v1;
    v1->x = temp_v.x * _cos - temp_v.y * _sin;
    v1->y = temp_v.x * _sin + temp_v.y * _cos;
    *v1 = add(*v1, middle);

}

typedef struct{
    float x, y, z;
} vec3;

vec3     add   ( vec3 v1,  vec3 v2  ){ return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }
vec3     sub   ( vec3 v1,  vec3 v2  ){ return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}; }
vec3     mult  ( vec3 v,   float k  ){ return {v.x * k, v.y * k, v.z * k}; }
float   dot    ( vec3 v1,  vec3 v2  ){ return  v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }
float   lensq  ( vec3 v             ){ return  dot(v, v); }
float   len    ( vec3 v             ){ return  sqrt(lensq(v));}
float   getcos ( vec3 v1,  vec3 v2  ){ return  dot(v1, v2) / sqrt(lensq(v1) * lensq(v2));}

typedef struct{
    vec3 m1, m2, m3;
} mat3x3;

vec3 mult (vec3 v, mat3x3 mat){
    return {
        dot(v, mat.m1),
        dot(v, mat.m2), 
        dot(v, mat.m3)
    };
}

mat3x3 create_rotation_matrix(float angle){
    float _cos =  cos(angle);
    float _sin =  sin(angle);
    return {
        {_cos, -_sin, 0},
        {_sin,  _cos, 0},
        {   0,     0, 1}
    };
}

mat3x3 create_move_matrix(vec2 dir){
    return {
        {1, 0, dir.x},
        {0, 1, dir.y},
        {0, 0,     1}
    };
}

mat3x3 create_scale_matrix(vec2 scale){
    return {
        {scale.x,       0, 0},
        {      0, scale.y, 0},
        {      0,       0, 1}
    };
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
        0,0, WIDTH, HEIGHT, 5, white, black
    );

	XSetStandardProperties(X_info.dis, X_info.win,"Lab2", "lab2",None,NULL,0,NULL);

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

#define TARGET_FPS 60.f

typedef enum {
    DEFAULT,
    MOVE_UP,
    MOVE_RIGHT,
    MOVE_DOWN,
    MOVE_LEFT,
    BIGGER,
    SMALLER,
    ROTATE_CLOCKWISE,
    ROTATE_COUNTERCLOCKWISE,
    SHOOT
} control;

control checkEvents(){
    control ctrl = DEFAULT;
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
    //    case 't':
    //        ctrl = SHOOT;
    //        break;
        default:
            break;
        }



    }   

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
} vec2tor;

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
void drawLine_brez( line l_p){

    int x0 = l_p.p0.x;
    int y0 = l_p.p0.y;
    int x1 = l_p.p1.x;
    int y1 = l_p.p1.y;

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

    for(int i = 0; i < d[reverse]; i++){
        coords[reverse] += _s;
        coords[!reverse] += k;
        XDrawPoint(X_INFO, coords[0]>>(6*reverse), coords[1]>>(6*!reverse));
    }
}   

/*void draw_screen_borders(){
    drawLine_brez({{      0,        0 }, {WIDTH-1,        0}});
    drawLine_brez({{WIDTH-1,        0 }, {WIDTH-1, HEIGHT-1}});
    drawLine_brez({{WIDTH-1, HEIGHT-1 }, {      0, HEIGHT-1}});
    drawLine_brez({{      0, HEIGHT-1 }, {      0,        0}});
}*/

typedef struct {
    vec3 *vertices;
    vec2 scale;
    vec2 pos;
    int num_of_vertices;
    float rot;
} polygon;

polygon create_polygon(int num){
    polygon q;
    q.num_of_vertices = num;
    q.vertices = (vec3*)malloc(sizeof(vec3) * num);
    for(int i = 0; i < num; i++){
        float angle = float(i)/(float)num * 6.28;
        q.vertices[i] = { (float)cos(angle), (float)sin(angle), 1};
    }
    q.scale = {1, 1};
    q.pos = {0, 0};
    q.rot = 0;

    return q;
}   


void update_polygon(const polygon* _q){

    mat3x3 disp = create_move_matrix(_q->pos);
    mat3x3 scale = create_scale_matrix(_q->scale);
    mat3x3 rot = create_rotation_matrix(_q->rot);
    vec3 *polygon_temp = (vec3*)malloc(sizeof(vec3) * _q->num_of_vertices);
    for(int i = 0; i < _q->num_of_vertices; i++){
        polygon_temp[i] = mult(_q->vertices[i], scale);
        polygon_temp[i] = mult(polygon_temp[i], rot);
        polygon_temp[i] = mult(polygon_temp[i], disp);
    }


    for(int i = 0; i < _q->num_of_vertices; i++){
        drawLine_brez({
            {polygon_temp[i].x, polygon_temp[i].y},
            {polygon_temp[(i+1)%_q->num_of_vertices].x,polygon_temp[(i+1)%_q->num_of_vertices].y}
        });
    }
    
}

polygon Q;

/*polygon asteroids[NUM_OF_ASTEROIDS];
polygon spaceship;

int score = 0;

struct {
    polygon bullets[MAX_NUM_OF_BULLETS];
    vec2 velocity[MAX_NUM_OF_BULLETS];
    int active_bullets[MAX_NUM_OF_BULLETS];
}bullets;

void shoot(){
    int i;
    for(i = 0; i < MAX_NUM_OF_BULLETS; i++)
        if(bullets.active_bullets[i] == 0)
            break;
    if(i == MAX_NUM_OF_BULLETS){
        printf("no bullets free. wait\n");
        return;
    }
    bullets.active_bullets[i] = 1;
    bullets.bullets[i].pos = spaceship.pos;
    bullets.velocity[i] = {0, -1};
    bullets.velocity[i] = mult(bullets.velocity[i], BULLET_SPEED);
    rotate(&bullets.velocity[i], spaceship.rot+1.57);
}

void init_bullets(){
    for(int i = 0; i < MAX_NUM_OF_BULLETS; i++){
        bullets.bullets[i] = create_polygon(1);
        bullets.bullets[i].scale = {1, 1};
    }
}

void update_bullets(){
    for(int i = 0; i < MAX_NUM_OF_BULLETS; i++){
        if(!bullets.active_bullets[i])
            continue;
        bullets.bullets[i].pos = add(bullets.bullets[i].pos, bullets.velocity[i]);
        update_polygon(&bullets.bullets[i]);
        if(bullets.bullets[i].pos.x > WIDTH || bullets.bullets[i].pos.x < 0 || bullets.bullets[i].pos.y > HEIGHT || bullets.bullets[i].pos.y < 0){
            bullets.active_bullets[i] = 0;
        }
    }
}

void update_asteroids(){
    for(int i = 0; i < NUM_OF_ASTEROIDS; i++){
        asteroids[i].pos = add(asteroids[i].pos, {0, 1});
        if(asteroids[i].pos.y > HEIGHT){
            asteroids[i].pos.y = -20;
            score -= 5;
        }
        update_polygon(&asteroids[i]);
        for(int j = 0; j < MAX_NUM_OF_BULLETS; j++){
            if(!bullets.active_bullets[j])
                continue;
            if(lensq(sub(asteroids[i].pos, bullets.bullets[j].pos)) <= 100){
                bullets.active_bullets[i] = 0;
                asteroids[i].pos.y = -20;
                score += 10;
            }

        }
    }
}*/

void control_polygon(polygon* _q, control ctrl){
    switch (ctrl){
    case MOVE_UP:
        _q->pos = add(_q->pos, {0, -SPACESHIP_SPEED});
        break;
    case MOVE_RIGHT:
        _q->pos = add(_q->pos, {SPACESHIP_SPEED, 0});
        break;
    case MOVE_DOWN:
        _q->pos = add(_q->pos, {0, SPACESHIP_SPEED});
        break;
    case MOVE_LEFT:
        _q->pos = add(_q->pos, {-SPACESHIP_SPEED, 0});
        break;
    case BIGGER:
        _q->scale = mult(_q->scale, 1.1);
        break;
    case SMALLER:
        _q->scale = mult(_q->scale, 0.9);
        break;
    case ROTATE_CLOCKWISE:
        _q->rot += 0.34;
        break;
    case ROTATE_COUNTERCLOCKWISE:
        _q->rot -= 0.34;
        break;
//    case SHOOT:
//       shoot();
    default:
        break;
    }
}


void Update(){
    //draw_screen_borders();
    update_polygon(&Q);
    //update_bullets();
    //update_asteroids();

    //char str_score[20];
    //sprintf(str_score, "score: %d\0", score);
    //XDrawString(X_INFO, 500, 10, str_score, strlen(str_score));
}

int main(){
    init_x();
    srand(time(NULL));

    unsigned char num_of_vertices;
    printf("Enter num of vertices: ");
    scanf("%d", &num_of_vertices);
    

    Q = create_polygon(num_of_vertices);
    Q.pos = {WIDTH / 2, HEIGHT / 2};
    Q.scale = {50, 50};

    while(1){
        control ctrl = checkEvents();
        XClearWindow(X_info.dis, X_info.win); 
        control_polygon(&Q, ctrl);
        Update();
    }

    /*spaceship = create_polygon(3);
    spaceship.pos = {WIDTH/2, HEIGHT-100};
    spaceship.rot = -1.52;
    spaceship.scale = {20, 5};  
    init_bullets();

    for(int i = 0; i < NUM_OF_ASTEROIDS; i++){
        asteroids[i] = create_polygon(6);
        asteroids[i].pos = {float(rand()%WIDTH), -i*50.f};
        asteroids[i].rot = i;
        asteroids[i].scale = {10, 10};  
    }

    while(1){
        control ctrl = checkEvents();
        XClearWindow(X_info.dis, X_info.win); 
        control_polygon(&spaceship, ctrl);
        Update();
    }*/


    close_x();

    return 0;
}