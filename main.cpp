#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>
#include <chipmunk/chipmunk.h>
#include <SDL2/SDL.h>
#include <vector>
#include <optional>
#include <memory>
#include <unordered_map>

#define SQUARE_SIZE 40.0f
#define SQUARE_SIZE_INT int(SQUARE_SIZE)
#define CHIPMUNK_SCALE 1000.0f

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

#define SDL_CALL(expr) \
    if((expr)) throw std::runtime_error("sdl call failed: " + std::string(SDL_GetError()));

#define FT_CALL(expr) \
    if(expr) throw std::runtime_error("failed to execute: " + std::string(#expr));

struct Glyph {
    uint32_t w,h;
    SDL_Texture* texture;
    FT_Vector advance;
    struct {
        unsigned int width;
        unsigned int height;
    } bitmap;
    FT_Glyph_Metrics metrics;
    FT_Int bitmap_left;
    FT_Int bitmap_top;
};

struct TextRendering {
    FT_Library lib;
    std::unordered_map<unsigned char,std::shared_ptr<Glyph>> glyphs;
};

void populateGlyphs(TextRendering& t, SDL_Renderer* r){
    FT_CALL(FT_Init_FreeType(&t.lib));

    FT_Face face;
    FT_CALL(FT_New_Face(t.lib,"../UbuntuMono-R.ttf", 0, &face));

    FT_CALL(FT_Set_Pixel_Sizes(face,0,50));

    for(uint8_t i = 1; i < UINT8_MAX; i++){

        auto glyphIndex = FT_Get_Char_Index(face,i);
        FT_CALL(FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT));
        FT_CALL(FT_Render_Glyph(face->glyph,FT_RENDER_MODE_NORMAL));

        auto& bitmap = face->glyph->bitmap;

        SDL_Rect rect = {
            0,0,
            int(bitmap.width),
            int(bitmap.rows)
        };

        if(!bitmap.width || !bitmap.rows){
            continue;
        }

        SDL_Texture* faceTexture = SDL_CreateTexture(
            r,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            rect.w,
            rect.h
        );
        if(!faceTexture){
            throw std::runtime_error("failed to create texture: " + std::string(SDL_GetError()));
        }

        auto* pixels = (uint8_t*) malloc(bitmap.width * bitmap.rows * 4);

        uint32_t j = 0, h = 0;
        while(h < (bitmap.width * bitmap.rows)){
            pixels[j++] = bitmap.buffer[h];
            pixels[j++] = bitmap.buffer[h];
            pixels[j++] = bitmap.buffer[h];
            pixels[j++] = UINT8_MAX;
            h++;
        }

        SDL_CALL(SDL_UpdateTexture(faceTexture, &rect, pixels,bitmap.width * 4));

        t.glyphs[i] = std::make_unique<Glyph>(Glyph{
            bitmap.width,
            bitmap.rows,
            faceTexture,
            face->glyph->advance,
            {
                .width = bitmap.width,
                .height = bitmap.rows,
            },
            face->glyph->metrics,
            face->glyph->bitmap_left,face->glyph->bitmap_top
        });

        free(pixels);
    }
}

void renderText(TextRendering& t, SDL_Renderer* r, std::string text){
    int destX = 0;
    int destY = 0;
    std::vector<std::shared_ptr<Glyph>> glyphs;
    int max_bitmap_top = std::numeric_limits<int>::min();
    for(const char& l : text){
        if(!t.glyphs.count(l)){
            throw std::runtime_error("no glyph found for: " + std::to_string(l));
        }

        const auto g = t.glyphs[l];

        if(g->bitmap_top > max_bitmap_top){
            max_bitmap_top = g->bitmap_top;
        }

        glyphs.emplace_back(g);
    }
    for(const std::shared_ptr<Glyph>& glyph : glyphs){
        SDL_Rect dest = {
            destX + glyph->bitmap_left,destY + max_bitmap_top - glyph->bitmap_top,
            int(glyph->w),int(glyph->h)
        };
        SDL_Rect src = {0,0,int(glyph->w),int(glyph->h)};

        SDL_RenderCopy(r,glyph->texture,&src,&dest);

        destX += glyph->advance.x / 64;
    }
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
//    cpSpaceSetIterations(space,10);

    cpFloat width = SQUARE_SIZE * CHIPMUNK_SCALE;
    cpFloat height = SQUARE_SIZE * CHIPMUNK_SCALE;

    bool isPlayable;

    for(i = 0; i < initialBoxCount; i++){
        isPlayable = i == (initialBoxCount - 1);
        x = float(double(i) * width);
        y = float(double(i) * height);

        /**
         * create playerBody
         */
        cpFloat moment = cpMomentForBox(mass,width,height);
        cpBody* playerBody = isPlayable ? cpBodyNew(mass, moment) : cpBodyNewStatic();
        cpBodySetPosition(playerBody, cpv(x, y));
        cpSpaceAddBody(space, playerBody);

        /**
         * create playerBody shape
         */
        cpShape* shape = cpSpaceAddShape(space, cpBoxShapeNew(playerBody, width, height, 0.0));
        cpShapeSetFriction(shape,1.0);
//        cpShapeSetElasticity(shape,1.0);

        auto& b = bodies.emplace_back(Body{
            .position = {x / CHIPMUNK_SCALE,y / CHIPMUNK_SCALE},
            .color = {1.0f,1.0f,1.0f,1.0f},
            .body = playerBody,
            .shape = shape
        });

        if(isPlayable){
            BodyPlayable playable{};
//            auto& controlBody = playable.controlBody;

            b.playable = playable;
        } else {
            cpBodyActivateStatic(playerBody, shape);
        }
    }

    const cpFloat dt = 1.0 / 60.0;
    SDL_Rect rect;

    TextRendering textRendering;
    populateGlyphs(textRendering,renderer);

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
                        inc[0] *= 2.0 * CHIPMUNK_SCALE;
                        inc[1] *= 2.0 * CHIPMUNK_SCALE;

                        cpBodySetVelocity(b.body,cpv(inc[0],inc[1]));
//                        cpBodyApplyImpulseAtLocalPoint(b.body,cpv(inc[0],inc[1]),cpv(0,0));
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
            cpVect vel = cpBodyGetVelocity(body.body);
            vel = vel - (vel * 0.0625 * dt);
            cpBodySetVelocity(body.body, vel);
        }
        /**
         * advance in time
         */
        cpSpaceStep(space,dt);
        /**
         * render everything
         */
        uint8_t j = 0;
        SDL_SetRenderDrawColor(renderer,200,200,200,255);
        SDL_RenderClear(renderer);
        for(auto& b : bodies){
            cpVect position = cpBodyGetPosition(b.body);
            rect = {
                .x = int(position.x / CHIPMUNK_SCALE),
                .y = int(position.y / CHIPMUNK_SCALE),
                .w = SQUARE_SIZE_INT,
                .h = SQUARE_SIZE_INT
            };
            uint8_t color[4] = {UINT8_MAX,j,UINT8_MAX,UINT8_MAX};
            SDL_SetRenderDrawColor(renderer,color[0],color[1],color[2],color[3]);
            SDL_RenderFillRect(renderer,&rect);
            j++;
        }
        renderText(textRendering,renderer,"testing");
        SDL_RenderPresent(renderer);
    }
    return 0;
}
