#include <iostream>
#include <chipmunk/chipmunk.h>
#include <chipmunk/chipmunk_structs.h>
#include <SDL2/SDL.h>
#include <vector>
#include <optional>

#define SQUARE_SIZE 40.0f
#define SQUARE_SIZE_INT int(SQUARE_SIZE)
#define CHIPMUNK_SCALE 10.0f

struct BodyPlayable{
    cpBody* targetPoint;
};

struct Body {
    float position[2];
    float color[4];
    cpBody* body;
    cpShape* shape;
    std::optional<BodyPlayable> playable;
};

int main() {
    SDL_Init(SDL_INIT_EVERYTHING);
    auto* w = SDL_CreateWindow(
        "Test",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        800,600,0
    );
    SDL_GL_SetSwapInterval(0);
    auto* renderer = SDL_CreateRenderer(w,0,SDL_RENDERER_ACCELERATED);
    bool running = true;
    SDL_Event e;
    cpSpace* space = cpSpaceNew();
    std::vector<Body> bodies{};
    int initialBoxCount = 6;
    float x,y,mass = 0.25f;

    cpFloat width = SQUARE_SIZE * CHIPMUNK_SCALE;
    cpFloat height = SQUARE_SIZE * CHIPMUNK_SCALE;
    cpBody* rootAnchorBody = cpBodyNew(mass, cpMomentForBox(mass,width,height));
    {
        cpShape* rootAnchorShape = cpBoxShapeNew(rootAnchorBody,width,height,0.0f);
        cpShapeSetFriction(rootAnchorShape,1.0f);
        cpShapeSetDensity(rootAnchorShape,1.0f);
        cpBodySetPosition(rootAnchorBody,cpvzero);
        cpSpaceAddBody(space,rootAnchorBody);
        cpSpaceAddShape(space,rootAnchorShape);
    }

    for(int i = 0; i < initialBoxCount; i++){
        cpFloat moment = cpMomentForBox(mass, width, height);
        bool isPlayable = i == (initialBoxCount - 1);
        cpBody* body = isPlayable ? cpBodyNew(mass,moment) : cpBodyNewStatic();

        /**
         * create body shape
         */
        cpShape* shape = cpBoxShapeNew(body, width, height, 0.0f);
        cpShapeSetFriction(shape,10000.0f);

        y = float(i) * SQUARE_SIZE;
        x = float(i) * SQUARE_SIZE;
        cpBodySetPosition(body,cpv(x * CHIPMUNK_SCALE,y * CHIPMUNK_SCALE));
        cpSpaceAddBody(space,body);
        auto& b = bodies.emplace_back(Body{
            .position = {x,y},
            .color = {1.0f,1.0f,1.0f,1.0f},
            .body = body,
            .shape = shape
        });
        if(isPlayable){
            BodyPlayable playable{};
//            playable.targetPoint = cpBodyNew(
//                mass,
//                cpMomentForBox(mass,CHIPMUNK_SCALE * SQUARE_SIZE,CHIPMUNK_SCALE * CHIPMUNK_SCALE)
//            );
//            cpBodySetPosition(playable.targetPoint,cpv(x,y));
//
//            cpConstraint* pj = cpPivotJointNew(playable.targetPoint,body,cpvzero);
//            pj->maxBias = 200.0f;
//            pj->maxForce = 1000.0f;
//            cpSpaceAddConstraint(space,pj);
//
//            cpConstraint* gj = cpGearJointNew(playable.targetPoint,body,0.0f,1.0f);
//            cpSpaceAddConstraint(space,gj);

            b.playable = playable;
            cpSpaceAddShape(space,shape);
        } else {
            cpBodyActivateStatic(body,shape);
            cpSpaceAddShape(space,shape);
        }
    }

    const cpFloat dt = 1.0f / 60.0f;
    SDL_Rect rect;

    while(running) {
        while(SDL_PollEvent(&e)){
            switch(e.type){
                case SDL_KEYDOWN: {
                    float inc[2] = {
                        0.0f,
                        0.0f
                    };
                    switch(e.key.keysym.sym){
                        case SDLK_w:
                            inc[1] -= 1.0f;
                            break;
                        case SDLK_s:
                            inc[1] += 1.0f;
                            break;
                        case SDLK_a:
                            inc[0] -= 1.0f;
                            break;
                        case SDLK_d:
                            inc[0] += 1.0f;
                            break;
                    }
                    for(auto& b : bodies){
                        if(!b.playable.has_value()){
                            continue;
                        }
//                        b.position[0] += inc[0] * 2.0f;
//                        b.position[1] += inc[1] * 2.0f;
//                        inc[0] *= 100.0f;
//                        inc[1] *= 100.0f;
                        inc[0] *= CHIPMUNK_SCALE;
                        inc[1] *= CHIPMUNK_SCALE;
//                        if(inc[0] == 0.0f && inc[1] == 0.0f){
//                            cpBodySetVelocity(b.body,cpv(0,0));
//                            break;
//                        }
//                        cpBodySetVelocity(b.body,cpv(inc[0],inc[1]));
                        cpBodyApplyImpulseAtLocalPoint(b.body,cpv(inc[0],inc[1]),cpv(0.0f,0.0f));
//                        cpBodyApplyForceAtLocalPoint(b.body,cpv(-100.0f,-100.0f),cpv(0.0f,0.0f));)
                        break;
                    }
                    break;
                }
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_WINDOWEVENT:
                    if(e.window.type == SDL_WINDOWEVENT_CLOSE){
                        running = false;
                    }
                    break;
            }
        }
        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderClear(renderer);
        /**
         * update physics engine
         */
//        for(auto& b : bodies){
//            cpBodySetPosition(b.body,cpv(b.position[0],b.position[1]));
//        }
        /**
         * advance in time
         */
        cpSpaceStep(space,dt);
        for(auto& b : bodies){
            cpVect position = cpBodyGetPosition(b.body);
            rect = {
                .x = int(position.x / CHIPMUNK_SCALE),
                .y = int(position.y / CHIPMUNK_SCALE),
                .w = SQUARE_SIZE_INT,
                .h = SQUARE_SIZE_INT
            };
            SDL_SetRenderDrawColor(renderer,255,255,255,255);
            SDL_RenderFillRect(renderer,&rect);
            b.body->w *= .99f;
        }
        SDL_RenderPresent(renderer);
    }
    return 0;
}
