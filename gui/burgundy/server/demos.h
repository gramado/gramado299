
// demos.h

#ifndef __DEMOS_H
#define __DEMOS_H  1


//
// models
//

struct cat_model_d
{
    int eyesVisible;
    int whiskersVisible;
    int mouthVisible;
    // ...
};
struct cat_model_d CatModel;


//
// == Prototypes ===============
//

void demoClearSurface(unsigned int color);
void demoFlushSurface(void);

void demos_startup_animation(int i);

void demoLines(void);
void demoLine1(void);

void demoTriangle(void);

void demoPolygon(void);
void demoPolygon2(void);

// curve+string
void demoCurve(void);

// cat
void setupCatModel(int eyes, int whiskers, int mouth );
void demoCat(void);

#endif    



