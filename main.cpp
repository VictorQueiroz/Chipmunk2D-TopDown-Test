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
    cpBody* controlBody;
};

struct Body {
    float position[2];
    float color[4];
    cpBody* body;
    cpShape* shape;
    std::optional<BodyPlayable> playable;
};

cpBody* createBox(cpSpace* space,double w, double h){
    double mass = 1.0;

    cpBody* body = cpSpaceAddBody(space, cpBodyNew(mass,cpMomentForBox(mass,w,h)));
//    cpBodySetPosition(body,)

    cpShape* shape = cpSpaceAddShape(space,cpBoxShapeNew(body, w, h, 0.0f));
    cpShapeSetFriction(shape,0.5);
    cpShapeSetElasticity(shape,0.0);

    return body;
}

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
    std::vector<Body> bodies{};
    unsigned int i,initialBoxCount = 6;
    float x,y,mass = 0.25f;

    /**
     * create space
     */
    cpSpace* space = cpSpaceNew();
    cpSpaceSetGravity(space,cpv(0,0));

    cpFloat width = SQUARE_SIZE * CHIPMUNK_SCALE;
    cpFloat height = SQUARE_SIZE * CHIPMUNK_SCALE;

    bool isPlayable;

    for(i = 0; i < initialBoxCount; i++){
        isPlayable = i == (initialBoxCount - 1);
        y = float(i) * SQUARE_SIZE * CHIPMUNK_SCALE;
        x = float(i) * SQUARE_SIZE * CHIPMUNK_SCALE;

        /**
         * create playerBody
         */
        cpBody* playerBody = isPlayable ? cpBodyNew(mass, INFINITY) : cpBodyNewStatic();
        cpBodySetPosition(playerBody, cpv(x, y));
        cpSpaceAddBody(space, playerBody);

        /**
         * create playerBody shape
         */
        cpShape* shape = cpBoxShapeNew(playerBody, width, height, 0.0);
        cpShapeSetFriction(shape,1.0);
        cpShapeSetElasticity(shape,0.0);

        auto& b = bodies.emplace_back(Body{
            .position = {x / CHIPMUNK_SCALE,y / CHIPMUNK_SCALE},
            .color = {1.0f,1.0f,1.0f,1.0f},
            .body = playerBody,
            .shape = shape
        });

        if(isPlayable){
            BodyPlayable playable{};
            auto& controlBody = playable.controlBody;

            /**
             * target point
             */
            controlBody = cpSpaceAddBody(space, cpBodyNewKinematic());
//            cpBodySetPosition(controlBody, cpv(x, y));

            cpConstraint* pj = cpPivotJointNew2(controlBody, playerBody, cpvzero, cpvzero);
            cpConstraintSetMaxBias(pj,0.0f * CHIPMUNK_SCALE);
            cpConstraintSetMaxForce(pj,10000.0f * CHIPMUNK_SCALE);
            cpSpaceAddConstraint(space,pj);

            cpConstraint* gj = cpGearJointNew(controlBody, playerBody, 0.0f, 1.0f);
            cpConstraintSetErrorBias(gj,0);
            cpConstraintSetMaxBias(gj,1.2f * CHIPMUNK_SCALE);
            cpConstraintSetMaxForce(gj,50000.0f * CHIPMUNK_SCALE);
            cpSpaceAddConstraint(space,gj);

            b.playable = playable;
        } else {
            cpBodyActivateStatic(playerBody, shape);
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

                        /**
                         * multiply inc by chipmunk scale
                         */
                        inc[0] *= CHIPMUNK_SCALE;
                        inc[1] *= CHIPMUNK_SCALE;

                        cpBody* targetPoint = b.playable->controlBody;
//                        cpVect vel = cpBodyGetVelocity(targetPoint);
//                        vel = cpv(inc[0],inc[1]);
                        cpBodySetVelocity(targetPoint,cpv(inc[0],inc[1]));
//
//                        if(inc[0] == 0.0f){
//                            cpVect vel = cpBodyGetVelocity(targetPoint);
//                            cpBodySetVelocity(targetPoint,cpv(0.0,vel.y));
//                        }
//
//                        if(inc[1] == 0.0f){
//                            cpVect vel = cpBodyGetVelocity(targetPoint);
//                            cpBodySetVelocity(targetPoint,cpv(vel.x,0.0));
//                        }
//
//                        if(inc[0] == 0.0f && inc[1] == 0.0f){
//                            break;
//                        }
//
//                        cpBodySetVelocity(targetPoint,cpv(inc[0],inc[1]));
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
         * update velocity of playable bodies
         */
        for(Body& body : bodies){
            if(!body.playable.has_value()) continue;
            cpVect vel = cpBodyGetVelocity(body.playable->controlBody);
            vel = vel - (vel * 0.25 * dt);
            cpBodySetVelocity(body.playable->controlBody, vel);
        }
        /**
         * advance in time
         */
        cpSpaceStep(space,dt);
        uint8_t j = 0;
        for(auto& b : bodies){
            cpVect position = cpBodyGetPosition(b.body);
            rect = {
                .x = int(position.x / CHIPMUNK_SCALE),
                .y = int(position.y / CHIPMUNK_SCALE),
                .w = SQUARE_SIZE_INT,
                .h = SQUARE_SIZE_INT
            };
            uint8_t color[4] = {255,j,255,255};
            SDL_SetRenderDrawColor(renderer,color[0],color[1],color[2],color[3]);
            SDL_RenderFillRect(renderer,&rect);
            j++;
        }
        SDL_RenderPresent(renderer);
    }
    return 0;
}
