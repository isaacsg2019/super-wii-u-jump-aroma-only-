#include <SDL2/SDL.h>
#include <vpad/input.h>
#include <string.h>
#include <stdio.h>

#define SCREEN_W 1280
#define SCREEN_H 720
#define GROUND_Y 588
#define LEVEL_W 5900
#define PLAYER_W 48
#define PLAYER_H 96
#define SPRITE_COUNT 7
#define COIN_COUNT 18

/* Tiny built-in 5x7 font: no SDL_ttf or extra assets needed. */
static const char *glyph(char c)
{
    switch (c) {
    case 'A': return "01110100011000111111100011000110001";
    case 'B': return "11110100011000111110100011000111110";
    case 'C': return "01111100001000010000100001000001111";
    case 'D': return "11110100011000110001100011000111110";
    case 'E': return "11111100001000011110100001000011111";
    case 'F': return "11111100001000011110100001000010000";
    case 'G': return "01111100001000010111100011000101111";
    case 'H': return "10001100011000111111100011000110001";
    case 'I': return "11111001000010000100001000010011111";
    case 'J': return "00111000100001000010100101001001100";
    case 'K': return "10001100101010011000101001001010001";
    case 'L': return "10000100001000010000100001000011111";
    case 'M': return "10001110111010110101100011000110001";
    case 'N': return "10001110011010110011100011000110001";
    case 'O': return "01110100011000110001100011000101110";
    case 'P': return "11110100011000111110100001000010000";
    case 'Q': return "01110100011000110001101011001001101";
    case 'R': return "11110100011000111110101001001010001";
    case 'S': return "01111100001000001110000010000111110";
    case 'T': return "11111001000010000100001000010000100";
    case 'U': return "10001100011000110001100011000101110";
    case 'V': return "10001100011000110001100010101000100";
    case 'W': return "10001100011000110101101011101110001";
    case 'X': return "10001100010101000100010101000110001";
    case 'Y': return "10001100010101000100001000010000100";
    case 'Z': return "11111000010001000100010001000011111";
    case '0': return "01110100011001110101110011000101110";
    case '1': return "00100011000010000100001000010001110";
    case '2': return "01110100010000100010001000100011111";
    case '3': return "11110000010000101110000010000111110";
    case '4': return "00010001100101010010111110001000010";
    case '5': return "11111100001000011110000010000111110";
    case '6': return "01111100001000011110100011000101110";
    case '7': return "11111000010001000100010000100001000";
    case '8': return "01110100011000101110100011000101110";
    case '9': return "01110100011000101111000010000111110";
    case '-': return "00000000000000011111000000000000000";
    case ':': return "00000001000010000000001000010000000";
    case '?': return "01110100010001000100000000000000100";
    case '!': return "00100001000010000100001000000000100";
    case '.': return "00000000000000000000000000000000100";
    default: return NULL;
    }
}

static void fill(SDL_Renderer *r, int x, int y, int w, int h,
                 Uint8 red, Uint8 green, Uint8 blue)
{
    SDL_Rect rect = {x, y, w, h};
    SDL_SetRenderDrawColor(r, red, green, blue, 255);
    SDL_RenderFillRect(r, &rect);
}

static void drawText(SDL_Renderer *r, const char *s, int x, int y, int scale)
{
    for (; *s; s++, x += 6 * scale) {
        const char *g = glyph(*s);
        if (!g) continue;
        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 5; col++) {
                if (g[row * 5 + col] == '1')
                    fill(r, x + col * scale, y + row * scale,
                         scale, scale, 255, 255, 255);
            }
        }
    }
}


/* Lightweight per-world atmosphere. Visual only: no physics or hitboxes changed.
   Uses deterministic particles, no textures or extra files. */
static int polishWorldForScene(int scene)
{
    if (scene == 1 || scene == 4) return 1;
    if (scene == 11 || scene == 12) return 2;
    if (scene == 22 || scene == 23 || scene == 26) return 3;
    if (scene == 40 || scene == 43) return 4;
    if (scene == 53) return 5;
    if (scene == 63) return 6;
    return 0;
}

static void drawWorldAtmosphere(SDL_Renderer *r, int world, Uint32 ticks)
{
    if (world < 1 || world > 6) return;
    const Uint8 palette[6][3] = {
        {255, 240, 164}, /* World 1: sunlight glints */
        {133, 226, 255}, /* World 2: blue bubbles */
        {255, 156,  77}, /* World 3: drifting embers */
        {255, 208, 134}, /* World 4: celebratory sparks */
        {189, 228, 204}, /* World 5: soft floating motes */
        {255,  79, 137}  /* World 6: corrupted data */
    };
    Uint32 step = ticks / 32;
    SDL_BlendMode previousMode = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(r, &previousMode);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < 27; ++i) {
        unsigned int seed = (unsigned int)(i * 7919u + world * 19997u);
        int x = (int)((seed * 37u + step * (unsigned int)(world == 6 ? 3 : 1)) % SCREEN_W);
        int y = (int)((seed * 13u + (world == 3 ? (720u - step % 720u) : step / 2u)) % 490u) + 72;
        int size = world == 2 ? 4 : (world == 6 ? 3 : 2);
        Uint8 alpha = (Uint8)(55 + ((seed + ticks / 90u) % 75u));
        if (world == 6 && ((ticks / 180u + (Uint32)i) % 7u == 0u)) alpha = 0;
        SDL_SetRenderDrawColor(r, palette[world-1][0], palette[world-1][1],
                               palette[world-1][2], alpha);
        SDL_Rect mote = {x, y, size, size};
        SDL_RenderFillRect(r, &mote);
        if (world == 2) {
            SDL_Rect bubble = {x-2, y-2, 8, 8};
            SDL_RenderDrawRect(r, &bubble);
        } else if (world == 6 && (i % 4 == 0)) {
            SDL_Rect glitch = {x, y, 11, 2};
            SDL_RenderFillRect(r, &glitch);
        }
    }
    SDL_SetRenderDrawBlendMode(r, previousMode);
}

/* Lives HUD shared by both courses. */
static void drawLives(SDL_Renderer *r, int lives)
{
    char message[32];
    SDL_snprintf(message, sizeof(message), "LIVES: %d", lives);
    fill(r, SCREEN_W-246, 12, 234, 54, 33, 74, 122);
    drawText(r, message, SCREEN_W-229, 28, 3);
}

static void centerText(SDL_Renderer *r, const char *s, int y, int scale)
{
    int width = (int)strlen(s) * 6 * scale;
    drawText(r, s, (SCREEN_W - width) / 2, y, scale);
}

/* Expanded course based on Isaac's original hand-drawn level. */
static const SDL_Rect platforms[] = {
    {350,475,76,34}, {590,465,76,34}, {790,500,76,34},
    {790,405,76,34}, {910,370,76,34}, {1180,470,76,34},
    {1600,470,76,34},
    /* New extended course: rising blocks, bridge and final challenge. */
    {3040,472,100,32}, {3210,410,100,32}, {3380,468,100,32},
    {3580,380,100,32}, {3760,445,110,32}, {3970,485,92,32},
    {4170,420,96,32}, {4370,362,96,32}, {4580,435,96,32},
    {4790,480,96,32}, {5030,420,96,32}, {5260,390,96,32}
};
#define PLATFORM_COUNT ((int)(sizeof(platforms)/sizeof(platforms[0])))
static const SDL_Point coinPositions[COIN_COUNT] = {
    {380,430}, {625,425}, {825,360}, {942,330},
    {1215,430}, {1630,430}, {2080,455}, {2440,470},
    {3070,430}, {3240,370}, {3420,425}, {3610,340},
    {3800,405}, {4000,440}, {4210,380}, {4410,320},
    {4830,440}, {5290,350}
};

static void pixelCloud(SDL_Renderer *r, int x, int y)
{
    fill(r,x,y+28,140,26,245,250,255);
    fill(r,x+20,y+8,45,46,245,250,255);
    fill(r,x+62,y,56,54,245,250,255);
    fill(r,x+111,y+17,33,38,245,250,255);
}

static void enemy(SDL_Renderer *r,int x,int y)
{
    /* Small original brown walking critter. */
    fill(r,x+6,y+9,40,28,147,91,56);
    fill(r,x+12,y+2,28,13,147,91,56);
    fill(r,x+3,y+35,19,10,75,61,58);
    fill(r,x+29,y+35,19,10,75,61,58);
    fill(r,x+15,y+19,7,12,255,242,217);
    fill(r,x+31,y+19,7,12,255,242,217);
    fill(r,x+18,y+25,3,6,29,35,49);
    fill(r,x+33,y+25,3,6,29,35,49);
}

static void brick(SDL_Renderer *r,int x,int y,int w,int h)
{
    fill(r,x,y,w,h,188,113,57);
    fill(r,x,y,w,7,236,162,83);
    fill(r,x+5,y+12,w-10,4,124,71,49);
    fill(r,x+w/2,y+17,5,h-19,124,71,49);
}

/* Remove pink/magenta anti-alias fringe before SDL's exact color-key cutout.
   Converted to ARGB8888 so pixel access is reliable across BMP formats. */
static SDL_Surface *cleanPink(SDL_Surface *input)
{
    if (!input) return NULL;
    SDL_Surface *s=SDL_ConvertSurfaceFormat(input,SDL_PIXELFORMAT_ARGB8888,0);
    SDL_FreeSurface(input);
    if (!s) return NULL;
    if (SDL_MUSTLOCK(s) && SDL_LockSurface(s)!=0) return s;
    for (int y=0;y<s->h;y++) {
        Uint32 *row=(Uint32*)((Uint8*)s->pixels+y*s->pitch);
        for (int x=0;x<s->w;x++) {
            Uint8 red,green,blue;
            SDL_GetRGB(row[x],s->format,&red,&green,&blue);
            if (red>155 && blue>145 && green<130 &&
                red>green+55 && blue>green+55)
                row[x]=SDL_MapRGB(s->format,255,0,255);
        }
    }
    if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(s);
    return s;
}

/* Four-frame run cycle advances every four gameplay frames (roughly 15 FPS at 60 Hz). */
/* Visual-only character animation polish; does not change physics or hitboxes.
   Applies to both Mii and Undercover, using Isaac's existing seven BMP poses. */
/* Night title screen: deterministic pixel stars, moon, clouds and hills. */
static void drawNightTitle(SDL_Renderer *r)
{
    SDL_SetRenderDrawColor(r, 8, 15, 49, 255);
    SDL_RenderClear(r);
    for (int i=0; i<115; ++i) {
        int x=(i*317+53)%SCREEN_W;
        int y=(i*179+19)%450;
        int twinkle=((SDL_GetTicks()/650+i)%7==0);
        fill(r,x,y,twinkle?3:2,twinkle?3:2,170,193,234);
    }
    /* Pixel moon */
    fill(r,1115,61,79,74,194,210,242);
    fill(r,1125,49,59,97,194,210,242);
    fill(r,1105,72,99,50,194,210,242);
    fill(r,1134,76,19,16,154,175,218);
    fill(r,1167,105,15,13,154,175,218);
    fill(r,1156,61,11,10,154,175,218);
    /* Distant night clouds */
    fill(r,90,133,161,16,29,45,101);
    fill(r,132,114,70,22,29,45,101);
    fill(r,1000,240,186,15,29,45,101);
    fill(r,1054,219,80,24,29,45,101);
    for (int x=0;x<SCREEN_W;x+=64) {
        int h=24+((x*13/64)%5)*11;
        fill(r,x,480-h,68,h,4,18,46);
    }
    fill(r,0,480,SCREEN_W,240,7,70,51);
    fill(r,0,480,SCREEN_W,15,56,174,77);
}

/* Make a seated menu pose from the character's own idle sprite.
   Cropping preserves the actual character art without requiring new assets. */
static void drawSeatedTitleMii(SDL_Renderer *r, SDL_Texture *idle, int x, int groundY)
{
    if (!idle) {
        fill(r,x+9,groundY-47,34,31,53,165,77);
        fill(r,x+10,groundY-17,51,17,43,42,54);
        return;
    }
    int w=0,h=0;
    if (SDL_QueryTexture(idle,NULL,NULL,&w,&h)!=0 || w<=0 || h<=0) return;
    /* Head and torso upright, legs projected forward to show sitting. */
    SDL_Rect upperSrc={0,0,w,h*72/100};
    SDL_Rect upperDst={x+10,groundY-74,46,65};
    SDL_RenderCopy(r,idle,&upperSrc,&upperDst);
    SDL_Rect legsSrc={0,h*72/100,w,h-h*72/100};
    SDL_Rect legsDst={x+9,groundY-23,67,23};
    SDL_RenderCopy(r,idle,&legsSrc,&legsDst);
}

static void drawPlayer(SDL_Renderer *renderer, SDL_Texture *sprites[],
                       int spriteIndex, float x, float y, int facingRight, int camera)
{
    static Uint32 lastDraw[2] = {0, 0};
    static int lastPose[2] = {0, 0};
    static Uint32 landingUntil[2] = {0, 0};
    /* Sprite arrays are stable for the life of the game; remember each separately. */
    static SDL_Texture **firstSet = NULL;
    int character = 0;
    if (!firstSet) firstSet = sprites;
    if (sprites != firstSet) character = 1;
    Uint32 now = SDL_GetTicks();
    if (lastPose[character] >= 5 && spriteIndex <= 4 &&
        now - lastDraw[character] < 160)
        landingUntil[character] = now + 110;
    lastPose[character] = spriteIndex;
    lastDraw[character] = now;

    SDL_Rect dst = {(int)x - camera, (int)y, PLAYER_W, PLAYER_H};
    if (spriteIndex == 0) {
        /* Slow two-pixel breathing animation when standing still. */
        int breathe = (int)((now / 260) % 8);
        if (breathe >= 4) breathe = 7 - breathe;
        dst.y += breathe / 2;
        dst.h -= breathe / 2;
    } else if (spriteIndex >= 1 && spriteIndex <= 4) {
        /* Footstep bounce in sync with the four running frames. */
        if (spriteIndex == 2 || spriteIndex == 4) dst.y -= 2;
    } else if (spriteIndex == 5) {
        /* Stretch on ascent, anchored at the feet. */
        dst.y -= 5; dst.h += 5; dst.w -= 3; dst.x += 1;
    } else if (spriteIndex == 6) {
        /* Slight widening while falling. */
        dst.w += 3; dst.x -= 1; dst.h -= 2;
    }
    if (spriteIndex <= 4 && now < landingUntil[character]) {
        /* Quick landing squash, preserving the player's feet position. */
        int shrink = 4;
        dst.y += shrink; dst.h -= shrink; dst.w += 4; dst.x -= 2;
    }
    if (sprites[spriteIndex]) {
        SDL_RenderCopyEx(renderer, sprites[spriteIndex], NULL, &dst,
                         0.0, NULL,
                         facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);
    } else {
        fill(renderer, dst.x, dst.y, dst.w, dst.h, 0, 220, 80);
    }
}

static void drawWorld(SDL_Renderer *r, int camera, const int taken[], int coins)
{
    SDL_SetRenderDrawColor(r,109,188,249,255);
    SDL_RenderClear(r);
    for (int i=0;i<12;i++) {
        int cx=i*300-(camera/5);
        pixelCloud(r,cx,85+(i%3)*45);
    }
    /* Layered distant hills give the retro landscape depth. */
    for (int i=0;i<14;i++) {
        int hx=i*270-camera/3;
        fill(r,hx,456+(i%2)*30,220,130,109,197,135);
        fill(r,hx+60,420+(i%2)*30,100,70,109,197,135);
        fill(r,hx+30,495,220,90,80,168,115);
    }
    fill(r,0,GROUND_Y,SCREEN_W,SCREEN_H-GROUND_Y,153,97,53);
    fill(r,0,GROUND_Y,SCREEN_W,19,72,211,83);
    for (int i=0;i<LEVEL_W/64;i++) {
        int gx=i*64-camera;
        if(gx>-70 && gx<SCREEN_W) {
            fill(r,gx+9,630+(i%2)*32,29,9,116,72,45);
            fill(r,gx+31,GROUND_Y-7,8,7,54,166,69);
        }
    }
    /* Start arrow drawn on the sketch. */
    int sx=85-camera;
    fill(r,sx,GROUND_Y-106,8,106,106,73,48);
    fill(r,sx-21,GROUND_Y-155,116,53,244,221,146);
    fill(r,sx-17,GROUND_Y-151,108,45,140,92,47);
    drawText(r,"GO",sx+5,GROUND_Y-139,4);
    fill(r,sx+58,GROUND_Y-126,26,10,255,233,144);
    fill(r,sx+75,GROUND_Y-135,12,27,255,233,144);

    for(int i=0;i<PLATFORM_COUNT;i++) {
        SDL_Rect p=platforms[i];
        int px=p.x-camera;
        if(px+p.w<0 || px>SCREEN_W) continue;
        brick(r,px,p.y,p.w,p.h);
    }
    /* Original question-mark-styled special blocks on the sketch. */
    int qx=790-camera;
    fill(r,qx+3,410,70,24,235,185,57);
    drawText(r,"?",qx+25,413,3);


    /* Friendly collectible coins. */
    for(int i=0;i<COIN_COUNT;i++) {
        if(taken[i])continue;
        int cx=coinPositions[i].x-camera;
        int cy=coinPositions[i].y;
        fill(r,cx-10,cy-14,20,28,240,161,26);
        fill(r,cx-5,cy-10,10,20,255,230,75);
        fill(r,cx-1,cy-7,3,14,240,161,26);
    }

    /* Original walking critter before the big obstacle. */
    enemy(r,1090-camera,GROUND_Y-45);

    /* Big rectangular obstacle with a hidden panel, as sketched. */
    brick(r,1790-camera,GROUND_Y-132,175,132);
    fill(r,1751-camera,GROUND_Y-108,37,55,253,219,115);
    fill(r,1756-camera,GROUND_Y-103,27,45,154,95,65);

    /* Another small character appears in the sketch near the end. */
    enemy(r,2260-camera,GROUND_Y-45);
    enemy(r,3500-camera,GROUND_Y-45);
    enemy(r,4700-camera,GROUND_Y-45);

    /* Short staircase leading to the level's finishing flag. */
    for(int i=0;i<5;i++) {
        int stepX=2520+i*50-camera;
        int stepH=(i+1)*30;
        brick(r,stepX,GROUND_Y-stepH,50,stepH);
    }
    /* New scenery and obstacles in the extended half of the course. */
    for (int i=0; i<4; ++i) {
        int treeX=3030+i*660-camera;
        fill(r,treeX,GROUND_Y-130,20,130,116,77,47);
        fill(r,treeX-43,GROUND_Y-203,105,90,45,155,78);
        fill(r,treeX-23,GROUND_Y-239,65,55,65,183,92);
    }
    for (int i=0; i<3; ++i) {
        int pillarX=3860+i*470-camera;
        brick(r,pillarX,GROUND_Y-72,72,72);
    }
    /* End approach: short, climbable step sequence. */
    for (int i=0; i<4; ++i) {
        int stepX=5450+i*48-camera;
        int stepH=(i+1)*34;
        brick(r,stepX,GROUND_Y-stepH,48,stepH);
    }
    int flagX=5740-camera;
    fill(r,flagX,GROUND_Y-310,8,310,240,239,228);
    fill(r,flagX+8,GROUND_Y-307,76,46,231,77,93);
    fill(r,flagX+22,GROUND_Y-295,25,11,255,230,142);
    fill(r,flagX-16,GROUND_Y-16,44,16,165,164,151);

    fill(r,10,12,360,58,33,74,122);
    drawText(r,"WORLD 1-1",24,27,3);
    char counter[32];
    SDL_snprintf(counter,sizeof(counter),"COINS: %d",coins);
    drawText(r,counter,24,85,3);
}


/* WORLD 1-2: the Nintendo Land-inspired 10,000-pixel boss course.
   The boss is vulnerable ONLY when the player throws a bomb upward from below. */
#define LEVEL2_W 10000
#define BOSS_X 9320
#define BLOCK_X 2400
#define LASER_X 6550
#define BOSS_SHOT_INTERVAL 3000u  /* milliseconds */
#define BOSS_LASER_SPEED 9.0f
#define BOSS_LASER_COUNT 8
#define BOSS_ARENA_START (BOSS_X - 900)

typedef struct {
    float x, y, vx, vy;
    int active;
} BossLaser;

static void clearBossLasers(BossLaser shots[BOSS_LASER_COUNT])
{
    memset(shots, 0, sizeof(BossLaser) * BOSS_LASER_COUNT);
}

static void spawnBossLaser(BossLaser shots[BOSS_LASER_COUNT],
                           float targetX, float targetY)
{
    /* Aim at the player's position when the shot is fired. */
    float sx = BOSS_X - 25.0f;
    float sy = 275.0f;
    float dx = targetX - sx, dy = targetY - sy;
    float largest = dx < 0 ? -dx : dx;
    float absY = dy < 0 ? -dy : dy;
    if (absY > largest) largest = absY;
    if (largest < 1.0f) largest = 1.0f;
    for (int i = 0; i < BOSS_LASER_COUNT; ++i) {
        if (!shots[i].active) {
            shots[i].x = sx;
            shots[i].y = sy;
            shots[i].vx = BOSS_LASER_SPEED * dx / largest;
            shots[i].vy = BOSS_LASER_SPEED * dy / largest;
            shots[i].active = 1;
            break;
        }
    }
}

static int updateBossLasers(BossLaser shots[BOSS_LASER_COUNT],
                            float px, float py)
{
    for (int i = 0; i < BOSS_LASER_COUNT; ++i) {
        BossLaser *b = &shots[i];
        if (!b->active) continue;
        b->x += b->vx;
        b->y += b->vy;
        if (b->x < BOSS_ARENA_START - 100 || b->x > LEVEL2_W + 50 ||
            b->y < -30 || b->y > SCREEN_H + 30) {
            b->active = 0;
            continue;
        }
        /* Player coordinates and projectile coordinates are both world-space. */
        if (b->x + 20 > px && b->x < px + PLAYER_W &&
            b->y + 12 > py && b->y < py + PLAYER_H) {
            b->active = 0;
            return 1;
        }
    }
    return 0;
}

static void drawBossLasers(SDL_Renderer *r,
                           BossLaser shots[BOSS_LASER_COUNT], int camera)
{
    for (int i = 0; i < BOSS_LASER_COUNT; ++i) {
        if (!shots[i].active) continue;
        int sx = (int)shots[i].x - camera;
        int sy = (int)shots[i].y;
        fill(r, sx-4, sy-3, 28, 18, 18, 94, 223);
        fill(r, sx, sy, 20, 12, 32, 198, 255);
        fill(r, sx+3, sy+3, 14, 6, 245, 253, 255);
    }
}

static void drawBomb(SDL_Renderer *r, int bx, int by)
{
    fill(r,bx+5,by+4,24,24,35,37,45);
    fill(r,bx+11,by+0,12,7,99,99,100);
    fill(r,bx+8,by+10,6,6,107,113,124);
    fill(r,bx+17,by-5,5,7,242,174,52);
}

static void drawSSD(SDL_Renderer *r, SDL_Texture *ssd, int xx, int yy)
{
    if (ssd) {
        SDL_Rect dst={xx,yy,128,88};
        SDL_RenderCopy(r,ssd,NULL,&dst);
    } else {
        fill(r,xx,yy,128,88,38,42,50);
        drawText(r,"SSD",xx+24,yy+31,4);
    }
}

static void drawController(SDL_Renderer *r, int xx, int yy, int defeated)
{
    /* A stylized, original pixel-art Pro-style controller silhouette. */
    Uint8 shade=defeated?91:38;
    fill(r,xx+36,yy+18,196,90,shade,shade,shade+9);
    fill(r,xx+5,yy+47,75,111,shade,shade,shade+9);
    fill(r,xx+185,yy+47,75,111,shade,shade,shade+9);
    fill(r,xx+60,yy+32,150,18,76,79,91);
    fill(r,xx+36,yy+77,50,14,103,107,117);
    fill(r,xx+187,yy+80,51,14,103,107,117);
    /* D-pad and face buttons */
    fill(r,xx+72,yy+89,38,13,143,146,157);
    fill(r,xx+84,yy+77,13,38,143,146,157);
    fill(r,xx+180,yy+63,12,12,246,88,85);
    fill(r,xx+206,yy+79,12,12,89,169,246);
    fill(r,xx+180,yy+96,12,12,93,207,118);
    fill(r,xx+156,yy+79,12,12,252,214,86);
    /* Glowing weak point underneath the controller. */
    if (!defeated) {
        fill(r,xx+114,yy+133,38,13,255,92,81);
        fill(r,xx+123,yy+136,20,7,255,232,80);
    }
}

static void drawLevel2(SDL_Renderer *r, int camera, int bombReady,
                       int blockUsed, int bossDefeated, int ssdCollected,
                       float flyingBombY, int bombFlying, SDL_Texture *ssd,
                        SDL_Texture *park, SDL_Texture *bossSprites[6], int bossShotFlash)
{
    SDL_SetRenderDrawColor(r,105,193,234,255);
    SDL_RenderClear(r);
    for (int i=0;i<45;i++) {
        int xx=i*300-camera/4;
        pixelCloud(r,xx,85+(i%4)*45);
    }
    /* Nintendo Land-inspired theme park repeats in parallax behind the level. */
    if (park) {
        for (int i=0;i<22;i++) {
            SDL_Rect dst={i*900-camera/4,100,900,490};
            if (dst.x < SCREEN_W && dst.x+dst.w > 0)
                SDL_RenderCopy(r,park,NULL,&dst);
        }
    } else {
        for (int i=0;i<44;i++) {
            int xx=i*260-camera/3;
            fill(r,xx,448+(i%3)*15,200,145,117,202,170);
            fill(r,xx+40,417+(i%3)*15,92,55,117,202,170);
        }
    }
    fill(r,0,GROUND_Y,SCREEN_W,SCREEN_H-GROUND_Y,125,100,81);
    fill(r,0,GROUND_Y,SCREEN_W,17,76,217,132);
    for(int i=0;i<LEVEL2_W/80;i++) {
        int xx=i*80-camera;
        if (xx>-80 && xx<SCREEN_W) fill(r,xx+24,630+(i%2)*33,30,7,93,76,65);
    }
    int sign=100-camera;
    fill(r,sign,GROUND_Y-112,8,112,85,59,45);
    fill(r,sign-20,GROUND_Y-158,133,55,239,203,114);
    fill(r,sign-16,GROUND_Y-154,125,47,105,100,144);
    drawText(r,"GO",sign+19,GROUND_Y-137,4);
    /* ? block and floating platforms from Isaac's sketch */
    for(int i=0;i<3;i++) brick(r,2280+i*82-camera,400,80,40);
    fill(r,BLOCK_X-camera,397,80,44,
         blockUsed?147:247,blockUsed?142:189,blockUsed?131:56);
    drawText(r,blockUsed?"!":"?",BLOCK_X+26-camera,401,4);
    if (!blockUsed) drawBomb(r,BLOCK_X+24-camera,343);
    /* Decorative arena platforms + crossing laser hazard */
    for(int i=0;i<4;i++) brick(r,4100+i*420-camera,466-(i%2)*68,110,35);
    fill(r,LASER_X-camera,GROUND_Y-150,30,150,70,68,91);
    fill(r,LASER_X+460-camera,GROUND_Y-150,30,150,70,68,91);
    int phase=(SDL_GetTicks()/16)%180;
    if (phase<90) fill(r,LASER_X+30-camera,GROUND_Y-118,430,10,249,76,96);
    for(int i=0;i<3;i++) brick(r,7400+i*245-camera,420+(i%2)*62,115,32);
    /* Boss is high enough to walk UNDER its weak point. */
    if (!bossDefeated) {
        /* Sprite art is controller ONLY: no head, arms or legs. */
        int phase=(int)(SDL_GetTicks()/170) % 24;
        int which=bossShotFlash ? 1 : (phase<13?0:(phase<17?1:(phase<20?2:3)));
        if (bossSprites[which]) {
            SDL_Rect dst={BOSS_X-135-camera,185+((phase%6)-3)*2,270,170};
            SDL_RenderCopy(r,bossSprites[which],NULL,&dst);
        } else drawController(r,BOSS_X-130-camera,196,0);
        drawText(r,"WEAK POINT",BOSS_X-132-camera,375,2);
        fill(r,BOSS_X-2-camera,391,6,25,255,207,83);
    } else if (!ssdCollected) {
        drawSSD(r,ssd,BOSS_X-64-camera,GROUND_Y-91);
    }
    int flag=9820-camera;
    fill(r,flag,GROUND_Y-280,9,280,239,237,233);
    fill(r,flag+9,GROUND_Y-275,73,40,250,90,109);
    if (bombReady && !bombFlying) drawBomb(r,SCREEN_W-86,22);
    if (bombFlying) drawBomb(r,BOSS_X-16-camera,(int)flyingBombY);
    fill(r,10,12,400,54,33,74,122);
    drawText(r,"WORLD 1-2",22,27,3);
    if (!bombReady && !blockUsed) drawText(r,"HIT ? BLOCK FOR BOMB",24,83,2);
    else if (bombReady && !bossDefeated) drawText(r,"UNDER BOSS  PRESS Y",24,83,2);
    else if (bossDefeated && !ssdCollected) drawText(r,"COLLECT SSD",24,83,2);
    else if (ssdCollected) drawText(r,"REACH THE FLAG",24,83,2);
}

/* World 2-1 (2013): a short pipe bonus room based on Isaac's sketch.
   Kept separate from World 1 and from the multiplayer transport. */
#define W21_WIDTH 4500
#define W21_ENTRY_PIPE_X 1700
#define W21_EXIT_PIPE_X 2870
#define W21_PIPE_TOP (GROUND_Y-115)
#define W21_ROOM_EXIT_X 950

static const SDL_Rect w21Platforms[] = {
    {410,470,112,28}, {715,415,110,28}, {1040,455,120,28},
    {1340,392,115,28}, {2110,438,120,28}, {2370,470,120,28},
    {3240,440,110,28}, {3480,405,110,28},
    /* Pipe rims are solid landing surfaces. */
    {W21_ENTRY_PIPE_X-10,W21_PIPE_TOP-18,140,18},
    {W21_EXIT_PIPE_X-10,W21_PIPE_TOP-18,140,18},
    /* End stairs. */
    {3880,GROUND_Y-36,65,36}, {3945,GROUND_Y-72,65,72},
    {4010,GROUND_Y-108,65,108}, {4075,GROUND_Y-144,65,144}
};

static const SDL_Rect w21RoomPlatforms[] = {
    {W21_ROOM_EXIT_X-10,W21_PIPE_TOP-18,140,18}
};

static void w21DrawPipe(SDL_Renderer *r, int xx, int top, int camera)
{
    int x=xx-camera;
    fill(r,x,top,120,GROUND_Y-top,23,150,72);
    fill(r,x+11,top+12,18,GROUND_Y-top-12,56,199,106);
    fill(r,x+103,top+12,11,GROUND_Y-top-12,16,100,53);
    fill(r,x-10,top-18,140,26,26,187,84);
    fill(r,x-4,top-13,128,8,93,236,130);
}

static void w21DrawTinySSD(SDL_Renderer *r, SDL_Texture *ssd, int xx, int yy)
{
    if (ssd) {
        SDL_Rect dst={xx,yy,72,50};
        SDL_RenderCopy(r,ssd,NULL,&dst);
    } else {
        fill(r,xx,yy,72,50,42,46,59);
        fill(r,xx+5,yy+5,62,40,104,112,131);
        drawText(r,"SSD",xx+11,yy+16,2);
    }
}

static void w21DrawOutside(SDL_Renderer *r, int camera, int hasSSD)
{
    SDL_SetRenderDrawColor(r,108,195,248,255);
    SDL_RenderClear(r);
    for(int i=0;i<22;i++) pixelCloud(r,i*270-camera/4,80+(i%3)*42);
    for(int i=0;i<25;i++) {
        int xx=i*220-camera/3;
        fill(r,xx,GROUND_Y-138+(i%3)*18,170,140,111,194,130);
        fill(r,xx+48,GROUND_Y-170+(i%3)*18,76,48,111,194,130);
    }
    fill(r,0,GROUND_Y,SCREEN_W,SCREEN_H-GROUND_Y,145,94,58);
    fill(r,0,GROUND_Y,SCREEN_W,18,74,207,87);
    for(int i=0;i<(int)(sizeof(w21Platforms)/sizeof(w21Platforms[0]));i++) {
        SDL_Rect p=w21Platforms[i];
        /* Pipe rims are drawn with the pipes below. */
        if(i==8 || i==9) continue;
        brick(r,p.x-camera,p.y,p.w,p.h);
    }
    w21DrawPipe(r,W21_ENTRY_PIPE_X,W21_PIPE_TOP,camera);
    w21DrawPipe(r,W21_EXIT_PIPE_X,W21_PIPE_TOP,camera);
    /* Sign, question block, critter, and finish stairs from the sketch. */
    fill(r,100-camera,GROUND_Y-100,7,100,97,68,50);
    fill(r,80-camera,GROUND_Y-146,150,49,244,205,118);
    drawText(r,"GO",111-camera,GROUND_Y-134,3);
    fill(r,625-camera,320,70,70,245,187,48);
    drawText(r,"?",643-camera,330,5);
    fill(r,980-camera,GROUND_Y-36,43,36,134,77,40);
    fill(r,988-camera,GROUND_Y-25,8,8,255,255,255);
    fill(r,1006-camera,GROUND_Y-25,8,8,255,255,255);
    fill(r,4220-camera,GROUND_Y-280,9,280,236,234,233);
    fill(r,4229-camera,GROUND_Y-275,70,42,242,91,100);
    drawText(r,"WORLD 2-1",20,24,3);
    if(hasSSD) drawText(r,"SSD FOUND  REACH THE FLAG",20,70,2);
}

static void w21DrawRoom(SDL_Renderer *r, SDL_Texture *ssd, int hasSSD)
{
    SDL_SetRenderDrawColor(r,22,25,49,255);
    SDL_RenderClear(r);
    /* One-screen compact bonus room; solid-looking wall surround. */
    fill(r,125,135,1020,510,64,66,92);
    fill(r,155,160,960,GROUND_Y-160,87,99,121);
    for(int i=0;i<12;i++) {
        fill(r,170+i*78,170+(i%2)*24,54,14,101,113,135);
    }
    fill(r,155,GROUND_Y,960,55,119,84,72);
    fill(r,155,GROUND_Y,960,13,162,127,91);
    /* Entrance is visible on the left; exit is a separate usable pipe. */
    w21DrawPipe(r,195,W21_PIPE_TOP,0);
    w21DrawPipe(r,W21_ROOM_EXIT_X,W21_PIPE_TOP,0);
    if(!hasSSD) w21DrawTinySSD(r,ssd,590,GROUND_Y-64);
    drawText(r,"SSD PIPE ROOM",445,205,3);
    drawText(r,"EXIT",972,W21_PIPE_TOP-85,2);
}

/* Frame-rate-compatible platform physics, reusing World 1's controls. */
static int w21Move(float *x, float *y, float *vy, int *facing,
                   int *frame, int hold, int trigger, float stickX, int room)
{
    const SDL_Rect *platforms2=room?w21RoomPlatforms:w21Platforms;
    int count=room ? (int)(sizeof(w21RoomPlatforms)/sizeof(w21RoomPlatforms[0]))
                   : (int)(sizeof(w21Platforms)/sizeof(w21Platforms[0]));
    int left=(hold&VPAD_BUTTON_LEFT) || stickX < -0.25f;
    int right=(hold&VPAD_BUTTON_RIGHT) || stickX > 0.25f;
    int moving=0;
    if(left && !right) { *x-=5.0f; *facing=0; moving=1; }
    if(right && !left) { *x+=5.0f; *facing=1; moving=1; }
    if(*x<(room?165.0f:0.0f)) *x=room?165.0f:0.0f;
    if(*x>(room?1065.0f:(float)(W21_WIDTH-PLAYER_W)))
        *x=room?1065.0f:(float)(W21_WIDTH-PLAYER_W);
    float oldBottom=*y+PLAYER_H;
    int grounded=(*vy>=0 && oldBottom>=GROUND_Y-2 && oldBottom<=GROUND_Y+3);
    for(int i=0;i<count;i++) {
        SDL_Rect p=platforms2[i];
        if(*vy>=0 && *x+PLAYER_W>p.x && *x<p.x+p.w &&
           oldBottom>=p.y-3 && oldBottom<=p.y+5) grounded=1;
    }
    if(grounded && (trigger&(VPAD_BUTTON_A|VPAD_BUTTON_B))) *vy=-15.5f;
    *vy+=0.55f;
    if(*vy>15.0f) *vy=15.0f;
    *y+=*vy;
    if(*vy>=0) {
        float landing=GROUND_Y-PLAYER_H;
        for(int i=0;i<count;i++) {
            SDL_Rect p=platforms2[i];
            if(*x+PLAYER_W>p.x && *x<p.x+p.w &&
               oldBottom<=p.y+5 && *y+PLAYER_H>=p.y &&
               p.y-PLAYER_H<landing) landing=p.y-PLAYER_H;
        }
        if(*y>=landing) { *y=landing; *vy=0; }
    }
    if(moving && *vy==0.0f) (*frame)++;
    else *frame=0;
    return *vy!=0.0f ? (*vy<0?5:6) : (moving?1+((*frame/4)%4):0);
}

static int w21OnPipe(float x, float y, int pipeX)
{
    float feet=y+PLAYER_H;
    return x+PLAYER_W>pipeX-10 && x<pipeX+130 &&
           feet>=W21_PIPE_TOP-23 && feet<=W21_PIPE_TOP-12;
}

/* WORLD 2-2 (2013): brick-built harbor city. Background is a separate BMP.
   Platforms, studs, obstacles and the goal are drawn in code; no extra sprites. */
#define W22_WIDTH 5600
#define W22_STUD_COUNT 18
static const SDL_Rect w22Platforms[] = {
    {330,478,145,27}, {610,422,125,27}, {890,475,130,27},
    {1250,445,150,27}, {1520,389,130,27}, {1780,458,140,27},
    {2100,435,150,27}, {2420,375,140,27}, {2700,460,130,27},
    {3030,437,150,27}, {3280,390,150,27}, {3550,460,140,27},
    {3850,415,155,27}, {4160,470,145,27}, {4440,405,145,27},
    {4730,450,150,27}, {5010,405,155,27}
};
#define W22_PLATFORM_COUNT ((int)(sizeof(w22Platforms)/sizeof(w22Platforms[0])))
static const SDL_Point w22Studs[W22_STUD_COUNT] = {
    {385,438},{655,382},{935,435},{1300,405},{1570,349},{1830,418},
    {2160,395},{2475,335},{2750,420},{3090,397},{3340,350},{3620,420},
    {3900,375},{4210,430},{4490,365},{4800,410},{5060,365},{5240,495}
};
static const int w22PoliceX[] = {1090,1940,2890,4050,4900};

static void w22DrawPolice(SDL_Renderer *r,int x,int y)
{
    /* Small original blue-uniform patrol obstacle. */
    fill(r,x+7,y,27,9,19,50,123);      /* cap */
    fill(r,x+11,y+9,20,14,240,201,111);
    fill(r,x+8,y+24,28,24,20,82,174);
    fill(r,x+3,y+26,7,18,20,82,174);
    fill(r,x+36,y+26,7,18,20,82,174);
    fill(r,x+9,y+48,12,9,30,44,78);
    fill(r,x+24,y+48,12,9,30,44,78);
    fill(r,x+15,y+14,4,4,24,35,54);
    fill(r,x+26,y+14,4,4,24,35,54);
    fill(r,x+19,y+29,7,8,247,213,83);
}

static void w22DrawPlatform(SDL_Renderer *r,SDL_Rect p,int camera)
{
    int x=p.x-camera;
    if(x+p.w<0 || x>SCREEN_W) return;
    fill(r,x,p.y,p.w,p.h,20,68,150);
    fill(r,x,p.y,p.w,7,45,145,241);
    fill(r,x+5,p.y+12,p.w-10,7,9,38,104);
    for(int j=0;j<p.w/27;j++) fill(r,x+12+j*27,p.y-6,15,7,61,167,255);
}

static void w22Draw(SDL_Renderer *r,SDL_Texture *backdrop,int camera,
                    const int collected[],int studs,int gotSSD,SDL_Texture *ssd)
{
    SDL_SetRenderDrawColor(r,55,157,238,255);
    SDL_RenderClear(r);
    if(backdrop) {
        /* Quarter-speed scrolling, repeated for the full harbor. */
        int shift=(camera/4)%SCREEN_W;
        SDL_Rect first={-shift,0,SCREEN_W,SCREEN_H};
        SDL_Rect second={SCREEN_W-shift,0,SCREEN_W,SCREEN_H};
        SDL_RenderCopy(r,backdrop,NULL,&first);
        SDL_RenderCopy(r,backdrop,NULL,&second);
    } else {
        for(int i=0;i<12;i++) {
            int xx=i*220-camera/4;
            fill(r,xx,300+(i%3)*30,155,285,68,119,170);
            for(int j=0;j<4;j++) fill(r,xx+15+j*32,345,19,27,134,210,242);
        }
    }
    /* Dockside pavement, separated from the decorative background water. */
    fill(r,0,GROUND_Y,SCREEN_W,SCREEN_H-GROUND_Y,51,59,76);
    fill(r,0,GROUND_Y,SCREEN_W,13,174,183,190);
    for(int i=0;i<W22_WIDTH/84;i++) {
        int xx=i*84-camera;
        if(xx>-84 && xx<SCREEN_W) {
            fill(r,xx,GROUND_Y+44,78,4,75,85,106);
            fill(r,xx+20,GROUND_Y+89,56,4,75,85,106);
            fill(r,xx+30,GROUND_Y+3,17,5,227,189,52);
        }
    }
    for(int i=0;i<W22_PLATFORM_COUNT;i++) w22DrawPlatform(r,w22Platforms[i],camera);
    /* Low cargo crates; drawn as scenery and jump markers. */
    const int cargo[]={770,1660,2600,3710,4590};
    for(int i=0;i<5;i++) {
        int xx=cargo[i]-camera;
        if(xx<-120 || xx>SCREEN_W) continue;
        fill(r,xx,GROUND_Y-72,100,72,i%2?204:38,i%2?89:112,i%2?68:192);
        fill(r,xx+6,GROUND_Y-66,88,8,250,173,95);
        for(int k=0;k<4;k++) fill(r,xx+16+k*22,GROUND_Y-49,4,37,47,63,94);
    }
    /* Studs are collectible and disappear when taken. */
    for(int i=0;i<W22_STUD_COUNT;i++) {
        if(collected[i]) continue;
        int xx=w22Studs[i].x-camera,yy=w22Studs[i].y;
        if(xx<-30 || xx>SCREEN_W+30) continue;
        fill(r,xx-9,yy-13,18,25,220,135,20);
        fill(r,xx-5,yy-9,10,17,255,229,67);
    }
    for(int i=0;i<5;i++) {
        int xx=w22PoliceX[i]-camera;
        if(xx>-70 && xx<SCREEN_W+50) w22DrawPolice(r,xx,GROUND_Y-57);
    }
    /* Optional collectible, accessible by jumping onto the blue dock platforms. */
    if(!gotSSD) w21DrawTinySSD(r,ssd,3350-camera,GROUND_Y-270);
    int flag=5390-camera;
    fill(r,flag,GROUND_Y-300,9,300,230,237,246);
    fill(r,flag+9,GROUND_Y-295,90,52,40,134,233);
    fill(r,flag+23,GROUND_Y-280,15,18,255,221,66);
    fill(r,12,12,340,54,33,74,122);
    drawText(r,"WORLD 2-2",24,28,3);
    char count[32];
    SDL_snprintf(count,sizeof(count),"STUDS: %d",studs);
    drawText(r,count,24,86,3);
    if(gotSSD) drawText(r,"SSD FOUND",24,126,2);
}

static int w22Move(float *x,float *y,float *vy,int *facing,int *frame,
                   int hold,int trigger,float stickX)
{
    int left=(hold&VPAD_BUTTON_LEFT)||stickX < -0.25f;
    int right=(hold&VPAD_BUTTON_RIGHT)||stickX > 0.25f;
    int moving=0;
    if(left&&!right) {*x-=5.0f;*facing=0;moving=1;}
    if(right&&!left) {*x+=5.0f;*facing=1;moving=1;}
    if(*x<0)*x=0;
    if(*x>W22_WIDTH-PLAYER_W)*x=W22_WIDTH-PLAYER_W;
    float oldBottom=*y+PLAYER_H;
    int grounded=(*vy>=0 && oldBottom>=GROUND_Y-2 && oldBottom<=GROUND_Y+3);
    for(int i=0;i<W22_PLATFORM_COUNT;i++) {
        SDL_Rect p=w22Platforms[i];
        if(*vy>=0 && *x+PLAYER_W>p.x && *x<p.x+p.w &&
           oldBottom>=p.y-3 && oldBottom<=p.y+5) grounded=1;
    }
    if(grounded && (trigger&(VPAD_BUTTON_A|VPAD_BUTTON_B))) *vy=-15.5f;
    *vy+=0.55f;
    if(*vy>15.0f)*vy=15.0f;
    *y+=*vy;
    if(*vy>=0) {
        float landing=GROUND_Y-PLAYER_H;
        for(int i=0;i<W22_PLATFORM_COUNT;i++) {
            SDL_Rect p=w22Platforms[i];
            if(*x+PLAYER_W>p.x && *x<p.x+p.w &&
               oldBottom<=p.y+5 && *y+PLAYER_H>=p.y &&
               p.y-PLAYER_H<landing) landing=p.y-PLAYER_H;
        }
        if(*y>=landing){*y=landing;*vy=0;}
    }
    if(moving && *vy==0.0f) (*frame)++;
    else *frame=0;
    return *vy!=0.0f ? (*vy<0?5:6) : (moving?1+((*frame/4)%4):0);
}


/* WORLD 2-3 (2013): giant garden adventure inspired by Pikmin 3.
   The lush parallax background is rendered in code: no new BMP is needed. */
#define W23_WIDTH 6000
#define W23_NECTAR_COUNT 18
#define W23_PIKMIN_COUNT 3
#define W23_BUG_COUNT 4

static const SDL_Rect w23Platforms[] = {
    {350,477,135,24}, {675,419,140,24}, {950,468,120,24},
    {1110,435,120,24}, {1380,450,150,24}, {1680,395,135,24},
    {1960,470,145,24}, {2240,414,135,24}, {2540,458,135,24},
    {2800,410,135,24}, {3050,465,125,24},
    {3820,460,145,24}, {4090,410,135,24}, {4360,460,135,24},
    {4760,440,145,24}, {4940,355,130,24}, {5200,420,145,24},
    {5470,475,130,24}
};
#define W23_PLATFORM_COUNT ((int)(sizeof(w23Platforms)/sizeof(w23Platforms[0])))

static const SDL_Point w23Nectar[W23_NECTAR_COUNT] = {
    {400,437},{720,378},{995,425},{1155,395},{1430,410},
    {1740,355},{2000,430},{2300,374},{2590,418},{2850,370},
    {3100,425},{3870,420},{4150,370},{4410,420},{4810,400},
    {4980,315},{5250,380},{5530,435}
};
static const SDL_Point w23Pikmin[W23_PIKMIN_COUNT] = {
    {750,GROUND_Y-45},{1800,GROUND_Y-45},{2930,GROUND_Y-45}
};
static const int w23BugBases[W23_BUG_COUNT] = {1020,2450,4040,5350};

static int w23BugX(int index, Uint32 now)
{
    int phase=(int)((now/26u + (Uint32)(index*31))%120u);
    return w23BugBases[index] + (phase<60 ? phase : 120-phase) - 30;
}

static void w23Leaf(SDL_Renderer *r,int x,int y,int w,int h)
{
    fill(r,x,y,w,h,28,117,52);
    fill(r,x+5,y+3,w-10,h-8,68,185,78);
    fill(r,x+w/4,y+5,w/2,5,144,225,92);
    fill(r,x+w/2-3,y+8,6,h-11,41,136,61);
}

static void w23Mushroom(SDL_Renderer *r,int x,int y,int size)
{
    /* y is the ground line; mushrooms tower over the player. */
    fill(r,x+size/3,y-size,size/3,size,219,191,137);
    fill(r,x+size/3+5,y-size+8,size/8,size-18,248,222,173);
    fill(r,x,y-size-size/3,size,size/2,185,53,85);
    fill(r,x+size/8,y-size-size/3+10,size*3/4,size/5,238,103,115);
    fill(r,x+size/4,y-size-size/3+14,size/8,size/8,255,225,176);
    fill(r,x+size*2/3,y-size-size/3+25,size/9,size/8,255,225,176);
}

static void w23Flower(SDL_Renderer *r,int x,int y,int size,int pink)
{
    fill(r,x+size/2-4,y-size/2,8,size/2,35,133,63);
    fill(r,x+size/2-27,y-size/3,30,10,53,171,77);
    fill(r,x+size/2+4,y-size/4,26,9,65,184,80);
    Uint8 rr=pink?242:245,gg=pink?111:206,bb=pink?156:64;
    fill(r,x+size/2-14,y-size+9,28,32,rr,gg,bb);
    fill(r,x+size/2-33,y-size+28,66,24,rr,gg,bb);
    fill(r,x+size/2-12,y-size+39,25,27,rr,gg,bb);
    fill(r,x+size/2-10,y-size+27,20,20,255,228,91);
}

static void w23DrawPikmin(SDL_Renderer *r,int x,int y,int color)
{
    const Uint8 rr[3]={230,239,53},gg[3]={64,206,136},bb[3]={62,59,241};
    fill(r,x+12,y,4,14,33,109,53);
    fill(r,x+10,y-5,16,8,100,211,81);
    fill(r,x+5,y+14,26,24,rr[color],gg[color],bb[color]);
    fill(r,x+9,y+37,7,9,rr[color],gg[color],bb[color]);
    fill(r,x+23,y+37,7,9,rr[color],gg[color],bb[color]);
    fill(r,x+11,y+20,6,8,255,255,240);
    fill(r,x+23,y+20,6,8,255,255,240);
    fill(r,x+14,y+23,2,4,31,40,49);
    fill(r,x+25,y+23,2,4,31,40,49);
}

static void w23DrawBug(SDL_Renderer *r,int x,int y)
{
    /* Original little garden bug; a moving obstacle, not a Pikmin sprite. */
    fill(r,x+5,y+15,53,31,126,66,53);
    fill(r,x+13,y+5,37,18,218,118,84);
    fill(r,x+9,y+2,20,21,240,223,174);
    fill(r,x+36,y+2,20,21,240,223,174);
    fill(r,x+17,y+8,7,8,27,35,39);
    fill(r,x+44,y+8,7,8,27,35,39);
    fill(r,x+11,y+44,14,9,89,53,41);
    fill(r,x+40,y+44,14,9,89,53,41);
}

static void w23Draw(SDL_Renderer *r,int camera,const int nectarTaken[],
                    int nectarCount,const int pikminFound[],int gotSSD,
                    SDL_Texture *ssd)
{
    /* Pastel sky and giant parallax garden, all native SDL rectangles. */
    SDL_SetRenderDrawColor(r,124,196,220,255);
    SDL_RenderClear(r);
    for(int band=0;band<12;band++)
        fill(r,0,band*49,SCREEN_W,49,
             (Uint8)(118+band*5),(Uint8)(191+band*3),(Uint8)(219-band*4));
    for(int i=0;i<12;i++) {
        int xx=i*240-camera/6;
        pixelCloud(r,xx,55+(i%3)*28);
    }
    for(int i=0;i<28;i++) {
        int xx=i*260-camera/4;
        fill(r,xx,375+(i%3)*22,195,235,92,157,121);
        fill(r,xx+30,345+(i%3)*24,120,100,121,182,128);
    }
    for(int i=0;i<16;i++) {
        int xx=i*380-camera/3;
        fill(r,xx+40,165+(i%3)*30,52,420,97,117,73);
        fill(r,xx+52,160+(i%3)*30,13,420,137,155,90);
        fill(r,xx-50,100+(i%3)*30,235,105,39,133,66);
        fill(r,xx-18,68+(i%3)*30,170,78,60,157,72);
        fill(r,xx+20,55+(i%3)*30,85,63,99,184,91);
    }
    for(int i=0;i<18;i++) {
        int xx=i*355-camera/2;
        w23Flower(r,xx,GROUND_Y-18,90+(i%3)*16,i%2);
    }
    /* Dark rich soil and bright grass are in the playable layer. */
    fill(r,0,GROUND_Y,SCREEN_W,SCREEN_H-GROUND_Y,100,70,54);
    fill(r,0,GROUND_Y,SCREEN_W,17,66,170,68);
    for(int i=0;i<W23_WIDTH/52;i++) {
        int xx=i*52-camera;
        if(xx>-55 && xx<SCREEN_W+10) {
            fill(r,xx+7,GROUND_Y+40+(i%2)*28,26,7,139,96,68);
            fill(r,xx+20,GROUND_Y-8,6,9,29,132,58);
        }
    }
    /* Giant foreground mushrooms and berries. */
    for(int i=0;i<9;i++) {
        int xx=380+i*640-camera;
        if(xx>-220 && xx<SCREEN_W+220)
            w23Mushroom(r,xx,GROUND_Y,90+(i%3)*28);
    }
    for(int i=0;i<W23_PLATFORM_COUNT;i++) {
        SDL_Rect p=w23Platforms[i];
        if(p.x+p.w<camera || p.x>camera+SCREEN_W) continue;
        w23Leaf(r,p.x-camera,p.y,p.w,p.h);
        fill(r,p.x+18-camera,p.y+p.h,7,GROUND_Y-p.y-p.h,69,112,56);
    }
    /* Small ponds require a jump. The central pond is solved by gathering
       three Pikmin to assemble the crossing. */
    const int ponds[2][2]={{1185,1335},{4510,4690}};
    for(int i=0;i<2;i++) {
        int xx=ponds[i][0]-camera,ww=ponds[i][1]-ponds[i][0];
        fill(r,xx,GROUND_Y-1,ww,53,31,128,196);
        fill(r,xx+12,GROUND_Y+6,ww-24,7,91,207,244);
    }
    int count=pikminFound[0]+pikminFound[1]+pikminFound[2];
    int bridgeX=3330-camera;
    fill(r,bridgeX,GROUND_Y-1,420,56,28,124,196);
    fill(r,bridgeX+12,GROUND_Y+8,394,8,97,205,237);
    if(count==3) {
        for(int i=0;i<11;i++) {
            fill(r,bridgeX+i*38,GROUND_Y-10,36,24,184,139,82);
            fill(r,bridgeX+i*38+3,GROUND_Y-7,30,5,231,184,104);
        }
        fill(r,bridgeX,GROUND_Y-29,7,20,97,73,52);
        fill(r,bridgeX+408,GROUND_Y-29,7,20,97,73,52);
    } else {
        /* A giant root blocks the crossing until all 3 are rescued. */
        fill(r,3300-camera,GROUND_Y-220,52,220,97,65,50);
        fill(r,3290-camera,GROUND_Y-227,74,20,125,90,55);
        fill(r,3298-camera,GROUND_Y-234,58,9,175,119,67);
    }
    for(int i=0;i<W23_NECTAR_COUNT;i++) {
        if(nectarTaken[i])continue;
        int xx=w23Nectar[i].x-camera,yy=w23Nectar[i].y;
        if(xx<-30 || xx>SCREEN_W+30)continue;
        fill(r,xx-10,yy-10,20,23,230,139,40);
        fill(r,xx-5,yy-7,10,15,255,223,83);
        fill(r,xx-1,yy-4,4,9,255,251,184);
    }
    for(int i=0;i<W23_PIKMIN_COUNT;i++)
        if(!pikminFound[i]) w23DrawPikmin(r,w23Pikmin[i].x-camera,
                                        w23Pikmin[i].y-5,i);
    Uint32 now=SDL_GetTicks();
    for(int i=0;i<W23_BUG_COUNT;i++) {
        int xx=w23BugX(i,now)-camera;
        if(xx>-75 && xx<SCREEN_W+70) w23DrawBug(r,xx,GROUND_Y-54);
    }
    if(!gotSSD) w21DrawTinySSD(r,ssd,4990-camera,GROUND_Y-298);
    /* Two visual checkpoint signs. */
    for(int i=0;i<2;i++) {
        int xx=(i==0?2150:3910)-camera;
        fill(r,xx,GROUND_Y-100,7,100,122,85,58);
        fill(r,xx-9,GROUND_Y-134,98,40,76,164,99);
        drawText(r,"SAVE",xx+1,GROUND_Y-124,2);
    }
    int flag=5810-camera;
    fill(r,flag,GROUND_Y-315,9,315,239,235,215);
    fill(r,flag+9,GROUND_Y-312,86,51,239,137,70);
    fill(r,flag+20,GROUND_Y-300,15,18,255,235,138);
    fill(r,12,12,320,54,39,92,70);
    drawText(r,"WORLD 2-3",24,28,3);
    char hud[40];
    SDL_snprintf(hud,sizeof(hud),"PIKMIN: %d/3",count);
    drawText(r,hud,24,86,3);
    SDL_snprintf(hud,sizeof(hud),"NECTAR: %d",nectarCount);
    drawText(r,hud,24,126,2);
    if(gotSSD)drawText(r,"SSD FOUND",24,160,2);
}

static int w23Move(float *x,float *y,float *vy,int *facing,int *frame,
                   int hold,int trigger,float stickX,int pikminCount)
{
    int left=(hold&VPAD_BUTTON_LEFT)||stickX < -0.25f;
    int right=(hold&VPAD_BUTTON_RIGHT)||stickX > 0.25f;
    int moving=0;
    if(left&&!right){*x-=5.0f;*facing=0;moving=1;}
    if(right&&!left){*x+=5.0f;*facing=1;moving=1;}
    if(*x<0)*x=0;
    if(*x>W23_WIDTH-PLAYER_W)*x=W23_WIDTH-PLAYER_W;
    /* The root gate cannot be bypassed before all three Pikmin are found. */
    if(pikminCount<3 && *x>3250.0f)*x=3250.0f;
    float oldBottom=*y+PLAYER_H;
    int grounded=(*vy>=0 && oldBottom>=GROUND_Y-2 && oldBottom<=GROUND_Y+3);
    for(int i=0;i<W23_PLATFORM_COUNT;i++) {
        SDL_Rect p=w23Platforms[i];
        if(*vy>=0 && *x+PLAYER_W>p.x && *x<p.x+p.w &&
           oldBottom>=p.y-3 && oldBottom<=p.y+5)grounded=1;
    }
    if(grounded && (trigger&(VPAD_BUTTON_A|VPAD_BUTTON_B)))*vy=-15.5f;
    *vy+=0.55f;
    if(*vy>15.0f)*vy=15.0f;
    *y+=*vy;
    if(*vy>=0) {
        float landing=GROUND_Y-PLAYER_H;
        for(int i=0;i<W23_PLATFORM_COUNT;i++) {
            SDL_Rect p=w23Platforms[i];
            if(*x+PLAYER_W>p.x && *x<p.x+p.w &&
               oldBottom<=p.y+5 && *y+PLAYER_H>=p.y &&
               p.y-PLAYER_H<landing)landing=p.y-PLAYER_H;
        }
        if(*y>=landing){*y=landing;*vy=0;}
    }
    if(moving && *vy==0.0f)(*frame)++;
    else *frame=0;
    return *vy!=0.0f ? (*vy<0?5:6) : (moving?1+((*frame/4)%4):0);
}

static void w23Followers(SDL_Renderer *r,float x,float y,int camera,
                         int facing,const int pikminFound[])
{
    int n=0;
    for(int i=0;i<W23_PIKMIN_COUNT;i++) {
        if(!pikminFound[i])continue;
        int xx=(int)x-camera+(facing?-45-n*38:PLAYER_W+14+n*38);
        w23DrawPikmin(r,xx,(int)y+PLAYER_H-40,i);
        n++;
    }
}

static void w23Reset(float *x,float *y,float *vy,int *facing,int *frame,
                     int found[W23_PIKMIN_COUNT],int nectar[W23_NECTAR_COUNT],
                     int *nectarCount,int *gotSSD,int *checkpoint)
{
    *x=90.0f;*y=GROUND_Y-PLAYER_H;*vy=0.0f;
    *facing=1;*frame=0;*nectarCount=0;*gotSSD=0;*checkpoint=90;
    memset(found,0,sizeof(int)*W23_PIKMIN_COUNT);
    memset(nectar,0,sizeof(int)*W23_NECTAR_COUNT);
}


/* WORLD 2-4 (2013): original ocean-adventure tribute to Wind Waker HD.
   Sailing, island pickups, a storm and the finale; drawn without extra assets. */
#define W24_WIDTH 6300
#define W24_PEARL_COUNT 3
#define W24_CRAB_COUNT 4
static const SDL_Rect w24Platforms[]={
    {360,467,125,26},{620,416,140,26},{920,458,135,26},
    {1210,425,145,26},{2470,445,145,26},{2800,402,140,26},
    {3190,458,140,26},{3480,418,140,26},
    {4680,449,130,26},{5010,416,140,26},
    {5320,420,160,26},{5510,455,135,26}
};
#define W24_PLATFORM_COUNT ((int)(sizeof(w24Platforms)/sizeof(w24Platforms[0])))
static const SDL_Point w24Pearls[W24_PEARL_COUNT]={
    {760,GROUND_Y-60},{3020,GROUND_Y-60},{5110,GROUND_Y-60}
};
static const int w24CrabBases[W24_CRAB_COUNT]={1090,2630,4790,5500};

/* Accessible boardwalks connect the islands.  The boats remain optional. */
static int w24Land(float xx)
{
    return xx>=0.0f && xx<(float)W24_WIDTH;
}
static int w24CrabX(int index,Uint32 now)
{
    int t=(int)((now/38u+(Uint32)(index*29))%108u);
    return w24CrabBases[index]+(t<54?t:108-t)-27;
}
static void w24Boat(SDL_Renderer *r,int xx,int yy)
{
    /* Little red-sailed boat; the player's Mii stands above its deck. */
    fill(r,xx,yy+32,175,20,121,64,41);
    fill(r,xx+18,yy+52,138,17,76,49,38);
    fill(r,xx+30,yy+25,118,10,232,180,93);
    fill(r,xx+89,yy-110,7,141,95,70,49);
    fill(r,xx+99,yy-101,53,100,198,59,52);
    fill(r,xx+105,yy-92,38,12,238,119,88);
    fill(r,xx+18,yy+58,13,9,249,210,125);
    fill(r,xx+142,yy+58,13,9,249,210,125);
}
static void w24Crab(SDL_Renderer *r,int xx,int yy)
{
    fill(r,xx+10,yy+15,37,25,217,80,68);
    fill(r,xx+4,yy+20,10,8,248,121,94);
    fill(r,xx+44,yy+20,10,8,248,121,94);
    fill(r,xx+14,yy+3,10,15,245,189,125);
    fill(r,xx+35,yy+3,10,15,245,189,125);
    fill(r,xx+17,yy+5,4,6,34,50,66);
    fill(r,xx+38,yy+5,4,6,34,50,66);
    fill(r,xx+14,yy+38,9,8,122,56,53);
    fill(r,xx+34,yy+38,9,8,122,56,53);
}
static void w24Pearl(SDL_Renderer *r,int xx,int yy)
{
    fill(r,xx-16,yy-16,32,32,56,139,191);
    fill(r,xx-12,yy-12,24,24,228,240,245);
    fill(r,xx-7,yy-9,11,10,255,255,255);
    fill(r,xx-2,yy+7,11,5,150,213,239);
}
static void w24Island(SDL_Renderer *r,int x,int width,int camera,int index)
{
    int xx=x-camera;
    if(xx+width<0 || xx>SCREEN_W)return;
    fill(r,xx,GROUND_Y,width,SCREEN_H-GROUND_Y,190,157,101);
    fill(r,xx,GROUND_Y,width,18,96,171,91);
    for(int i=0;i<width/75;i++) {
        int tx=xx+i*75+12;
        fill(r,tx,GROUND_Y+41+(i%3)*18,32,8,155,123,84);
        fill(r,tx+11,GROUND_Y-7,7,7,59,151,72);
    }
    /* Dock and palm-tree silhouettes. */
    int palm=xx+width/2;
    fill(r,palm,GROUND_Y-195,20,195,132,92,55);
    fill(r,palm-92,GROUND_Y-215,198,20,40,133,75);
    fill(r,palm-70,GROUND_Y-243,143,28,52,155,76);
    fill(r,palm-40,GROUND_Y-263,92,32,66,172,85);
    if(index<2) {
        fill(r,xx+width-145,GROUND_Y-13,155,19,150,100,63);
        for(int k=0;k<3;k++)fill(r,xx+width-140+k*61,GROUND_Y+6,13,61,117,78,55);
    }
}
static void w24Draw(SDL_Renderer *r,int camera,const int pearls[],int gotSSD,
                    int sailing,int boatX,Uint32 now,SDL_Texture *ssd)
{
    SDL_SetRenderDrawColor(r,95,179,224,255);
    SDL_RenderClear(r);
    /* Layered cel-shaded sky, clouds, distant islands and endless ocean. */
    for(int b=0;b<9;b++)
        fill(r,0,b*54,SCREEN_W,54,(Uint8)(91+b*6),
             (Uint8)(172+b*4),(Uint8)(226-b*3));
    for(int i=0;i<26;i++) {
        int cx=i*260-camera/7;
        pixelCloud(r,cx,66+(i%4)*35);
    }
    for(int i=0;i<18;i++) {
        int hx=i*410-camera/4;
        fill(r,hx,468+(i%3)*22,260,135,83,143,141);
        fill(r,hx+62,424+(i%3)*22,122,55,108,165,152);
    }
    fill(r,0,GROUND_Y-4,SCREEN_W,SCREEN_H-GROUND_Y+4,28,128,188);
    fill(r,0,GROUND_Y+11,SCREEN_W,27,47,167,207);
    for(int i=0;i<34;i++) {
        int wx=i*84-(camera/2)%84;
        int wave=(int)((now/110u+(Uint32)(i*7))%24u);
        fill(r,wx,GROUND_Y+36+wave,52,5,152,223,234);
        fill(r,wx+16,GROUND_Y+92-wave/2,35,4,91,195,228);
    }
    w24Island(r,0,1600,camera,0);
    w24Island(r,2320,1470,camera,1);
    w24Island(r,4550,W24_WIDTH-4550,camera,2);
    /* Walkable timber bridges across both formerly unjumpable ocean gaps.
       Collision is handled by w24Land(), at the same GROUND_Y height. */
    const int bridgeStarts[2]={1600,3790};
    const int bridgeEnds[2]={2320,4550};
    for(int b=0;b<2;b++) {
        int left=bridgeStarts[b]-camera;
        int width=bridgeEnds[b]-bridgeStarts[b];
        fill(r,left,GROUND_Y,width,18,153,103,62);
        fill(r,left,GROUND_Y,width,7,224,177,104);
        for(int k=0;k<width;k+=56) {
            fill(r,left+k,GROUND_Y+8,5,12,93,62,44);
            fill(r,left+k+13,GROUND_Y+4,6,4,98,77,54);
        }
        for(int k=70;k<width;k+=150)
            fill(r,left+k,GROUND_Y+18,13,78,105,75,54);
    }
    /* Sailing landmarks and fortress are scenery, not collision shapes. */
    int fortress=5710-camera;
    fill(r,fortress,GROUND_Y-335,330,335,93,100,133);
    fill(r,fortress+30,GROUND_Y-430,80,95,117,122,153);
    fill(r,fortress+210,GROUND_Y-430,80,95,117,122,153);
    fill(r,fortress+123,GROUND_Y-380,83,95,75,84,120);
    for(int j=0;j<4;j++) {
        fill(r,fortress+j*81,GROUND_Y-354,50,28,128,130,164);
        fill(r,fortress+46+j*73,GROUND_Y-300,14,42,235,207,119);
    }
    for(int i=0;i<W24_PLATFORM_COUNT;i++) {
        SDL_Rect p=w24Platforms[i];
        int xx=p.x-camera;
        if(xx+p.w<0 || xx>SCREEN_W)continue;
        fill(r,xx,p.y,p.w,p.h,143,113,72);
        fill(r,xx,p.y,p.w,8,79,166,89);
        fill(r,xx+8,p.y+14,p.w-16,5,104,83,58);
    }
    for(int i=0;i<W24_PEARL_COUNT;i++)
        if(!pearls[i])w24Pearl(r,w24Pearls[i].x-camera,w24Pearls[i].y);
    for(int i=0;i<W24_CRAB_COUNT;i+=2) {
        int xx=w24CrabX(i,now)-camera;
        if(xx>-60 && xx<SCREEN_W+60)w24Crab(r,xx,GROUND_Y-46);
    }
    if(!gotSSD)w21DrawTinySSD(r,ssd,5365-camera,365);
    /* Pearl gate only disappears after all three islands are explored. */
    int count=pearls[0]+pearls[1]+pearls[2];
    if(count<3) {
        fill(r,5660-camera,GROUND_Y-245,35,245,52,107,178);
        fill(r,5654-camera,GROUND_Y-250,47,18,153,221,237);
        for(int j=0;j<3;j++)w24Pearl(r,5677-camera,GROUND_Y-213+j*65);
    }
    /* Lightning flashes on a predictable timer. */
    int lightning=((now/16u)%160u)<66u;
    int storm=5880-camera;
    fill(r,storm-45,GROUND_Y-330,165,42,57,77,112);
    fill(r,storm-16,GROUND_Y-357,110,48,72,91,124);
    if(lightning) {
        fill(r,storm+35,GROUND_Y-260,22,120,255,231,107);
        fill(r,storm+17,GROUND_Y-153,43,31,255,239,148);
        fill(r,storm+25,GROUND_Y-124,22,124,255,231,107);
    }
    if(sailing) {
        int bob=(int)((now/130u)%8u);
        w24Boat(r,boatX-65-camera,GROUND_Y-30+bob);
    }
    int flag=6130-camera;
    fill(r,flag,GROUND_Y-325,9,325,250,244,226);
    fill(r,flag+9,GROUND_Y-322,102,60,248,213,115);
    fill(r,flag+22,GROUND_Y-304,35,15,211,86,76);
    fill(r,12,12,330,54,27,87,126);
    drawText(r,"WORLD 2-4",24,28,3);
    char hud[32];
    SDL_snprintf(hud,sizeof(hud),"PEARLS: %d/3",count);
    drawText(r,hud,24,88,3);
    if(gotSSD)drawText(r,"SSD FOUND",24,130,2);
}

static int w24Move(float *x,float *y,float *vy,int *facing,int *frame,
                   int hold,int trigger,float stickX,int pearls)
{
    int left=(hold&VPAD_BUTTON_LEFT)||stickX< -0.25f;
    int right=(hold&VPAD_BUTTON_RIGHT)||stickX>0.25f;
    int moving=0;
    if(left&&!right){*x-=5.6f;*facing=0;moving=1;}
    if(right&&!left){*x+=5.6f;*facing=1;moving=1;}
    if(*x<0)*x=0;
    if(*x>W24_WIDTH-PLAYER_W)*x=W24_WIDTH-PLAYER_W;
    /* The fortress gate stays closed until all three pearls are found. */
    if(pearls<3 && *x>5610.0f)*x=5610.0f;
    float oldBottom=*y+PLAYER_H;
    int grounded=(*vy>=0 && w24Land(*x+PLAYER_W/2.0f) &&
                 oldBottom>=GROUND_Y-2 && oldBottom<=GROUND_Y+3);
    for(int i=0;i<W24_PLATFORM_COUNT;i++) {
        SDL_Rect p=w24Platforms[i];
        if(*vy>=0 && *x+PLAYER_W>p.x && *x<p.x+p.w &&
           oldBottom>=p.y-3 && oldBottom<=p.y+5)grounded=1;
    }
    if(grounded && (trigger&(VPAD_BUTTON_A|VPAD_BUTTON_B)))*vy=-18.0f;
    *vy+=0.55f;
    if(*vy>15.0f)*vy=15.0f;
    *y+=*vy;
    if(*vy>=0) {
        float landing=SCREEN_H+100.0f;
        if(w24Land(*x+PLAYER_W/2.0f))landing=GROUND_Y-PLAYER_H;
        for(int i=0;i<W24_PLATFORM_COUNT;i++) {
            SDL_Rect p=w24Platforms[i];
            if(*x+PLAYER_W>p.x && *x<p.x+p.w &&
               oldBottom<=p.y+5 && *y+PLAYER_H>=p.y &&
               p.y-PLAYER_H<landing)landing=p.y-PLAYER_H;
        }
        if(*y>=landing){*y=landing;*vy=0;}
    }
    if(moving && *vy==0.0f)(*frame)++;else *frame=0;
    return *vy!=0.0f ? (*vy<0?5:6) : (moving?1+((*frame/4)%4):0);
}
static void w24Reset(float *x,float *y,float *vy,int *facing,int *frame,
                     int pearls[W24_PEARL_COUNT],int *ssd,int *checkpoint,
                     int *sailing,Uint32 *safeUntil)
{
    *x=90.0f;*y=GROUND_Y-PLAYER_H;*vy=0.0f;
    *facing=1;*frame=0;*ssd=0;*checkpoint=90;*sailing=0;*safeUntil=0;
    memset(pearls,0,sizeof(int)*W24_PEARL_COUNT);
}

static void resetGame(float *x, float *y, float *vy, int coinsTaken[],
                      int *coinCount, int *animFrame, int *facingRight)
{
    *x = 90.0f;
    *y = GROUND_Y - PLAYER_H;
    *vy = 0.0f;
    *coinCount = 0;
    *animFrame = 0;
    *facingRight = 1;
    memset(coinsTaken, 0, sizeof(int) * COIN_COUNT);
}


/* Nine original 2014-inspired courses. All artwork is generated with SDL,
   requiring no new image files. These are independent playable levels. */
#define RETRO9_W 4800
#define RETRO9_ITEMS 12
static const char *retro9Names[9]={
 "WII FIT U", "TROPICAL FREEZE", "MARIO KART 8", "WII SPORTS CLUB",
 "HYRULE WARRIORS", "BAYONETTA 2", "SMASH BROS WII U",
 "CAPTAIN TOAD", "NES REMIX PACK"
};
typedef struct {
 float x,y,vy; int facing,frame,score,health,items[RETRO9_ITEMS];
 Uint32 invulnerableUntil;
} Retro9State;
static void retro9Reset(Retro9State *s) {
 memset(s,0,sizeof(*s)); s->x=75; s->y=GROUND_Y-PLAYER_H;
 s->facing=1;s->health=3;
}
static SDL_Rect retro9Platform(int level,int i) {
 SDL_Rect p={300+i*295, 470-((i*3+level)%4)*39, 130,26};
 return p;
}
static int retro9EnemyX(int level,int i,Uint32 tick) {
 int shift=(int)((tick/34+(Uint32)(i*19+level*11))%95);
 return 650+i*480+(shift<48?shift:95-shift);
}
static int retro9Move(Retro9State *s,int level,int hold,int trigger,float stick) {
 int left=(hold&VPAD_BUTTON_LEFT)||stick<-.25f;
 int right=(hold&VPAD_BUTTON_RIGHT)||stick>.25f;
 int moving=0;
 if(left&&!right){s->x-=5.5f;s->facing=0;moving=1;}
 if(right&&!left){s->x+=5.5f;s->facing=1;moving=1;}
 if(s->x<0)s->x=0;
 if(s->x>RETRO9_W-PLAYER_W)s->x=RETRO9_W-PLAYER_W;
 float oldBottom=s->y+PLAYER_H;
 int grounded=(s->vy>=0 && oldBottom>=GROUND_Y-3 && oldBottom<=GROUND_Y+4);
 for(int i=0;i<15;i++) {
  SDL_Rect p=retro9Platform(level,i);
  if(s->vy>=0 && s->x+PLAYER_W>p.x && s->x<p.x+p.w &&
     oldBottom>=p.y-3 && oldBottom<=p.y+5)grounded=1;
 }
 if(grounded&&(trigger&(VPAD_BUTTON_A|VPAD_BUTTON_B)))s->vy=-16.5f;
 s->vy+=.60f;if(s->vy>16)s->vy=16;
 s->y+=s->vy;
 if(s->vy>=0) {
  float landing=GROUND_Y-PLAYER_H;
  for(int i=0;i<15;i++) {
   SDL_Rect p=retro9Platform(level,i);
   if(s->x+PLAYER_W>p.x && s->x<p.x+p.w && oldBottom<=p.y+5 &&
      s->y+PLAYER_H>=p.y && p.y-PLAYER_H<landing)landing=p.y-PLAYER_H;
  }
  if(s->y>=landing){s->y=landing;s->vy=0;}
 }
 if(moving&&s->vy==0)s->frame++;else s->frame=0;
 return s->vy!=0?(s->vy<0?5:6):(moving?1+(s->frame/4)%4:0);
}
static void retro9Draw(SDL_Renderer *r,int level,int camera,
                       const Retro9State *s,Uint32 now) {
 static const Uint8 sky[9][3]={{117,212,237},{92,183,228},{75,124,215},
 {84,170,225},{91,84,160},{45,34,88},{213,117,104},
 {220,191,112},{34,41,93}};
 SDL_SetRenderDrawColor(r,sky[level][0],sky[level][1],sky[level][2],255);
 SDL_RenderClear(r);
 for(int i=0;i<22;i++) {
  int xx=i*280-camera/4;
  if(level==0) {fill(r,xx,410,130,180,95,176,125);fill(r,xx+30,365,70,45,192,235,180);}
  else if(level==1) {fill(r,xx+20,315,35,280,99,69,55);fill(r,xx-30,295,130,60,51,153,83);}
  else if(level==2) {fill(r,xx,390,155,195,55,71,122);fill(r,xx+18,415,18,25,240,221,97);}
  else if(level==3) {fill(r,xx,435,205,135,37,116,77);fill(r,xx+10,460,180,8,236,246,249);}
  else if(level==4) {fill(r,xx,320,55,270,91,79,94);fill(r,xx-40,300,135,45,143,136,152);}
  else if(level==5) {fill(r,xx,350,120,240,65,51,91);fill(r,xx+40,300,25,290,161,90,183);}
  else if(level==6) {fill(r,xx,390,190,200,132,74,79);fill(r,xx+30,355,120,45,239,183,103);}
  else if(level==7) {fill(r,xx,380,190,210,190,136,67);fill(r,xx+50,345,80,45,240,211,131);}
  else {fill(r,xx,405,155,185,27,48,119);fill(r,xx+20,420,110,18,73,125,230);}
 }
 fill(r,0,GROUND_Y,SCREEN_W,SCREEN_H-GROUND_Y,
      level==7?151:level==5?68:105,level==7?111:level==5?56:83,level==7?63:level==5?100:71);
 fill(r,0,GROUND_Y,SCREEN_W,16,
      level==1?224:level==7?239:75,level==1?244:level==7?188:194,
      level==1?255:level==7?100:94);
 for(int i=0;i<15;i++) {
  SDL_Rect p=retro9Platform(level,i);
  int xx=p.x-camera;
  if(xx>SCREEN_W||xx+p.w<0)continue;
  fill(r,xx,p.y,p.w,p.h,level==7?180:58,level==7?133:101,level==7?68:166);
  fill(r,xx,p.y,p.w,6,level==7?255:140,level==7?214:213,level==7?112:233);
  if(level==7)for(int k=0;k<3;k++)fill(r,xx+k*40+9,p.y+9,8,8,107,70,43);
 }
 for(int i=0;i<RETRO9_ITEMS;i++)if(!s->items[i]) {
  int xx=380+i*355-camera, yy=385-((i*3+level)%4)*39;
  fill(r,xx-12,yy-12,24,24,244,171,31);
  fill(r,xx-7,yy-8,14,16,255,239,104);
 }
 for(int i=0;i<8;i++) {
  int xx=retro9EnemyX(level,i,now)-camera;
  if(xx>-70&&xx<SCREEN_W+70) {
   if(level==2) {fill(r,xx,GROUND_Y-34,54,30,240,68,62);fill(r,xx+7,GROUND_Y-9,12,12,35,38,51);fill(r,xx+37,GROUND_Y-9,12,12,35,38,51);}
   else if(level==3) {fill(r,xx,GROUND_Y-55,18,55,240,240,240);fill(r,xx+19,GROUND_Y-55,18,55,240,240,240);}
   else if(level==7) {fill(r,xx,GROUND_Y-44,42,44,171,92,47);fill(r,xx+7,GROUND_Y-33,8,8,255,255,255);}
   else {
    /* Non-Mario worlds use a neutral moving hazard, never a Goomba. */
    fill(r,xx+4,GROUND_Y-39,44,34,74,83,105);
    fill(r,xx+9,GROUND_Y-34,34,8,157,175,195);
    fill(r,xx+10,GROUND_Y-8,10,8,34,39,52);
    fill(r,xx+34,GROUND_Y-8,10,8,34,39,52);
    fill(r,xx+20,GROUND_Y-23,12,9,235,187,73);
   }
  }
 }
 int flag=RETRO9_W-125-camera;
 fill(r,flag,GROUND_Y-295,8,295,243,242,226);
 fill(r,flag+8,GROUND_Y-292,88,49,244,78,91);
 fill(r,12,12,480,60,29,55,105);
 char hud[80];SDL_snprintf(hud,sizeof(hud),"2014 - LEVEL %d OF 9",level+1);
 drawText(r,hud,23,28,3);
 drawText(r,retro9Names[level],24,85,3);
 SDL_snprintf(hud,sizeof(hud),"TOKENS: %d/12",s->score);
 drawText(r,hud,24,127,2);
 SDL_snprintf(hud,sizeof(hud),"HEALTH: %d",s->health);
 drawText(r,hud,24,155,2);
 if(level==0)drawText(r,"FITNESS HILLS",24,190,2);
 if(level==1)drawText(r,"FROZEN JUNGLE",24,190,2);
 if(level==2)drawText(r,"DODGE THE KARTS",24,190,2);
 if(level==3)drawText(r,"STADIUM SPRINT",24,190,2);
 if(level==4)drawText(r,"CASTLE SIEGE",24,190,2);
 if(level==5)drawText(r,"MIDNIGHT TOWERS",24,190,2);
 if(level==6)drawText(r,"ARENA RUSH",24,190,2);
 if(level==7)drawText(r,"TREASURE TEMPLE",24,190,2);
 if(level==8)drawText(r,"RETRO GAUNTLET",24,190,2);
}


/* WORLD 4: 2015 Wii U releases, in the supplied list order.
   Thirteen playable courses, each with its own themed palette and an SSD. */
#define W4_LEVELS 13
static const char *w4Names[W4_LEVELS]={
 "KIRBY RAINBOW CURSE", "MARIO PARTY 10", "MARIO VS DONKEY KONG",
 "SPLATOON", "ART ACADEMY", "SUPER MARIO MAKER",
 "NEW SUPER MARIO BROS U", "YOSHIS WOOLLY WORLD",
 "FATAL FRAME", "ANIMAL CROSSING AMIIBO",
 "MARIO TENNIS", "XENOBLADE CHRONICLES X", "DEVILS THIRD"
};
static const char *w4Themes[W4_LEVELS]={
 "RAINBOW CLAY", "PARTY BOARD", "TOY FACTORY", "INK ARENA",
 "PAINT STUDIO", "BUILDERS COURSE", "MUSHROOM HILLS", "YARN VALLEY",
 "HAUNTED HALL", "VILLAGE FESTIVAL", "TENNIS COURT", "ALIEN PLANET",
 "FORTRESS RUN"
};
static const Uint8 w4Sky[W4_LEVELS][3]={
 {128,195,241},{244,170,92},{180,135,92},{57,76,125},
 {237,216,183},{115,199,252},{103,188,244},{184,215,238},
 {43,47,70},{145,212,172},{85,167,223},{96,121,134},{84,77,79}
};
static void w4Draw(SDL_Renderer *r,int level,int camera,
                   const Retro9State *s,Uint32 now,int ssdTaken) {
 SDL_SetRenderDrawColor(r,w4Sky[level][0],w4Sky[level][1],w4Sky[level][2],255);
 SDL_RenderClear(r);
 for(int i=0;i<24;i++) {
  int xx=i*240-camera/4;
  int yy=345+(i%3)*40;
  /* Distinct visual motifs for each 2015 title, drawn without new assets. */
  switch(level) {
   case 0: fill(r,xx,yy,140,120,244,122,176);fill(r,xx+35,yy-30,65,65,255,220,90);break;
   case 1: fill(r,xx,yy,135,145,243,213,77);fill(r,xx+48,yy-45,35,45,232,89,82);break;
   case 2: fill(r,xx,yy-60,150,190,163,99,58);fill(r,xx+20,yy-35,90,45,239,190,71);break;
   case 3: fill(r,xx,yy,150,155,220,77,157);fill(r,xx+35,yy-30,90,75,87,218,173);break;
   case 4: fill(r,xx,yy,150,155,248,242,221);fill(r,xx+30,yy+12,55,100,120,166,216);break;
   case 5: fill(r,xx,yy,135,155,180,117,64);fill(r,xx+20,yy-38,95,45,245,207,72);break;
   case 6: fill(r,xx,yy,150,140,72,177,93);fill(r,xx+40,yy-40,70,55,239,239,218);break;
   case 7: fill(r,xx,yy,140,140,234,180,190);fill(r,xx+25,yy-35,90,55,255,236,159);break;
   case 8: fill(r,xx,yy-80,155,225,62,62,78);fill(r,xx+30,yy-45,45,95,140,155,163);break;
   case 9: fill(r,xx,yy,135,135,91,171,108);fill(r,xx+35,yy-45,65,60,222,172,102);break;
   case 10:fill(r,xx,yy,145,130,66,151,95);fill(r,xx+50,yy-25,12,100,249,249,249);break;
   case 11:fill(r,xx,yy-75,150,215,86,116,110);fill(r,xx+30,yy-110,50,70,181,213,192);break;
   default:fill(r,xx,yy-80,150,220,86,86,91);fill(r,xx+25,yy-120,95,55,145,143,141);break;
  }
 }
 fill(r,0,GROUND_Y,SCREEN_W,SCREEN_H-GROUND_Y,70,82,92);
 fill(r,0,GROUND_Y,SCREEN_W,15,level==3?220:level==7?237:112,
      level==3?78:level==7?181:183,level==3?159:level==7?203:103);
 for(int i=0;i<15;i++) {
  SDL_Rect p=retro9Platform(level,i);int xx=p.x-camera;
  if(xx+p.w<0||xx>SCREEN_W)continue;
  fill(r,xx,p.y,p.w,p.h,level==7?230:108,level==7?174:113,level==7?195:127);
  fill(r,xx,p.y,p.w,6,235,214,127);
 }
 for(int i=0;i<RETRO9_ITEMS;i++)if(!s->items[i]) {
  int xx=380+i*355-camera,yy=385-((i*3+level)%4)*39;
  fill(r,xx-12,yy-12,24,24,244,178,39);
  fill(r,xx-6,yy-6,12,12,255,237,128);
 }
 for(int i=0;i<8;i++) {
  int xx=retro9EnemyX(level,i,now)-camera;
  if(xx<-70||xx>SCREEN_W+70)continue;
  /* Neutral hazards, not Goombas, even in the Mario-themed levels. */
  fill(r,xx+3,GROUND_Y-40,45,36,57,64,82);
  fill(r,xx+8,GROUND_Y-34,35,9,183,185,201);
  fill(r,xx+11,GROUND_Y-9,10,9,28,33,44);
  fill(r,xx+34,GROUND_Y-9,10,9,28,33,44);
 }
 /* One collectible SSD per level, placed above a reachable platform. */
 if(!ssdTaken) {
  int xx=2040-camera;
  fill(r,xx,340,55,72,44,52,67);
  fill(r,xx+7,348,41,55,162,184,206);
  fill(r,xx+14,360,27,25,54,66,80);
  fill(r,xx+22,367,11,11,231,198,69);
 }
 int fx=RETRO9_W-125-camera;
 fill(r,fx,GROUND_Y-295,8,295,240,238,215);
 fill(r,fx+8,GROUND_Y-292,88,49,240,86,105);
 fill(r,12,12,540,60,24,49,84);
 char hud[100];SDL_snprintf(hud,sizeof(hud),"WORLD 4-%d / 13",level+1);
 drawText(r,hud,24,28,3);
 drawText(r,w4Names[level],24,84,2);
 drawText(r,w4Themes[level],24,117,2);
 SDL_snprintf(hud,sizeof(hud),"TOKENS: %d/12  HP: %d",s->score,s->health);
 drawText(r,hud,24,150,2);
 drawText(r,ssdTaken?"SSD COLLECTED":"FIND THE SSD",24,182,2);
}

/* World 5: eight 2016-themed courses; existing SDL art and SSD texture. */
#define W5_LEVELS 8
static const char *w5Names[W5_LEVELS]={
 "TWILIGHT PRINCESS HD", "POKKEN TOURNAMENT", "STAR FOX ZERO",
 "STAR FOX GUARD", "MINI MARIO AND FRIENDS", "RIO 2016",
 "TOKYO MIRAGE SESSIONS", "PAPER MARIO COLOR SPLASH"
};
static const char *w5Themes[W5_LEVELS]={
 "TWILIGHT CASTLE", "BATTLE ARENA", "SPACE FLIGHT", "SECURITY BASE",
 "MINI TOY FACTORY", "OLYMPIC TRACK", "NEON CITY", "PAINTED WORLD"
};
static const Uint8 w5Sky[W5_LEVELS][3]={
 {61,59,109},{131,78,139},{29,62,117},{49,93,117},
 {195,149,103},{98,197,229},{116,63,143},{245,176,103}
};
/* One persistent miniboss per course. Stomp twice to defeat it. */
#define MINI_X 4380
/* Boss projectile: one rolling Goomba-like hazard at a time. It spawns near
   the boss on a repeatable timer, only when the player approaches the arena. */
static int worldBossShotX(Uint32 now,int bossX) {
 Uint32 phase=now%3400u;
 if(phase<650u)return bossX+90; /* windup */
 return bossX-((int)(phase-650u)*360/1000);
}
static void worldBossShotDraw(SDL_Renderer *r,int camera,Uint32 now,int bossX,int hp) {
 if(hp<=0)return;
 int sx=worldBossShotX(now,bossX)-camera;
 if(sx<-55||sx>SCREEN_W+55)return;
 int y=GROUND_Y-34;
 fill(r,sx,y,34,34,140,75,44); /* body */
 fill(r,sx+5,y+7,8,10,255,239,211);fill(r,sx+21,y+7,8,10,255,239,211);
 fill(r,sx+9,y+11,4,5,25,19,20);fill(r,sx+23,y+11,4,5,25,19,20);
 fill(r,sx+2,y+29,12,6,62,38,33);fill(r,sx+21,y+29,12,6,62,38,33);
}
static void worldBossShotHit(Retro9State *p,int *hp,Uint32 now) {
 if(*hp<=0 || p->x<MINI_X-850 || now<p->invulnerableUntil)return;
 int sx=worldBossShotX(now,MINI_X);
 if(p->x+PLAYER_W>sx && p->x<sx+34 &&
    p->y+PLAYER_H>GROUND_Y-34 && p->y<GROUND_Y) {
  p->health--;p->invulnerableUntil=now+1600;p->vy=-10;
  if(p->health<=0){retro9Reset(p);*hp=2;}
 }
}
static void miniDraw(SDL_Renderer *r,int camera,int hp,Uint32 now) {
 if(hp<=0)return;
 int x=MINI_X-camera, y=GROUND_Y-80;
 int stride=(int)((now/170u)%4u);
 if(x < -100 || x > SCREEN_W+100)return;
 fill(r,x+6,y+20,66,52,75,42,100);
 fill(r,x+16,y,46,32,115,58,142); /* head */
 fill(r,x-3,y+27+stride*2,14,31,75,42,100); /* left arm */
 fill(r,x+67,y+27+(3-stride)*2,14,31,75,42,100); /* right arm */
 fill(r,x+13,y+67,18,13,49,31,75); /* left leg */
 fill(r,x+48,y+67,18,13,49,31,75); /* right leg */
 fill(r,x+7,y+7,64,18,175,92,211);
 fill(r,x+11,y+35,18,13,255,233,114);
 fill(r,x+48,y+35,18,13,255,233,114);
 fill(r,x+17,y+39,7,7,35,31,65);
 fill(r,x+54,y+39,7,7,35,31,65);
 fill(r,x+13,y+64,52,9,29,24,48);
 for(int i=0;i<hp;i++)fill(r,x+i*25,y-19,20,10,239,69,93);
 worldBossShotDraw(r,camera,now,MINI_X,hp);
}
static void miniUpdate(Retro9State *s,int *hp,Uint32 now) {
 if(*hp<=0)return;
 int overlap=s->x+PLAYER_W>MINI_X && s->x<MINI_X+78 &&
             s->y+PLAYER_H>GROUND_Y-80 && s->y<GROUND_Y;
 if(!overlap || now<s->invulnerableUntil)return;
 if(s->vy>0 && s->y+PLAYER_H<GROUND_Y-45) {
  --*hp;s->vy=-13.0f;s->invulnerableUntil=now+500;
 } else {
  --s->health;s->vy=-11.0f;s->invulnerableUntil=now+1600;
  if(s->health<=0){retro9Reset(s);*hp=2;}
 }
}
static void w5Draw(SDL_Renderer *r,int level,int camera,const Retro9State *s,
                   Uint32 now,int ssdTaken,SDL_Texture *ssd,int miniHP) {
 SDL_SetRenderDrawColor(r,w5Sky[level][0],w5Sky[level][1],w5Sky[level][2],255);
 SDL_RenderClear(r);
 for(int i=0;i<25;i++) {
  int x=i*245-camera/4, y=320+(i%3)*35;
  switch(level) {
   case 0: fill(r,x,y-110,150,270,61,55,91);fill(r,x+55,y-155,35,55,123,107,155);break;
   case 1: fill(r,x,y,155,150,94,64,133);fill(r,x+22,y-25,110,25,216,116,185);break;
   case 2: fill(r,x,y+25,190,110,40,82,147);fill(r,x+60,y-70,70,105,170,208,235);break;
   case 3: fill(r,x,y-50,145,230,65,108,126);fill(r,x+42,y-90,50,60,209,217,229);break;
   case 4: fill(r,x,y,155,155,170,110,72);fill(r,x+30,y-25,85,50,230,180,95);break;
   case 5: fill(r,x,y+15,160,135,57,169,112);fill(r,x+45,y-35,60,55,246,226,139);break;
   case 6: fill(r,x,y-75,155,250,65,50,113);fill(r,x+24,y-115,95,40,226,92,200);break;
   default:fill(r,x,y-35,165,195,239,213,166);fill(r,x+32,y-65,100,55,224,85,106);break;
  }
 }
 fill(r,0,GROUND_Y,SCREEN_W,SCREEN_H-GROUND_Y,67,67,83);
 fill(r,0,GROUND_Y,SCREEN_W,15,level==7?239:88,level==7?158:167,level==7?75:173);
 for(int i=0;i<15;i++) {
  SDL_Rect p=retro9Platform(level,i);int x=p.x-camera;
  if(x+p.w<0||x>SCREEN_W)continue;
  fill(r,x,p.y,p.w,p.h,98,99,154);fill(r,x,p.y,p.w,6,190,185,228);
 }
 for(int i=0;i<RETRO9_ITEMS;i++)if(!s->items[i]) {
  int x=380+i*355-camera,y=385-((i*3+level)%4)*39;
  fill(r,x-12,y-12,24,24,240,180,51);fill(r,x-6,y-6,12,12,255,238,131);
 }
 for(int i=0;i<8;i++) {
  int x=retro9EnemyX(level,i,now)-camera;
  if(x>-70&&x<SCREEN_W+70) {
   fill(r,x,GROUND_Y-42,48,42,52,63,88);
   fill(r,x+8,GROUND_Y-32,32,8,174,196,224);
  }
 }
 if(!ssdTaken)w21DrawTinySSD(r,ssd,2040-camera,340);
 if(level==W5_LEVELS-1)miniDraw(r,camera,miniHP,now);
 int flag=RETRO9_W-125-camera;
 fill(r,flag,GROUND_Y-295,8,295,240,238,215);
 fill(r,flag+8,GROUND_Y-292,88,49,240,86,105);
 fill(r,12,12,570,60,24,49,84);
 char hud[100];SDL_snprintf(hud,sizeof(hud),"WORLD 5-%d / 8",level+1);
 drawText(r,hud,24,28,3);drawText(r,w5Names[level],24,85,2);
 drawText(r,w5Themes[level],24,117,2);
 SDL_snprintf(hud,sizeof(hud),"TOKENS: %d/12  HP: %d",s->score,s->health);
 drawText(r,hud,24,150,2);
 drawText(r,ssdTaken?"SSD COLLECTED":"FIND THE SSD",24,182,2);
 if(level==W5_LEVELS-1 && miniHP>0)drawText(r,"STOMP THE WORLD BOSS",24,215,2);
}


/* WORLD 6-1: fictional NAND corruption. No actual NAND I/O. */
typedef struct {
 Retro9State player;
 int bossHP, greatSSD, phase;
 Uint32 bossSafeUntil, laserStart;
 int bossX;
} Final6State;
#define FINAL6_BOSS_X 4260
#define FINAL6_BOSS_Y (GROUND_Y-156)
static void final6Reset(Final6State *f) {
 memset(f,0,sizeof(*f));retro9Reset(&f->player);
 f->player.health=5;f->bossHP=6;f->laserStart=SDL_GetTicks();f->bossX=FINAL6_BOSS_X;
}
static void final6Update(Final6State *f,Uint32 now) {
 Retro9State *p=&f->player;
 /* Patrol in a bounded arena. Faster in later phases. */
 if(f->bossHP>0) {
  int speed=f->bossHP>4?2:(f->bossHP>2?3:4);
  Uint32 tick=(now-f->laserStart)/16u;
  int distance=(int)((tick*(Uint32)speed)%840u);
  f->bossX=FINAL6_BOSS_X-420+(distance<420?distance:840-distance);
 }
 if(f->bossHP<=0) {
  if(!f->greatSSD && p->x+PLAYER_W>f->bossX+20 &&
     p->x<f->bossX+130 && p->y+PLAYER_H>GROUND_Y-110)
   f->greatSSD=1;
  return;
 }
 f->phase=f->bossHP>4?1:(f->bossHP>2?2:3);
 /* Stomp from above. A bounce and brief cooldown prevent multi-hits. */
 if(now>=f->bossSafeUntil && p->x+PLAYER_W>f->bossX &&
    p->x<f->bossX+148 && p->y+PLAYER_H>FINAL6_BOSS_Y &&
    p->y<GROUND_Y && p->vy>0 && p->y+PLAYER_H<FINAL6_BOSS_Y+48) {
  --f->bossHP;p->vy=-17.0f;f->bossSafeUntil=now+950;
 } else if(now>=p->invulnerableUntil &&
    p->x+PLAYER_W>f->bossX && p->x<f->bossX+148 &&
    p->y+PLAYER_H>FINAL6_BOSS_Y+40 && p->y<GROUND_Y) {
  --p->health;p->vy=-13.0f;p->invulnerableUntil=now+1700;
 }
 /* Three increasingly quick horizontal corruption pulses. */
 if(f->bossHP>0 && now>=p->invulnerableUntil) {
  Uint32 period=f->phase==1?2300u:(f->phase==2?1700u:1250u);
  Uint32 elapsed=(Uint32)(now-f->laserStart)%period;
  int laserX=f->bossX-(int)((elapsed*1150u)/period);
  if(elapsed<period-180 && p->x+PLAYER_W>laserX && p->x<laserX+44 &&
     p->y+PLAYER_H>GROUND_Y-56 && p->y<GROUND_Y-12) {
   --p->health;p->vy=-11.0f;p->invulnerableUntil=now+1700;
  }
 }
 if(p->health<=0)final6Reset(f);
}
static void final6Draw(SDL_Renderer *r,const Final6State *f,int camera,
                       Uint32 now,SDL_Texture *ssd) {
 const Retro9State *p=&f->player;
 SDL_SetRenderDrawColor(r,20,16,43,255);SDL_RenderClear(r);
 for(int i=0;i<26;i++) {
  int x=i*230-camera/5;
  fill(r,x,180+(i%4)*45,120,235,38+(i%3)*13,29,65);
  fill(r,x+20,200+(i%4)*45,20,110,126,35,91);
  fill(r,x+76,260+(i%4)*45,13,95,185,46,104);
 }
 fill(r,0,GROUND_Y,SCREEN_W,SCREEN_H-GROUND_Y,38,39,58);
 fill(r,0,GROUND_Y,SCREEN_W,11,213,45,88);
 for(int i=0;i<15;i++) {
  SDL_Rect plat=retro9Platform(0,i);int x=plat.x-camera;
  if(x+plat.w>=0&&x<SCREEN_W) {
   fill(r,x,plat.y,plat.w,plat.h,87,40,105);
   fill(r,x,plat.y,plat.w,6,236,56,104);
  }
 }
 for(int i=0;i<13;i++) {
  int x=380+i*330-camera;
  if(x>=0&&x<SCREEN_W) {
   fill(r,x,GROUND_Y-60-(i%3)*35,18,18,252,68,112);
   fill(r,x+5,GROUND_Y-55-(i%3)*35,8,8,255,216,236);
  }
 }
 if(f->bossHP>0) {
  int x=f->bossX-camera,y=FINAL6_BOSS_Y;
  int stride=(int)((now/125u)%4u);
  int swing=(stride==0?12:(stride==2?-12:0));
  /* Articulated NAND Destroyer: two animated legs, arms, torso and head. */
  fill(r,x+28+swing,y+112,31,44,73,25,111); /* left leg */
  fill(r,x+90-swing,y+112,31,44,73,25,111); /* right leg */
  fill(r,x+18+swing,y+144,48,12,28,20,50); /* left boot */
  fill(r,x+85-swing,y+144,48,12,28,20,50); /* right boot */
  fill(r,x-10-swing,y+55,31,66,92,30,132); /* left arm */
  fill(r,x+127+swing,y+55,31,66,92,30,132); /* right arm */
  fill(r,x-16-swing,y+105,34,25,218,54,110); /* left fist */
  fill(r,x+132+swing,y+105,34,25,218,54,110); /* right fist */
  fill(r,x+22,y+62,105,76,104,25,107); /* torso */
  fill(r,x+40,y+80,69,40,206,42,99); /* core */
  fill(r,x+35,y+4,80,67,124,36,147); /* head */
  fill(r,x+26,y+22,96,12,228,50,115); /* visor */
  fill(r,x+47,y+27,17,14,255,235,232); /* eyes */
  fill(r,x+86,y+27,17,14,255,235,232);
  fill(r,x+52,y+30,8,9,36,21,50);
  fill(r,x+91,y+30,8,9,36,21,50);
  fill(r,x+57,y+52,36,7,27,16,44); /* mouth */
  for(int i=0;i<f->bossHP;i++)fill(r,x+i*25,y-30,19,13,251,65,95);
  Uint32 period=f->phase==1?2300u:(f->phase==2?1700u:1250u);
  Uint32 elapsed=(Uint32)(now-f->laserStart)%period;
  int laserX=FINAL6_BOSS_X-(int)((elapsed*1150u)/period)-camera;
  if(elapsed<period-180) {
   fill(r,laserX,GROUND_Y-56,44,44,248,43,86);
   fill(r,laserX+9,GROUND_Y-45,26,23,255,170,192);
  }
 } else if(!f->greatSSD) {
  /* Reuse the existing SSD art, enlarged as the Great SSD. */
  drawSSD(r,ssd,f->bossX+15-camera,GROUND_Y-135);
  drawText(r,"TAKE THE GREAT SSD!",410,220,3);
 }
 fill(r,12,12,790,62,44,23,74);
 drawText(r,"WORLD 6-1: NAND FAILURE",24,28,3);
 char hud[90];SDL_snprintf(hud,sizeof(hud),"HP: %d  BOSS: %d/6  PHASE: %d",
                                p->health,f->bossHP,f->phase);
 drawText(r,hud,24,94,3);
 drawText(r,f->bossHP>0?"STOMP THE NAND DESTROYER!":"THE GREAT SSD IS FREE!",24,132,2);
 drawText(r,"FICTIONAL NAND - REAL NAND IS SAFE",24,165,2);
 if(f->greatSSD)centerText(r,"GREAT SSD RECOVERED!",210,4);
}
/* Minibosses for earlier worlds use the same stomp mechanics as Worlds 3-5. */
static void earlyMiniDraw(SDL_Renderer *r,int camera,int worldX,int hp) {
 if(hp<=0)return;
 int x=worldX-camera,y=GROUND_Y-82;
 if(x<-100||x>SCREEN_W+100)return;
 fill(r,x,y,76,82,86,43,109);
 fill(r,x+10,y+10,56,22,216,73,145);
 fill(r,x+12,y+43,16,13,252,234,203);
 fill(r,x+49,y+43,16,13,252,234,203);
 for(int i=0;i<hp;i++)fill(r,x+i*28,y-22,21,12,255,72,102);
}
static void earlyBossShotDraw(SDL_Renderer *r,int camera,int worldX,int hp,Uint32 now) {
 if(hp<=0)return;
 int sx=worldBossShotX(now,worldX)-camera;
 if(sx<-40||sx>SCREEN_W+40)return;
 fill(r,sx,GROUND_Y-32,32,32,138,76,43);
 fill(r,sx+6,GROUND_Y-26,7,8,255,245,220);
 fill(r,sx+20,GROUND_Y-26,7,8,255,245,220);
}
static int earlyBossShotHit(float px,float py,int worldX,int hp,Uint32 now,Uint32 safe) {
 if(hp<=0||now<safe||px<worldX-850)return 0;
 int sx=worldBossShotX(now,worldX);
 return px+PLAYER_W>sx && px<sx+32 && py+PLAYER_H>GROUND_Y-32 && py<GROUND_Y;
}
static int earlyMiniHit(float px,float py,float vy,int worldX,int *hp,Uint32 now,Uint32 *safe) {
 if(*hp<=0 || now<*safe)return 0;
 if(px+PLAYER_W<=worldX || px>=worldX+76 ||
    py+PLAYER_H<=GROUND_Y-82 || py>=GROUND_Y)return 0;
 *safe=now+1050;
 if(vy>0 && py+PLAYER_H<GROUND_Y-35){--*hp;return 1;}
 return -1;
}

/* Progress is stored on SD where possible, never in Wii U NAND. */
typedef struct { char magic[8]; int scene, retro9Level, w4Level, w5Level, character; } JumpSave;
static const char *jumpSavePaths[] = {
    "sd:/wiiu/apps/super-wii-u-jump/progress.dat",
    "sd:/wiiu/apps/super-wii-u-jump/save.dat",
    "progress.dat"
};
static int jumpLoad(JumpSave *s) {
    for (int i=0;i<3;i++) {
        FILE *f=fopen(jumpSavePaths[i],"rb");
        if (!f) continue;
        int ok=fread(s,1,sizeof(*s),f)==sizeof(*s); fclose(f);
        if (ok && memcmp(s->magic,"SWUJPR1",7)==0 &&
            (s->scene==1||s->scene==4||s->scene==11||s->scene==22||
             s->scene==23||s->scene==26||s->scene==40||s->scene==43||
             s->scene==53||s->scene==63) &&
            s->retro9Level>=0&&s->retro9Level<=8 &&
            s->w4Level>=0&&s->w4Level<=12 &&
            s->w5Level>=0&&s->w5Level<=7 &&
            s->character>=0&&s->character<=1) return 1;
    }
    return 0;
}
static int jumpSave(int scene,int retro9Level,int w4Level,int w5Level,int character) {
    JumpSave s={{0},scene,retro9Level,w4Level,w5Level,character};
    memcpy(s.magic,"SWUJPR1",7);
    for(int i=0;i<3;i++) {
        FILE *f=fopen(jumpSavePaths[i],"wb");
        if(!f) continue;
        int ok=fwrite(&s,1,sizeof(s),f)==sizeof(s);
        if(fflush(f)!=0) ok=0;
        fclose(f);
        if(ok) return 1;
    }
    return 0;
}
static void jumpFade(SDL_Renderer *r,Uint32 elapsed,const char *label) {
    if(elapsed>=800) return;
    Uint8 alpha=elapsed<180?(Uint8)(220-(elapsed*140/180)):
                elapsed<560?80:(Uint8)(80+(elapsed-560)*175/240);
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r,7,12,31,alpha);
    SDL_Rect full={0,0,SCREEN_W,SCREEN_H}; SDL_RenderFillRect(r,&full);
    if(elapsed<650 && label) centerText(r,label,335,4);
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
}
static void jumpBossBar(SDL_Renderer *r,int hp,int maxHP,int world) {
    if(maxHP<1||hp<=0) return;
    if(hp>maxHP) hp=maxHP;
    fill(r,385,65,510,43,15,22,44);
    fill(r,392,72,496,29,88,43,52);
    fill(r,392,72,(496*hp)/maxHP,29,242,80,99);
    char label[48];SDL_snprintf(label,sizeof(label),"WORLD %d BOSS",world);
    centerText(r,label,34,2);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) return 1;
    SDL_Window *window = SDL_CreateWindow("SUPER Wii U JUMP",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H, 0);
    if (!window) { SDL_Quit(); return 1; }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    /* World 1 soundtrack and title boot sound. Keep a visible diagnostic
       so missing content or audio-device failures cannot fail silently. */
    SDL_AudioSpec musicSpec;
    Uint8 *musicBuffer = NULL;
    Uint32 musicLength = 0;
    SDL_AudioDeviceID audioDevice = 0;
    int musicStatus = 0; /* 0=missing, 1=device failed, 2=ready, 3=queue failed */
    int hasMusic = SDL_LoadWAV("/vol/content/world1.wav", &musicSpec,
                               &musicBuffer, &musicLength) != NULL;
    if (!hasMusic) {
        SDL_Log("World 1 music load failed: %s", SDL_GetError());
    } else {
        SDL_AudioSpec wanted = musicSpec;
        wanted.callback = NULL;
        wanted.samples = 2048;
        /* No format changes: SDL converts to hardware format internally. */
        audioDevice = SDL_OpenAudioDevice(NULL, 0, &wanted, NULL, 0);
        if (audioDevice == 0) {
            musicStatus = 1;
            SDL_Log("World 1 audio device failed: %s", SDL_GetError());
        } else {
            musicStatus = 2;
            SDL_PauseAudioDevice(audioDevice, 0);
        }
    }

    /* Separate title music device: queue BEFORE the first menu frame. */
    SDL_AudioSpec bootSpec;
    Uint8 *bootBuffer = NULL;
    Uint32 bootLength = 0;
    SDL_AudioDeviceID bootDevice = 0;
    if (SDL_LoadWAV("/vol/content/boot.wav", &bootSpec,
                    &bootBuffer, &bootLength)) {
        SDL_AudioSpec desired = bootSpec;
        desired.callback = NULL;
        desired.samples = 1024;
        bootDevice = SDL_OpenAudioDevice(NULL, 0, &desired, NULL, 0);
        if (bootDevice) {
            SDL_QueueAudio(bootDevice, bootBuffer, bootLength);
            SDL_PauseAudioDevice(bootDevice, 0);
        } else SDL_Log("Boot audio device: %s", SDL_GetError());
    } else SDL_Log("Boot melody missing: %s", SDL_GetError());

    /* Original 22.05 kHz mono PCM effects, on a separate queue-based device. */
    enum { SFX_JUMP, SFX_PICKUP, SFX_HURT, SFX_BOSS_HIT,
           SFX_BOSS_SHOOT, SFX_LEVEL, SFX_VICTORY, SFX_LAND, SFX_COUNT };
    const char *sfxNames[SFX_COUNT] = {"jump", "pickup", "hurt", "boss_hit",
        "boss_shoot", "level", "victory", "land"};
    Uint8 *sfxData[SFX_COUNT] = {NULL};
    Uint32 sfxLength[SFX_COUNT] = {0};
    SDL_AudioDeviceID sfxDevice = 0;
    SDL_AudioSpec sfxWanted = {0};
    sfxWanted.freq = 22050;
    sfxWanted.format = AUDIO_S16SYS;
    sfxWanted.channels = 1;
    sfxWanted.samples = 512;
    sfxWanted.callback = NULL;
    sfxDevice = SDL_OpenAudioDevice(NULL, 0, &sfxWanted, NULL, 0);
    if (sfxDevice) SDL_PauseAudioDevice(sfxDevice, 0);
    else SDL_Log("Sound effects audio device: %s", SDL_GetError());
    for (int si=0; si<SFX_COUNT; ++si) {
        char path[128]; SDL_AudioSpec loaded;
        SDL_snprintf(path,sizeof(path),"/vol/content/%s.wav",sfxNames[si]);
        if (!SDL_LoadWAV(path,&loaded,&sfxData[si],&sfxLength[si])) {
            SDL_Log("Missing effect %s: %s",path,SDL_GetError());
        } else if (loaded.freq!=22050 || loaded.channels!=1 || loaded.format!=AUDIO_S16SYS) {
            SDL_Log("Effect has unexpected PCM format: %s",path);
            SDL_FreeWAV(sfxData[si]); sfxData[si]=NULL; sfxLength[si]=0;
        }
    }

    /* Optional LOCAL update notice, not an online download/check.
       Put a version number greater than 0.5 in /vol/content/latest_version.txt
       when distributing an update announcement. */
    int updateRequired = 0;
    SDL_RWops *versionFile = SDL_RWFromFile("/vol/content/latest_version.txt", "rb");
    if (versionFile) {
        char versionText[32] = {0};
        size_t bytes = SDL_RWread(versionFile, versionText, 1, sizeof(versionText)-1);
        SDL_RWclose(versionFile);
        if (bytes > 0 && SDL_atof(versionText) > 0.5) updateRequired = 1;
    }

    /* Isaac's original assets remain untouched. Undercover uses its own files. */
    const char *spriteSuffix[SPRITE_COUNT] = {
        "idle", "run1", "run2", "run3", "run4", "jump", "fall"
    };
    SDL_Texture *characterSprites[2][SPRITE_COUNT] = {{NULL}};
    for (int character = 0; character < 2; character++) {
        for (int i = 0; i < SPRITE_COUNT; i++) {
            char path[128];
            SDL_snprintf(path, sizeof(path), "/vol/content/%s_%s.bmp",
                         character == 0 ? "mii" : "undercover", spriteSuffix[i]);
            SDL_Surface *surface = cleanPink(SDL_LoadBMP(path));
            if (surface) {
                SDL_SetColorKey(surface, SDL_TRUE,
                    SDL_MapRGB(surface->format, 255, 0, 255));
                characterSprites[character][i] =
                    SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
            }
        }
    }

    SDL_Texture *ssdTexture=NULL;
    SDL_Surface *ssdSurface=SDL_LoadBMP("/vol/content/ssd.bmp");
    if (ssdSurface) {
        ssdTexture=SDL_CreateTextureFromSurface(renderer,ssdSurface);
        SDL_FreeSurface(ssdSurface);
    }

    /* World 1-2 park backdrop and new controller-only boss frames. */
    SDL_Texture *parkTexture=NULL;
    SDL_Surface *parkSurface=SDL_LoadBMP("/vol/content/nintendoland_park_background.bmp");
    if (parkSurface) {
        parkTexture=SDL_CreateTextureFromSurface(renderer,parkSurface);
        SDL_FreeSurface(parkSurface);
    }
    /* World 2-2 background-only BMP; game objects are drawn separately. */
    SDL_Texture *w22Backdrop=NULL;
    SDL_Surface *w22Surface=SDL_LoadBMP("/vol/content/world2_2_harbor_background.bmp");
    if(w22Surface) {
        w22Backdrop=SDL_CreateTextureFromSurface(renderer,w22Surface);
        SDL_FreeSurface(w22Surface);
    } else SDL_Log("World 2-2 background missing: %s",SDL_GetError());
    const char *bossNames[6]={"boss_idle","boss_shoot","boss_spin",
                              "boss_teleport","boss_hurt","boss_defeated"};
    SDL_Texture *bossSprites[6]={NULL};
    for (int i=0;i<6;i++) {
        char path[128];
        SDL_snprintf(path,sizeof(path),"/vol/content/%s.bmp",bossNames[i]);
        SDL_Surface *surface=SDL_LoadBMP(path);
        if (surface) {
            SDL_SetColorKey(surface,SDL_TRUE,
                            SDL_MapRGB(surface->format,255,0,255));
            bossSprites[i]=SDL_CreateTextureFromSurface(renderer,surface);
            SDL_FreeSurface(surface);
        }
    }

    /* 0=menu, 1/4=World 1, 11/12/14=World 2-1 + bonus room */
    int scene = 0;
    int retro9Level=0;
    Retro9State retro9; retro9Reset(&retro9);
    int w4Level=0,w4SSD=0; Retro9State w4;retro9Reset(&w4);
    int w5Level=0,w5SSD=0,w5MiniHP=2; Retro9State w5;retro9Reset(&w5);
    int w3MiniHP=2,w4MiniHP=2;
    int w1MiniHP=2,w2MiniHP=2;Uint32 w1MiniSafe=0,w2MiniSafe=0;
    Final6State final6;final6Reset(&final6);
    int previousScene = 0;
    int polishScene=0; Uint32 polishSceneAt=0;
    JumpSave loadedProgress; int canContinue=jumpLoad(&loadedProgress);
    int saveStatus=0;
    int selectedCharacter = 0; /* 0 Isaac, 1 Undercover */
    int running = 1;
    int facingRight = 1;
    int animationFrame = 0;
    int coinCount = 0;
    int coinsTaken[COIN_COUNT] = {0};
    float x = 90.0f, y = GROUND_Y - PLAYER_H, velocityY = 0.0f;
    VPADStatus pad;
    VPADReadError error;
    SDL_Event event;

    /* World 1-2 game state; kept separate from World 1-1. */
    float x2=90.0f, y2=GROUND_Y-PLAYER_H, vy2=0.0f;
    int facing2=1, frame2=0;
    int blockUsed=0, bombReady=0, bombFlying=0;
    float flyingBombY=0.0f;
    int bossDefeated=0, ssdCollected=0;
    /* Single-player World 2-1 and its small SSD pipe bonus room. */
    float w21x=90.0f, w21y=GROUND_Y-PLAYER_H, w21vy=0.0f;
    int w21Facing=1, w21Frame=0, w21SSD=0;
    /* World 2-2 independent player state and pickups. */
    float w22x=90.0f,w22y=GROUND_Y-PLAYER_H,w22vy=0.0f;
    int w22Facing=1,w22Frame=0,w22SSD=0,w22StudCount=0;
    int w22Collected[W22_STUD_COUNT]={0};
    /* World 2-3 garden pickups, checkpoints and enemy-hit cooldown. */
    float w23x=90.0f,w23y=GROUND_Y-PLAYER_H,w23vy=0.0f;
    int w23Facing=1,w23Frame=0,w23SSD=0,w23NectarCount=0;
    int w23Found[W23_PIKMIN_COUNT]={0};
    int w23Taken[W23_NECTAR_COUNT]={0};
    int w23Checkpoint=90;
    Uint32 w23SafeUntil=0;
    float w24x=90.0f,w24y=GROUND_Y-PLAYER_H,w24vy=0.0f;
    int w24Facing=1,w24Frame=0,w24SSD=0,w24Checkpoint=90,w24Sailing=0;
    int w24PearlsTaken[W24_PEARL_COUNT]={0};
    Uint32 w24SafeUntil=0;
    int musicStarted = 0;
    int lastPlayedW22=0;
    int lastPlayedW23=0;
    int lastPlayedW24=0;
    int lives = 3; /* Three lives for the complete World 1 run. */
    BossLaser bossLasers[BOSS_LASER_COUNT] = {{0}};
    Uint32 lastBossShot = 0;
    Uint32 bossShotFlashUntil = 0;
    int bossArenaEntered = 0;
    /* Audio event snapshots: fire once per change, not once per frame. */
    int soundPrevScene=scene, soundPrevCoins=coinCount;
    int soundPrevBoss=final6.bossHP, soundPrevW3=w3MiniHP;
    int soundPrevW4=w4MiniHP, soundPrevW5=w5MiniHP;
    int soundPrevHealth=final6.player.health;
    Uint32 soundLastShot=0, soundLastJump=0;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
        }
        VPADRead(VPAD_CHAN_0, &pad, 1, &error);
        int hold = 0, trigger = 0;
        float stickX = 0.0f;
        if (error == VPAD_READ_SUCCESS) {
            hold = pad.hold;
            trigger = pad.trigger;
            stickX = pad.leftStick.x;
        }

        /* Replay the title melody when returning from a level. */
        if (scene == 0 && previousScene != 0 && bootDevice && bootBuffer) {
            SDL_ClearQueuedAudio(bootDevice);
            SDL_QueueAudio(bootDevice, bootBuffer, bootLength);
        }
        if (scene != 0 && previousScene == 0 && bootDevice)
            SDL_ClearQueuedAudio(bootDevice);
        previousScene = scene;

        if (updateRequired) {
            SDL_SetRenderDrawColor(renderer, 21, 31, 68, 255);
            SDL_RenderClear(renderer);
            centerText(renderer, "PLEASE UPDATE THE GAME", 220, 4);
            centerText(renderer, "A NEW VERSION IS AVAILABLE", 325, 3);
            centerText(renderer, "COME BACK WHEN UPDATED", 400, 3);
            centerText(renderer, "PLUS TO EXIT", 600, 2);
            if (trigger & VPAD_BUTTON_PLUS) running = 0;
        } else if (scene == 63) {
            /* Final playable world: fight, recover Great SSD, reach ending. */
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
            else {
                int sprite=retro9Move(&final6.player,0,hold,trigger,stickX);
                Uint32 now=SDL_GetTicks();
                final6Update(&final6,now);
                int camera=(int)final6.player.x-350;
                if(camera<0)camera=0;
                if(camera>RETRO9_W-SCREEN_W)camera=RETRO9_W-SCREEN_W;
                final6Draw(renderer,&final6,camera,now,ssdTexture);
                if(now>=final6.player.invulnerableUntil||(now/110)%2==0)
                    drawPlayer(renderer,characterSprites[selectedCharacter],sprite,
                               final6.player.x,final6.player.y,final6.player.facing,camera);
                if(final6.greatSSD && final6.player.x>RETRO9_W-210)scene=64;
            }
        } else if (scene == 64) {
            SDL_SetRenderDrawColor(renderer,15,83,89,255);SDL_RenderClear(renderer);
            centerText(renderer,"THE GREAT SSD HAS BEEN FOUND!",165,4);
            centerText(renderer,"WII U NAND RESTORED!",285,5);
            centerText(renderer,"SUPER WII U JUMP COMPLETE!",405,4);
            centerText(renderer,"A TO REPLAY / PLUS TO MENU",565,3);
            if(trigger&VPAD_BUTTON_A){final6Reset(&final6);scene=63;musicStarted=1;}
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
        } else if (scene == 53) {
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
            else {
                int sprite=retro9Move(&w5,w5Level,hold,trigger,stickX);
                Uint32 now=SDL_GetTicks();
                for(int i=0;i<RETRO9_ITEMS;i++) {
                    int tx=380+i*355,ty=385-((i*3+w5Level)%4)*39;
                    if(!w5.items[i]&&w5.x+PLAYER_W>tx-13&&w5.x<tx+13&&
                       w5.y+PLAYER_H>ty-13&&w5.y<ty+13){w5.items[i]=1;w5.score++;}
                }
                if(!w5SSD&&w5.x+PLAYER_W>2040&&w5.x<2112&&
                   w5.y+PLAYER_H>340&&w5.y<390)w5SSD=1;
                if(now>=w5.invulnerableUntil)for(int i=0;i<8;i++) {
                    int ex=retro9EnemyX(w5Level,i,now);
                    if(w5.x+PLAYER_W>ex&&w5.x<ex+50&&
                       w5.y+PLAYER_H>GROUND_Y-44&&w5.y<GROUND_Y) {
                        w5.health--;w5.invulnerableUntil=now+1700;w5.vy=-11;
                        if(w5.health<=0){retro9Reset(&w5);w5SSD=0;w5MiniHP=2;}
                        break;
                    }
                }
                if(w5Level==W5_LEVELS-1){miniUpdate(&w5,&w5MiniHP,now);worldBossShotHit(&w5,&w5MiniHP,now);}
                int camera=(int)w5.x-350;
                if(camera<0)camera=0;
                if(camera>RETRO9_W-SCREEN_W)camera=RETRO9_W-SCREEN_W;
                w5Draw(renderer,w5Level,camera,&w5,now,w5SSD,ssdTexture,w5MiniHP);
                if(now>=w5.invulnerableUntil||(now/110)%2==0)
                    drawPlayer(renderer,characterSprites[selectedCharacter],sprite,
                               w5.x,w5.y,w5.facing,camera);
                if(w5.x+PLAYER_W>RETRO9_W-125 && (w5Level!=W5_LEVELS-1 || w5MiniHP<=0))scene=54;
            }
        } else if (scene == 54) {
            SDL_SetRenderDrawColor(renderer,29,57,91,255);SDL_RenderClear(renderer);
            centerText(renderer,"WORLD 5 LEVEL CLEARED!",150,4);
            centerText(renderer,w5Names[w5Level],245,3);
            centerText(renderer,w5SSD?"SSD FOUND!":"SSD MISSED!",405,3);
            centerText(renderer,w5Level==W5_LEVELS-1?"A TO FINISH":"A TO NEXT LEVEL",520,3);
            centerText(renderer,"PLUS TO MENU",585,2);
            if(trigger&(VPAD_BUTTON_A|VPAD_BUTTON_B)) {
                if(w5Level==W5_LEVELS-1)scene=55;
                else {w5Level++;retro9Reset(&w5);w5SSD=0;w5MiniHP=2;scene=53;}
            }
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
        } else if (scene == 55) {
            SDL_SetRenderDrawColor(renderer,35,92,112,255);SDL_RenderClear(renderer);
            centerText(renderer,"WORLD 5 COMPLETE!",180,5);
            centerText(renderer,"ALL 8 COURSES CLEARED",315,4);
            centerText(renderer,"WORLD 6-1 FINAL BOSS UNLOCKED",425,3);
            centerText(renderer,"A OR PLUS TO MENU",540,3);
            if(trigger&VPAD_BUTTON_A){final6Reset(&final6);scene=63;musicStarted=1;}
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
        } else if (scene == 43) {
            /* World 4: thirteen playable 2015-inspired levels. */
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
            else {
                int sprite=retro9Move(&w4,w4Level,hold,trigger,stickX);
                Uint32 now=SDL_GetTicks();
                for(int i=0;i<RETRO9_ITEMS;i++) {
                    int tx=380+i*355,ty=385-((i*3+w4Level)%4)*39;
                    if(!w4.items[i]&&w4.x+PLAYER_W>tx-13&&w4.x<tx+13&&
                       w4.y+PLAYER_H>ty-13&&w4.y<ty+13){w4.items[i]=1;w4.score++;}
                }
                if(!w4SSD&&w4.x+PLAYER_W>2040&&w4.x<2095&&
                   w4.y+PLAYER_H>340&&w4.y<412)w4SSD=1;
                if(now>=w4.invulnerableUntil)for(int i=0;i<8;i++) {
                    int ex=retro9EnemyX(w4Level,i,now);
                    if(w4.x+PLAYER_W>ex&&w4.x<ex+50&&
                       w4.y+PLAYER_H>GROUND_Y-44&&w4.y<GROUND_Y) {
                        w4.health--;w4.invulnerableUntil=now+1700;w4.vy=-11;
                        if(w4.health<=0){retro9Reset(&w4);w4SSD=0;w4MiniHP=2;}
                        break;
                    }
                }
                if(w4Level==W4_LEVELS-1){miniUpdate(&w4,&w4MiniHP,now);worldBossShotHit(&w4,&w4MiniHP,now);}
                int camera=(int)w4.x-350;
                if(camera<0)camera=0;
                if(camera>RETRO9_W-SCREEN_W)camera=RETRO9_W-SCREEN_W;
                w4Draw(renderer,w4Level,camera,&w4,now,w4SSD);
                if(w4Level==W4_LEVELS-1)miniDraw(renderer,camera,w4MiniHP,now);
                if(now>=w4.invulnerableUntil||(now/110)%2==0)
                    drawPlayer(renderer,characterSprites[selectedCharacter],sprite,
                               w4.x,w4.y,w4.facing,camera);
                if(w4.x+PLAYER_W>RETRO9_W-125 && (w4Level!=W4_LEVELS-1 || w4MiniHP<=0))scene=44;
            }
        } else if (scene == 44) {
            SDL_SetRenderDrawColor(renderer,29,57,91,255);SDL_RenderClear(renderer);
            centerText(renderer,"WORLD 4 LEVEL CLEARED!",150,4);
            centerText(renderer,w4Names[w4Level],245,3);
            char hud[80];SDL_snprintf(hud,sizeof(hud),"TOKENS: %d/12",w4.score);
            centerText(renderer,hud,335,3);
            centerText(renderer,w4SSD?"SSD FOUND!":"SSD MISSED!",405,3);
            centerText(renderer,w4Level==W4_LEVELS-1?"A TO FINISH":"A TO NEXT LEVEL",520,3);
            centerText(renderer,"PLUS TO MENU",585,2);
            if(trigger&(VPAD_BUTTON_A|VPAD_BUTTON_B)) {
                if(w4Level==W4_LEVELS-1)scene=45;
                else {w4Level++;retro9Reset(&w4);w4SSD=0;w4MiniHP=2;scene=43;}
            }
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
        } else if (scene == 45) {
            SDL_SetRenderDrawColor(renderer,35,92,112,255);SDL_RenderClear(renderer);
            centerText(renderer,"WORLD 4 COMPLETE!",180,5);
            centerText(renderer,"ALL 13 COURSES CLEARED",315,4);
            centerText(renderer,"A OR PLUS TO MENU",540,3);
            if(trigger&VPAD_BUTTON_A){w5Level=0;w5SSD=0;w5MiniHP=2;retro9Reset(&w5);scene=53;musicStarted=1;}
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
        } else if (scene == 40) {
            /* Nine distinct 2014 courses, progressing in the supplied order. */
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
            else {
                int sprite=retro9Move(&retro9,retro9Level,hold,trigger,stickX);
                Uint32 now=SDL_GetTicks();
                for(int i=0;i<RETRO9_ITEMS;i++) {
                    int tx=380+i*355,ty=385-((i*3+retro9Level)%4)*39;
                    if(!retro9.items[i] && retro9.x+PLAYER_W>tx-13 &&
                       retro9.x<tx+13 && retro9.y+PLAYER_H>ty-13 &&
                       retro9.y<ty+13){retro9.items[i]=1;retro9.score++;}
                }
                if(now>=retro9.invulnerableUntil)for(int i=0;i<8;i++) {
                    int ex=retro9EnemyX(retro9Level,i,now);
                    if(retro9.x+PLAYER_W>ex && retro9.x<ex+50 &&
                       retro9.y+PLAYER_H>GROUND_Y-44 && retro9.y<GROUND_Y) {
                        retro9.health--;retro9.invulnerableUntil=now+1700;
                        retro9.vy=-11;
                        if(retro9.health<=0){retro9Reset(&retro9);w3MiniHP=2;}
                        break;
                    }
                }
                if(retro9Level==8){miniUpdate(&retro9,&w3MiniHP,now);worldBossShotHit(&retro9,&w3MiniHP,now);}
                int camera=(int)retro9.x-350;
                if(camera<0)camera=0;
                if(camera>RETRO9_W-SCREEN_W)camera=RETRO9_W-SCREEN_W;
                retro9Draw(renderer,retro9Level,camera,&retro9,now);
                if(retro9Level==8)miniDraw(renderer,camera,w3MiniHP,now);
                if(now>=retro9.invulnerableUntil || (now/110)%2==0)
                    drawPlayer(renderer,characterSprites[selectedCharacter],sprite,
                               retro9.x,retro9.y,retro9.facing,camera);
                if(retro9.x+PLAYER_W>RETRO9_W-125 && (retro9Level!=8 || w3MiniHP<=0))scene=41;
            }
        } else if (scene == 41) {
            SDL_SetRenderDrawColor(renderer,27,58,94,255);
            SDL_RenderClear(renderer);
            centerText(renderer,"2014 LEVEL COMPLETE!",160,5);
            centerText(renderer,retro9Names[retro9Level],275,4);
            char score[40];SDL_snprintf(score,sizeof(score),"TOKENS: %d/12",retro9.score);
            centerText(renderer,score,380,3);
            centerText(renderer,retro9Level==8?"A TO FINISH":"A TO NEXT LEVEL",510,3);
            centerText(renderer,"PLUS TO MENU",580,2);
            if(trigger&(VPAD_BUTTON_A|VPAD_BUTTON_B)) {
                if(retro9Level==8)scene=42;
                else {retro9Level++;retro9Reset(&retro9);w3MiniHP=2;scene=40;}
            }
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
        } else if (scene == 42) {
            SDL_SetRenderDrawColor(renderer,24,77,105,255);
            SDL_RenderClear(renderer);
            centerText(renderer,"ALL NINE LEVELS CLEARED!",180,4);
            centerText(renderer,"2014 COMPLETE",315,5);
            centerText(renderer,"A OR PLUS TO MENU",535,3);
            if(trigger&VPAD_BUTTON_A){w4Level=0;w4SSD=0;w4MiniHP=2;retro9Reset(&w4);scene=43;musicStarted=1;}
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
        } else if (scene == 0) {
            drawNightTitle(renderer);
            centerText(renderer, "SUPER WII U JUMP", 146, 7);
            centerText(renderer, "WORLD 1 - 2012", 292, 4);
            centerText(renderer, "PRESS A TO START", 385, 4);
            if(canContinue) centerText(renderer,"X TO CONTINUE",430,3);
            centerText(renderer, "Y FOR CREDITS", 490, 3);
            centerText(renderer, "PLUS TO EXIT", 640, 2);
            drawSeatedTitleMii(renderer, characterSprites[selectedCharacter][0], 602, 480);
            if (trigger & (VPAD_BUTTON_A | VPAD_BUTTON_B)) {
                lastPlayedW22=0;lastPlayedW23=0;lastPlayedW24=0;
                scene = 3;
            }
            /* X loads only the last legitimately reached gameplay checkpoint. */
            if(canContinue && (trigger & VPAD_BUTTON_X)) {
                scene=loadedProgress.scene;
                selectedCharacter=loadedProgress.character;
                retro9Level=loadedProgress.retro9Level;
                w4Level=loadedProgress.w4Level;
                w5Level=loadedProgress.w5Level;
                retro9Reset(&retro9);retro9Reset(&w4);retro9Reset(&w5);
                final6Reset(&final6);
                w3MiniHP=2;w4MiniHP=2;w5MiniHP=2;
                w1MiniHP=2;w2MiniHP=2;
                x=90;y=GROUND_Y-PLAYER_H;velocityY=0;
                x2=90;y2=GROUND_Y-PLAYER_H;vy2=0;
                w21x=90;w21y=GROUND_Y-PLAYER_H;w21vy=0;
                w22x=90;w22y=GROUND_Y-PLAYER_H;w22vy=0;
                w23x=90;w23y=GROUND_Y-PLAYER_H;w23vy=0;
                w24x=90;w24y=GROUND_Y-PLAYER_H;w24vy=0;
                musicStarted=1;
            }
            if(trigger & VPAD_BUTTON_Y) {scene=70; musicStarted=1;}
            /* Release build: no menu world-skip/debug shortcuts. */
            if (trigger & VPAD_BUTTON_PLUS) running = 0;
        } else if (scene == 70) {
            /* Credits are accessible from the title via Y, without changing progress. */
            drawNightTitle(renderer);
            fill(renderer, 170, 108, SCREEN_W-340, 488, 16, 30, 64);
            centerText(renderer, "SUPER WII U JUMP", 150, 5);
            centerText(renderer, "CREDITS", 240, 5);
            centerText(renderer, "CREATED BY", 335, 3);
            centerText(renderer, "ISAACSG2019", 382, 4);
            centerText(renderer, "THANKS FOR PLAYING", 480, 3);
            centerText(renderer, "B OR PLUS TO MENU", 560, 2);
            if(trigger & (VPAD_BUTTON_B | VPAD_BUTTON_PLUS)) {scene=0;}
        } else if (scene == 3) {
            /* Choose Isaac or Undercover before entering World 1-1. */
            SDL_SetRenderDrawColor(renderer, 23, 36, 74, 255);
            SDL_RenderClear(renderer);
            centerText(renderer, "CHOOSE YOUR MII", 72, 6);
            centerText(renderer, "WORLD 1 - 2012", 151, 3);
            for (int c = 0; c < 2; c++) {
                int bx = c == 0 ? 230 : 700;
                int active = selectedCharacter == c;
                fill(renderer, bx-8, 215, 368, 360,
                     active ? 255 : 63, active ? 221 : 84, active ? 98 : 121);
                fill(renderer, bx, 223, 352, 344, 49, 82, 125);
                fill(renderer, bx+10, 233, 332, 252, 80, 155, 179);
                fill(renderer, bx+10, 467, 332, 18, 72, 211, 83);
                SDL_Texture *preview = characterSprites[c][0];
                if (preview) {
                    SDL_Rect dst = {bx+130, 261, 96, 192};
                    SDL_RenderCopy(renderer, preview, NULL, &dst);
                } else {
                    fill(renderer, bx+150, 285, 60, 155,
                         c == 0 ? 95 : 35, c == 0 ? 221 : 38, 62);
                }
                drawText(renderer, c == 0 ? "ISAAC MII" : "UNDERCOVER MII",
                         bx + (c == 0 ? 74 : 23), 516, 3);
                if (active) drawText(renderer, "SELECTED", bx+83, 593, 3);
            }
            centerText(renderer, "LEFT RIGHT TO CHOOSE", 640, 3);
            centerText(renderer, "A TO PLAY  B TO BACK", 679, 2);
            if (trigger & VPAD_BUTTON_LEFT) selectedCharacter = 0;
            if (trigger & VPAD_BUTTON_RIGHT) selectedCharacter = 1;
            if (trigger & VPAD_BUTTON_B || trigger & VPAD_BUTTON_PLUS) scene = 0;
            if (trigger & VPAD_BUTTON_A) {
                resetGame(&x, &y, &velocityY, coinsTaken, &coinCount,
                          &animationFrame, &facingRight);
                lives = 3;w1MiniHP=2;w1MiniSafe=0;
                clearBossLasers(bossLasers);
                bossArenaEntered = 0;
                scene = 1;
                musicStarted = 1;
            }
        } else if (scene == 1) {
            /* PLUS returns to menu. */
            if (trigger & VPAD_BUTTON_PLUS) { scene = 0; musicStarted = 0; }

            int left = (hold & VPAD_BUTTON_LEFT) || stickX < -0.25f;
            int right = (hold & VPAD_BUTTON_RIGHT) || stickX > 0.25f;
            int moving = 0;
            if (left && !right) {
                x -= 5.0f;
                facingRight = 0;
                moving = 1;
            }
            if (right && !left) {
                x += 5.0f;
                facingRight = 1;
                moving = 1;
            }
            if (x < 0) x = 0;
            if (x > LEVEL_W - PLAYER_W) x = LEVEL_W - PLAYER_W;

            /* Platforms and stairs are treated as the same landing surfaces.
               Landing onto stairs also enables jumping on the next frame. */
            const SDL_Rect solid[] = {
                {1790,GROUND_Y-132,175,132},
                {2520,GROUND_Y-30,50,30},
                {2570,GROUND_Y-60,50,60},
                {2620,GROUND_Y-90,50,90},
                {2670,GROUND_Y-120,50,120},
                {2720,GROUND_Y-150,50,150},
                {3860,GROUND_Y-72,72,72},
                {4330,GROUND_Y-72,72,72},
                {4800,GROUND_Y-72,72,72},
                {5450,GROUND_Y-34,48,34},
                {5498,GROUND_Y-68,48,68},
                {5546,GROUND_Y-102,48,102},
                {5594,GROUND_Y-136,48,136}
            };
            const int solidCount = (int)(sizeof(solid)/sizeof(solid[0]));
            float oldBottom = y + PLAYER_H;
            int grounded = (velocityY >= 0 &&
                            oldBottom >= GROUND_Y-1 && oldBottom <= GROUND_Y+2);
            for (int i=0; i<PLATFORM_COUNT+solidCount; ++i) {
                SDL_Rect p = i < PLATFORM_COUNT ? platforms[i] : solid[i-PLATFORM_COUNT];
                if (velocityY >= 0 && x+PLAYER_W > p.x && x < p.x+p.w &&
                    oldBottom >= p.y-2 && oldBottom <= p.y+3) grounded=1;
            }
            if (grounded && (trigger & (VPAD_BUTTON_A|VPAD_BUTTON_B)))
                velocityY = -15.5f;
            velocityY += 0.55f;
            if (velocityY > 15.0f) velocityY = 15.0f;
            y += velocityY;
            if (velocityY >= 0) {
                float landingY = GROUND_Y-PLAYER_H;
                for (int i=0; i<PLATFORM_COUNT+solidCount; ++i) {
                    SDL_Rect p = i < PLATFORM_COUNT ? platforms[i] : solid[i-PLATFORM_COUNT];
                    if (x+PLAYER_W > p.x && x < p.x+p.w &&
                        oldBottom <= p.y+3 && y+PLAYER_H >= p.y &&
                        p.y-PLAYER_H < landingY)
                        landingY = p.y-PLAYER_H;
                }
                if (y >= landingY) { y=landingY; velocityY=0; }
            }

            /* A brown critter costs one life. Respawn unless all lives are gone. */
            const int enemies[] = {1090,2260,3500,4700};
            for(int i=0;i<4;i++) {
                int ex=enemies[i];
                if(x+PLAYER_W>ex+5 && x<ex+47 &&
                   y+PLAYER_H>GROUND_Y-44 && y<GROUND_Y) {
                    lives--;
                    if (lives <= 0) {
                        scene = 6; /* Game Over */
                        musicStarted = 0;
                    } else {
                        resetGame(&x,&y,&velocityY,coinsTaken,&coinCount,
                                  &animationFrame,&facingRight);
                    }
                    break;
                }
            }

            int airborne = (velocityY != 0.0f);
            if (moving && !airborne) animationFrame++;
            else animationFrame = 0;
            int spriteIndex = airborne ? (velocityY < 0 ? 5 : 6) :
                (moving ? 1 + ((animationFrame / 4) % 4) : 0);

            for (int i = 0; i < COIN_COUNT; i++) {
                int cx = coinPositions[i].x, cy = coinPositions[i].y;
                if (!coinsTaken[i] && x < cx + 13 && x + PLAYER_W > cx - 13 &&
                    y < cy + 15 && y + PLAYER_H > cy - 15) {
                    coinsTaken[i] = 1;
                    coinCount++;
                }
            }

            int camera = (int)x - 350;
            if (camera < 0) camera = 0;
            if (camera > LEVEL_W - SCREEN_W)
                camera = LEVEL_W - SCREEN_W;
            drawWorld(renderer, camera, coinsTaken, coinCount);
            /* World 1 already has its unique controller boss in 1-2. */
            drawLives(renderer, lives);
            drawPlayer(renderer, characterSprites[selectedCharacter], spriteIndex, x, y,
                       facingRight, camera);
            if (scene == 1 && x + PLAYER_W > 5740 && x < 5790)
                scene = 2;
        } else if (scene == 4) {
            /* World 1-2: 10k pixels; stand under the boss and throw upward. */
            if (trigger & VPAD_BUTTON_PLUS) {
                scene=0;
                musicStarted=0;
            }
            int left=(hold & VPAD_BUTTON_LEFT) || stickX < -0.25f;
            int right=(hold & VPAD_BUTTON_RIGHT) || stickX > 0.25f;
            int moving=0;
            if (left && !right) { x2-=5.5f; facing2=0; moving=1; }
            if (right && !left) { x2+=5.5f; facing2=1; moving=1; }
            if (x2<0) x2=0;
            if (x2>LEVEL2_W-PLAYER_W) x2=LEVEL2_W-PLAYER_W;
            int jump=(trigger & (VPAD_BUTTON_A | VPAD_BUTTON_B))!=0;
            float oldBottom=y2+PLAYER_H;
            int grounded=(oldBottom >= GROUND_Y-1 && vy2>=0);
            /* First chunk is a raised platform; landing detection */
            SDL_Rect level2Platforms[] = {
                {2280,400,80,40},{2362,400,80,40},{2444,400,80,40},
                {4100,466,110,35},{4520,398,110,35},
                {4940,466,110,35},{5360,398,110,35},
                {7400,420,115,32},{7645,482,115,32},{7890,420,115,32}
            };
            int platformCount2=(int)(sizeof(level2Platforms)/sizeof(level2Platforms[0]));
            for(int i=0;i<platformCount2;i++) {
                SDL_Rect p=level2Platforms[i];
                if(vy2>=0 && x2+PLAYER_W>p.x && x2<p.x+p.w &&
                   oldBottom>=p.y-3 && oldBottom<=p.y+5) grounded=1;
            }
            if(grounded && jump) vy2=-16.0f;
            vy2+=0.55f;
            if(vy2>15) vy2=15;
            y2+=vy2;
            if(vy2>=0) {
                float landing=GROUND_Y-PLAYER_H;
                for(int i=0;i<platformCount2;i++) {
                    SDL_Rect p=level2Platforms[i];
                    if(x2+PLAYER_W>p.x && x2<p.x+p.w &&
                       oldBottom<=p.y+5 && y2+PLAYER_H>=p.y &&
                       p.y-PLAYER_H<landing) landing=p.y-PLAYER_H;
                }
                if(y2>=landing) {y2=landing; vy2=0;}
            }
            /* A ? block supplies the only weapon. Trigger from below or by contact. */
            if (!blockUsed && x2+PLAYER_W>BLOCK_X && x2<BLOCK_X+80 &&
                y2<440 && y2+PLAYER_H>390 && jump) {
                blockUsed=1;
                bombReady=1;
            }
            /* If the player misses, allow collecting the bomb displayed above the block. */
            if(!blockUsed && x2+PLAYER_W>BLOCK_X+18 && x2<BLOCK_X+66 &&
               y2<395 && y2+PLAYER_H>335) {
                blockUsed=1;
                bombReady=1;
            }
            /* Throw ONLY while standing below the boss's central weak point. */
            int throwPressed=(trigger & VPAD_BUTTON_Y)!=0;
            if (!bossDefeated && bombReady && !bombFlying && throwPressed &&
                x2+PLAYER_W/2>BOSS_X-65 && x2+PLAYER_W/2<BOSS_X+65) {
                bombFlying=1;
                bombReady=0;
                flyingBombY=y2-10;
            }
            if (bombFlying) {
                flyingBombY-=13.0f;
                /* Weak point is at approximately y=329-342. */
                if (flyingBombY<=345) {
                    bombFlying=0;
                    bossDefeated=1;
                }
            }
            /* Every 3 seconds the controller fires toward the Mii, but only
               after entering the boss area. Victory stops further firing. */
            Uint32 bossNow = SDL_GetTicks();
            if (!bossDefeated && x2 >= BOSS_ARENA_START) {
                if (!bossArenaEntered) {
                    bossArenaEntered = 1;
                    lastBossShot = bossNow;
                }
                if ((Uint32)(bossNow - lastBossShot) >= BOSS_SHOT_INTERVAL) {
                    spawnBossLaser(bossLasers, x2 + PLAYER_W / 2.0f,
                                   y2 + PLAYER_H / 2.0f);
                    lastBossShot = bossNow;
                    bossShotFlashUntil = bossNow + 260u;
                }
                if (updateBossLasers(bossLasers, x2, y2)) {
                    lives--;
                    clearBossLasers(bossLasers);
                    bossArenaEntered = 0;
                    lastBossShot = bossNow;
                    if (lives <= 0) {
                        scene = 6;
                        musicStarted = 0;
                    } else {
                        /* Retry World 1-2; preserve earned bomb and boss state. */
                        x2 = 90.0f;
                        y2 = GROUND_Y - PLAYER_H;
                        vy2 = 0.0f;
                    }
                }
            } else if (bossDefeated) {
                clearBossLasers(bossLasers);
                bossArenaEntered = 0;
            }
            /* Touching the active laser costs one life and restarts World 1-2. */
            if((SDL_GetTicks()/16)%180<90 &&
               x2+PLAYER_W>LASER_X+30 && x2<LASER_X+460 &&
               y2+PLAYER_H>GROUND_Y-118 && y2<GROUND_Y-108) {
                lives--;
                if (lives <= 0) {
                    scene = 6;
                    musicStarted = 0;
                } else {
                    x2=90; y2=GROUND_Y-PLAYER_H; vy2=0;
                    blockUsed=0; bombReady=0; bombFlying=0;
                    bossDefeated=0; ssdCollected=0;
                    clearBossLasers(bossLasers);
                    bossArenaEntered = 0;
                }
            }
            if(bossDefeated && !ssdCollected &&
               x2+PLAYER_W>BOSS_X-64 && x2<BOSS_X+64 &&
               y2+PLAYER_H>GROUND_Y-90) ssdCollected=1;
            if(moving && vy2==0) frame2++;
            else frame2=0;
            int anim=vy2!=0 ? (vy2<0?5:6) :
                     (moving ? 1+(frame2/4)%4 : 0);
            int camera=(int)x2-350;
            if(camera<0) camera=0;
            if(camera>LEVEL2_W-SCREEN_W) camera=LEVEL2_W-SCREEN_W;
            drawLevel2(renderer,camera,bombReady,blockUsed,bossDefeated,
                        ssdCollected,flyingBombY,bombFlying,ssdTexture,
                        parkTexture,bossSprites,
                        !bossDefeated && SDL_GetTicks() < bossShotFlashUntil);
            drawBossLasers(renderer,bossLasers,camera);
            drawLives(renderer, lives);
            drawPlayer(renderer,characterSprites[selectedCharacter],
                       anim,x2,y2,facing2,camera);
            if(scene == 4 && ssdCollected && x2+PLAYER_W>9820) scene=5;
        } else if (scene == 11) {
            /* World 2-1 overworld. Down on the first pipe enters the SSD room. */
            if (trigger & VPAD_BUTTON_PLUS) { scene=0; musicStarted=0; }
            int sprite=w21Move(&w21x,&w21y,&w21vy,&w21Facing,&w21Frame,
                               hold,trigger,stickX,0);
            int enteringPipe = (trigger & VPAD_BUTTON_DOWN) &&
                               w21OnPipe(w21x,w21y,W21_ENTRY_PIPE_X);
            int camera=(int)w21x-350;
            if(camera<0) camera=0;
            if(camera>W21_WIDTH-SCREEN_W) camera=W21_WIDTH-SCREEN_W;
            w21DrawOutside(renderer,camera,w21SSD);
            {Uint32 t=SDL_GetTicks();int hit=earlyMiniHit(w21x,w21y,w21vy,3700,&w2MiniHP,t,&w2MiniSafe);
             if(hit==1)w21vy=-14;
             if(hit==-1){w21vy=-11;if(lives>1)--lives;else{lives=3;w21x=90;w21y=GROUND_Y-PLAYER_H;w21vy=0;w2MiniHP=2;}}}
            if(earlyBossShotHit(w21x,w21y,3700,w2MiniHP,SDL_GetTicks(),w2MiniSafe)) {
                w2MiniSafe=SDL_GetTicks()+1500;w21vy=-11;
                if(lives>1)--lives;
                else {lives=3;w21x=90;w21y=GROUND_Y-PLAYER_H;w21vy=0;w2MiniHP=2;}
            }
            earlyMiniDraw(renderer,camera,3700,w2MiniHP);
            earlyBossShotDraw(renderer,camera,3700,w2MiniHP,SDL_GetTicks());
            drawLives(renderer,lives);
            drawPlayer(renderer,characterSprites[selectedCharacter],sprite,
                       w21x,w21y,w21Facing,camera);
            if(enteringPipe) {
                scene=12;
                w21x=380.0f; w21y=GROUND_Y-PLAYER_H; w21vy=0.0f;
                w21Frame=0;
            } else if(w21SSD && w2MiniHP<=0 && w21x+PLAYER_W>4220) scene=14;
        } else if (scene == 12) {
            /* The SSD can be collected once, and the exit pipe always works. */
            if (trigger & VPAD_BUTTON_PLUS) { scene=0; musicStarted=0; }
            int sprite=w21Move(&w21x,&w21y,&w21vy,&w21Facing,&w21Frame,
                               hold,trigger,stickX,1);
            if(!w21SSD && w21x+PLAYER_W>590 && w21x<662 &&
               w21y+PLAYER_H>GROUND_Y-64 && w21y<GROUND_Y-14)
                w21SSD=1;
            int leavingPipe = (trigger & VPAD_BUTTON_DOWN) &&
                              w21OnPipe(w21x,w21y,W21_ROOM_EXIT_X);
            w21DrawRoom(renderer,ssdTexture,w21SSD);
            if(w21SSD) drawText(renderer,"SSD COLLECTED!",470,290,3);
            drawPlayer(renderer,characterSprites[selectedCharacter],sprite,
                       w21x,w21y,w21Facing,0);
            if(leavingPipe) {
                scene=11;
                w21x=W21_EXIT_PIPE_X+155;
                w21y=GROUND_Y-PLAYER_H; w21vy=0.0f;
                w21Frame=0;
            }
        } else if (scene == 22) {
            /* World 2-2: harbor course. Same controls as single-player World 2-1. */
            lastPlayedW22=1;
            if(trigger & VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
            int sprite=w22Move(&w22x,&w22y,&w22vy,&w22Facing,&w22Frame,
                               hold,trigger,stickX);
            for(int i=0;i<W22_STUD_COUNT;i++) {
                if(!w22Collected[i] && w22x+PLAYER_W>w22Studs[i].x-12 &&
                   w22x<w22Studs[i].x+12 && w22y+PLAYER_H>w22Studs[i].y-15 &&
                   w22y<w22Studs[i].y+15) {
                    w22Collected[i]=1;w22StudCount++;
                }
            }
            if(!w22SSD && w22x+PLAYER_W>3350 && w22x<3422 &&
               w22y+PLAYER_H>GROUND_Y-270 && w22y<GROUND_Y-220) w22SSD=1;
            for(int i=0;i<5 && scene==22;i++) {
                int ex=w22PoliceX[i];
                if(w22x+PLAYER_W>ex+5 && w22x<ex+40 &&
                   w22y+PLAYER_H>GROUND_Y-56 && w22y<GROUND_Y) {
                    lives--;
                    if(lives<=0){scene=6;musicStarted=0;}
                    else {w22x=90.0f;w22y=GROUND_Y-PLAYER_H;w22vy=0.0f;}
                }
            }
            int camera=(int)w22x-350;
            if(camera<0)camera=0;
            if(camera>W22_WIDTH-SCREEN_W)camera=W22_WIDTH-SCREEN_W;
            w22Draw(renderer,w22Backdrop,camera,w22Collected,w22StudCount,
                    w22SSD,ssdTexture);
            drawLives(renderer,lives);
            drawPlayer(renderer,characterSprites[selectedCharacter],sprite,
                       w22x,w22y,w22Facing,camera);
            if(scene==22 && w22x+PLAYER_W>5390) scene=24;
        } else if (scene == 26) {
            /* World 2-4: collect pearls and sail between three islands. */
            lastPlayedW24=1;lastPlayedW23=0;lastPlayedW22=0;
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
            Uint32 now=SDL_GetTicks();
            int count=w24PearlsTaken[0]+w24PearlsTaken[1]+w24PearlsTaken[2];
            int sprite=0;
            if(w24Sailing) {
                w24x+=11.0f;
                w24y=GROUND_Y-PLAYER_H-20;
                w24vy=0;
                w24Facing=1;
                sprite=0;
                if(w24Sailing==1 && w24x>=2360.0f) {
                    w24Sailing=0;w24x=2380.0f;
                    w24y=GROUND_Y-PLAYER_H;w24Checkpoint=2380;
                } else if(w24Sailing==2 && w24x>=4570.0f) {
                    w24Sailing=0;w24x=4590.0f;
                    w24y=GROUND_Y-PLAYER_H;w24Checkpoint=4590;
                }
            } else {
                sprite=w24Move(&w24x,&w24y,&w24vy,&w24Facing,&w24Frame,
                               hold,trigger,stickX,count);
                for(int i=0;i<W24_PEARL_COUNT;i++) {
                    if(!w24PearlsTaken[i] &&
                       w24x+PLAYER_W>w24Pearls[i].x-38 &&
                       w24x<w24Pearls[i].x+38 &&
                       w24y+PLAYER_H>w24Pearls[i].y-36 &&
                       w24y<w24Pearls[i].y+36)
                        w24PearlsTaken[i]=1;
                }
                if(!w24SSD && w24x+PLAYER_W>5365 && w24x<5437 &&
                   w24y+PLAYER_H>365 && w24y<415)w24SSD=1;
                /* DOWN at each dock boards the boat once its pearl is found. */
                if((trigger&VPAD_BUTTON_DOWN) &&
                   w24y+PLAYER_H>=GROUND_Y-6 && w24vy>=0) {
                    if(w24PearlsTaken[0] && w24x>=1400 && w24x<=1600) {
                        w24Sailing=1;w24x=1550;w24y=GROUND_Y-PLAYER_H-20;
                    } else if(w24PearlsTaken[1] &&
                              w24x>=3580 && w24x<=3790) {
                        w24Sailing=2;w24x=3730;w24y=GROUND_Y-PLAYER_H-20;
                    }
                }
                int hit=w24y>SCREEN_H+10;
                for(int i=0;i<W24_CRAB_COUNT;i+=2) {
                    int ex=w24CrabX(i,now);
                    if(w24x+PLAYER_W>ex+8 && w24x<ex+48 &&
                       w24y+PLAYER_H>GROUND_Y-43 && w24y<GROUND_Y-4)
                        hit=1;
                }
                if(((now/16u)%160u)<24u &&
                   w24x+PLAYER_W>5900 && w24x<5945 &&
                   w24y+PLAYER_H>GROUND_Y-124)hit=1;
                if(hit && now>=w24SafeUntil) {
                    lives--;
                    if(lives<=0){scene=6;musicStarted=0;}
                    else {
                        w24x=(float)w24Checkpoint;
                        w24y=GROUND_Y-PLAYER_H;w24vy=0;w24Frame=0;
                        w24Sailing=0;w24SafeUntil=now+3000u;
                    }
                }
            }
            int camera=(int)w24x-350;
            if(camera<0)camera=0;
            if(camera>W24_WIDTH-SCREEN_W)camera=W24_WIDTH-SCREEN_W;
            w24Draw(renderer,camera,w24PearlsTaken,w24SSD,
                    w24Sailing,(int)w24x,now,ssdTexture);
            drawLives(renderer,lives);
            drawPlayer(renderer,characterSprites[selectedCharacter],sprite,
                       w24x,w24y,w24Facing,camera);
            if(scene==26 && w24x+PLAYER_W>6130 &&
               w24PearlsTaken[0] && w24PearlsTaken[1] && w24PearlsTaken[2])
                scene=27;
        } else if (scene == 27) {
            SDL_SetRenderDrawColor(renderer,28,91,142,255);
            SDL_RenderClear(renderer);
            centerText(renderer,"WORLD 2 COMPLETE!",140,6);
            centerText(renderer,"2013 FINISHED",280,5);
            centerText(renderer,"ALL FOUR LEVELS CLEARED",374,3);
            if(w24SSD)centerText(renderer,"FINAL SSD FOUND!",445,3);
            centerText(renderer,"A TO WORLD 3-1",525,3);
            centerText(renderer,"PLUS TO MENU",585,2);
            if(trigger&(VPAD_BUTTON_A|VPAD_BUTTON_B)) {
                /* World 2-4 leads into the first 2014 course, not the title. */
                retro9Level=0;
                w3MiniHP=2;
                retro9Reset(&retro9);
                scene=40;
                musicStarted=1;
                lastPlayedW24=0;
            }
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
        } else if (scene == 23) {
            /* Pikmin-inspired garden. Save progress at the two garden signs. */
            lastPlayedW23=1;lastPlayedW22=0;
            if(trigger & VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
            int pikminCount=w23Found[0]+w23Found[1]+w23Found[2];
            int sprite=w23Move(&w23x,&w23y,&w23vy,&w23Facing,&w23Frame,
                               hold,trigger,stickX,pikminCount);
            for(int i=0;i<W23_PIKMIN_COUNT;i++) {
                if(!w23Found[i] && w23x+PLAYER_W>w23Pikmin[i].x &&
                   w23x<w23Pikmin[i].x+36 &&
                   w23y+PLAYER_H>w23Pikmin[i].y-5 &&
                   w23y<w23Pikmin[i].y+42)w23Found[i]=1;
            }
            for(int i=0;i<W23_NECTAR_COUNT;i++) {
                if(!w23Taken[i] && w23x+PLAYER_W>w23Nectar[i].x-12 &&
                   w23x<w23Nectar[i].x+12 &&
                   w23y+PLAYER_H>w23Nectar[i].y-13 &&
                   w23y<w23Nectar[i].y+15) {
                    w23Taken[i]=1;w23NectarCount++;
                }
            }
            if(w23x>2150 && w23Checkpoint<2150)w23Checkpoint=2150;
            if(w23x>3910 && w23Checkpoint<3910)w23Checkpoint=3910;
            if(!w23SSD && w23x+PLAYER_W>4990 && w23x<5062 &&
               w23y+PLAYER_H>GROUND_Y-298 && w23y<GROUND_Y-248)
                w23SSD=1;
            /* Pond surfaces and garden bugs damage only when not airborne.
               Brief grace period prevents consecutive instant hits. */
            int hit=0;
            if(w23y+PLAYER_H>GROUND_Y-12) {
                if((w23x+PLAYER_W>1195 && w23x<1325) ||
                   (w23x+PLAYER_W>4520 && w23x<4680))hit=1;
                if(w23Found[0]+w23Found[1]+w23Found[2]<3 &&
                   w23x+PLAYER_W>3340 && w23x<3730)hit=1;
            }
            Uint32 now=SDL_GetTicks();
            for(int i=0;i<W23_BUG_COUNT;i++) {
                int ex=w23BugX(i,now);
                if(w23x+PLAYER_W>ex+7 && w23x<ex+57 &&
                   w23y+PLAYER_H>GROUND_Y-52 && w23y<GROUND_Y-8)
                    hit=1;
            }
            if(scene==23 && hit && now>=w23SafeUntil) {
                lives--;
                if(lives<=0){scene=6;musicStarted=0;}
                else {
                    w23x=(float)w23Checkpoint;
                    w23y=GROUND_Y-PLAYER_H;w23vy=0.0f;
                    w23Frame=0;w23SafeUntil=now+1600u;
                }
            }
            int camera=(int)w23x-350;
            if(camera<0)camera=0;
            if(camera>W23_WIDTH-SCREEN_W)camera=W23_WIDTH-SCREEN_W;
            w23Draw(renderer,camera,w23Taken,w23NectarCount,
                    w23Found,w23SSD,ssdTexture);
            drawLives(renderer,lives);
            w23Followers(renderer,w23x,w23y,camera,w23Facing,w23Found);
            drawPlayer(renderer,characterSprites[selectedCharacter],sprite,
                       w23x,w23y,w23Facing,camera);
            if(scene==23 && w23x+PLAYER_W>5810)scene=25;
        } else if (scene == 25) {
            SDL_SetRenderDrawColor(renderer,30,73,55,255);
            SDL_RenderClear(renderer);
            centerText(renderer,"WORLD 2-3 CLEARED!",160,5);
            char result[48];
            SDL_snprintf(result,sizeof(result),"NECTAR: %d",w23NectarCount);
            centerText(renderer,result,315,4);
            if(w23SSD)centerText(renderer,"SSD FOUND!",410,4);
            centerText(renderer,"A TO WORLD 2-4",500,3);
            centerText(renderer,"PLUS TO MENU",570,3);
            if(trigger&(VPAD_BUTTON_A|VPAD_BUTTON_B)) {
                w24Reset(&w24x,&w24y,&w24vy,&w24Facing,&w24Frame,
                         w24PearlsTaken,&w24SSD,&w24Checkpoint,
                         &w24Sailing,&w24SafeUntil);
                scene=26;musicStarted=1;lastPlayedW24=1;
            }
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
        } else if (scene == 24) {
            SDL_SetRenderDrawColor(renderer,24,56,89,255);
            SDL_RenderClear(renderer);
            centerText(renderer,"WORLD 2-2 CLEARED!",165,5);
            char count[32];
            SDL_snprintf(count,sizeof(count),"STUDS: %d",w22StudCount);
            centerText(renderer,count,325,4);
            if(w22SSD) centerText(renderer,"SSD FOUND!",410,4);
            centerText(renderer,"A TO WORLD 2-3",550,3);
            if(trigger&(VPAD_BUTTON_A|VPAD_BUTTON_B)) {
                w23Reset(&w23x,&w23y,&w23vy,&w23Facing,&w23Frame,
                         w23Found,w23Taken,&w23NectarCount,&w23SSD,&w23Checkpoint);
                scene=23;musicStarted=1;lastPlayedW22=0;
                lastPlayedW23=1;w23SafeUntil=0;
            }
            if(trigger&VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
        } else if (scene == 14) {
            SDL_SetRenderDrawColor(renderer,28,57,86,255);
            SDL_RenderClear(renderer);
            centerText(renderer,"WORLD 2-1 CLEARED!",165,6);
            w21DrawTinySSD(renderer,ssdTexture,604,315);
            centerText(renderer,"SSD FOUND!",425,4);
            centerText(renderer,"A TO WORLD 2-2",560,3);
            if(trigger & (VPAD_BUTTON_A|VPAD_BUTTON_B)) {
                w22x=90.0f;w22y=GROUND_Y-PLAYER_H;w22vy=0.0f;
                w22Facing=1;w22Frame=0;w22SSD=0;w22StudCount=0;
                memset(w22Collected,0,sizeof(w22Collected));
                scene=22;musicStarted=1;lastPlayedW23=0;
            }
            if(trigger & VPAD_BUTTON_PLUS){scene=0;musicStarted=0;}
        } else if (scene == 6) {
            SDL_SetRenderDrawColor(renderer, 27, 30, 57, 255);
            SDL_RenderClear(renderer);
            centerText(renderer, "GAME OVER", 192, 9);
            centerText(renderer, "NO LIVES LEFT", 343, 4);
            centerText(renderer, "A TO TRY AGAIN", 484, 3);
            centerText(renderer, "PLUS TO MENU", 560, 3);
            if (trigger & (VPAD_BUTTON_A | VPAD_BUTTON_B)) {
                /* Restart the last course if World 2-2 caused Game Over. */
                if(lastPlayedW24) {
                    lives=3;
                    w24Reset(&w24x,&w24y,&w24vy,&w24Facing,&w24Frame,
                             w24PearlsTaken,&w24SSD,&w24Checkpoint,
                             &w24Sailing,&w24SafeUntil);
                    scene=26;musicStarted=1;
                } else if(lastPlayedW23) {
                    lives=3;
                    w23Reset(&w23x,&w23y,&w23vy,&w23Facing,&w23Frame,
                             w23Found,w23Taken,&w23NectarCount,&w23SSD,&w23Checkpoint);
                    w23SafeUntil=0;scene=23;musicStarted=1;
                } else if(lastPlayedW22) {
                    lives=3;w22x=90.0f;w22y=GROUND_Y-PLAYER_H;w22vy=0.0f;
                    w22Frame=0;w22SSD=0;w22StudCount=0;
                    memset(w22Collected,0,sizeof(w22Collected));
                    scene=22;musicStarted=1;
                } else {
                /* Restart a fresh 3-life run from World 1-1. */
                lives=3;
                resetGame(&x,&y,&velocityY,coinsTaken,&coinCount,
                          &animationFrame,&facingRight);
                x2=90; y2=GROUND_Y-PLAYER_H; vy2=0;
                blockUsed=0; bombReady=0; bombFlying=0;
                bossDefeated=0; ssdCollected=0; frame2=0;
                scene=1;
                clearBossLasers(bossLasers);
                bossArenaEntered=0;
                musicStarted=1;
                }
            }
            if (trigger & VPAD_BUTTON_PLUS) {
                scene=0;
                musicStarted=0;
            }
        } else if (scene == 5) {
            SDL_SetRenderDrawColor(renderer,24,45,73,255);
            SDL_RenderClear(renderer);
            centerText(renderer,"WORLD 1-2 CLEARED!",110,5);
            drawSSD(renderer,ssdTexture,520,225);
            centerText(renderer,"YOU FOUND A PART OF AN SSD!",390,3);
            centerText(renderer,"A TO WORLD 2-1",545,3);
            centerText(renderer,"PLUS TO MENU",615,2);
            if(trigger & (VPAD_BUTTON_A|VPAD_BUTTON_B)) {
                w21x=90.0f; w21y=GROUND_Y-PLAYER_H; w21vy=0.0f;
                w21Facing=1; w21Frame=0; w21SSD=0;
                scene=11; musicStarted=1;
            }
            if(trigger & VPAD_BUTTON_PLUS) { scene=0; musicStarted=0; }
        } else {
            SDL_SetRenderDrawColor(renderer, 31, 75, 82, 255);
            SDL_RenderClear(renderer);
            centerText(renderer, "LEVEL COMPLETE!", 178, 6);
            centerText(renderer, "WORLD 1-1 CLEARED", 315, 4);
            char result[32];
            SDL_snprintf(result, sizeof(result), "COINS: %d", coinCount);
            centerText(renderer, result, 400, 4);
            centerText(renderer, "A TO MENU", 565, 3);
            if (trigger & (VPAD_BUTTON_A | VPAD_BUTTON_B)) {
                x2=90.0f; y2=GROUND_Y-PLAYER_H; vy2=0;
                blockUsed=0; bombReady=0; bombFlying=0;
                bossDefeated=0; ssdCollected=0; frame2=0;
                clearBossLasers(bossLasers);
                bossArenaEntered = 0;
                lastBossShot = SDL_GetTicks();
                scene=4;
            }
            if (trigger & VPAD_BUTTON_PLUS) {scene=0; musicStarted=0;}
        }

        /* Event-driven sound effects. Avoid queue buildup from repeated events. */
        int soundEvent=-1;
        Uint32 soundNow=SDL_GetTicks();
        if (scene != soundPrevScene) {
            soundEvent=(scene==64)?SFX_VICTORY:SFX_LEVEL;
            soundPrevScene=scene;
        } else if (scene==63 && final6.bossHP<soundPrevBoss) {
            soundEvent=SFX_BOSS_HIT;
        } else if ((w3MiniHP<soundPrevW3)||(w4MiniHP<soundPrevW4)||(w5MiniHP<soundPrevW5)) {
            soundEvent=SFX_BOSS_HIT;
        } else if (scene==63 && final6.player.health<soundPrevHealth) {
            soundEvent=SFX_HURT;
        } else if (coinCount>soundPrevCoins) {
            soundEvent=SFX_PICKUP;
        } else if (scene!=0 && scene!=64 &&
                   (trigger & (VPAD_BUTTON_A | VPAD_BUTTON_B)) &&
                   soundNow-soundLastJump>230) {
            soundEvent=SFX_JUMP;
            soundLastJump=soundNow;
        }
        /* Sparse boss warning tone, only during the final fight. */
        if (soundEvent<0 && scene==63 && final6.bossHP>0 &&
            soundNow-soundLastShot>1900) {
            soundEvent=SFX_BOSS_SHOOT; soundLastShot=soundNow;
        }
        if (soundEvent>=0 && sfxDevice && sfxData[soundEvent]) {
            if (SDL_GetQueuedAudioSize(sfxDevice)<22050) {
                SDL_QueueAudio(sfxDevice,sfxData[soundEvent],sfxLength[soundEvent]);
            }
        }
        soundPrevCoins=coinCount; soundPrevBoss=final6.bossHP;
        soundPrevW3=w3MiniHP; soundPrevW4=w4MiniHP;
        soundPrevW5=w5MiniHP; soundPrevHealth=final6.player.health;

        /* Existing music also plays in World 2-1 until its own track is added. */
        if (audioDevice != 0) {
            if (musicStarted && (scene == 1 || scene == 4 || scene == 11 || scene == 12 || scene == 22 || scene == 23 || scene == 26 || scene == 40 || scene == 43 || scene == 53 || scene == 63)) {
                if (SDL_GetQueuedAudioSize(audioDevice) < musicLength &&
                    SDL_QueueAudio(audioDevice, musicBuffer, musicLength) != 0) {
                    musicStatus = 3;
                    SDL_Log("World 1 audio queue failed: %s", SDL_GetError());
                }
            } else if (SDL_GetQueuedAudioSize(audioDevice) != 0) {
                SDL_ClearQueuedAudio(audioDevice);
            }
        }
        /* On-screen diagnostic, visible on Wii U and Cemu, only in gameplay. */
        if (scene == 1 || scene == 4 || scene == 11 || scene == 12 || scene == 22 || scene == 23 || scene == 26 || scene == 40 || scene == 43 || scene == 53 || scene == 63) {
            if (musicStatus != 2) {
                fill(renderer, 350, 12, 555, 48, 133, 35, 35);
                drawText(renderer,
                         musicStatus == 0 ? "MUSIC FILE NOT FOUND" :
                         musicStatus == 1 ? "AUDIO DEVICE FAILED" :
                                            "AUDIO QUEUE FAILED",
                         365, 27, 3);
            } else if (SDL_GetQueuedAudioSize(audioDevice) > 0) {
                drawText(renderer, "MUSIC LOADED", 420, 28, 2);
            }
        }
        drawWorldAtmosphere(renderer, polishWorldForScene(scene), SDL_GetTicks());
        /* World boss health bars, one boss per world. */
        if(scene==4 && !bossDefeated) jumpBossBar(renderer,1,1,1);
        if(scene==11 && w2MiniHP>0) jumpBossBar(renderer,w2MiniHP,2,2);
        if(scene==40 && retro9Level==8 && w3MiniHP>0) jumpBossBar(renderer,w3MiniHP,2,3);
        if(scene==43 && w4Level==W4_LEVELS-1 && w4MiniHP>0) jumpBossBar(renderer,w4MiniHP,2,4);
        if(scene==53 && w5Level==W5_LEVELS-1 && w5MiniHP>0) jumpBossBar(renderer,w5MiniHP,2,5);
        if(scene==63 && final6.bossHP>0) jumpBossBar(renderer,final6.bossHP,6,6);
        /* Save after level index advances even when scene ID stays unchanged. */
        if((scene==40||scene==43||scene==53) && canContinue &&
            scene==loadedProgress.scene &&
            (retro9Level!=loadedProgress.retro9Level||w4Level!=loadedProgress.w4Level||w5Level!=loadedProgress.w5Level)) {
            if(jumpSave(scene,retro9Level,w4Level,w5Level,selectedCharacter)) {
                loadedProgress.retro9Level=retro9Level;loadedProgress.w4Level=w4Level;
                loadedProgress.w5Level=w5Level;
            }
        }
        /* Save only new gameplay checkpoints; never overwrite on menu/death. */
        if(scene!=polishScene) {
            int checkpoint=(scene==1||scene==4||scene==11||scene==22||
                scene==23||scene==26||scene==40||scene==43||scene==53||scene==63);
            if(checkpoint) {
                saveStatus=jumpSave(scene,retro9Level,w4Level,w5Level,selectedCharacter);
                if(saveStatus) {canContinue=1;loadedProgress.scene=scene;
                    loadedProgress.retro9Level=retro9Level;loadedProgress.w4Level=w4Level;
                    loadedProgress.w5Level=w5Level;loadedProgress.character=selectedCharacter;}
            }
            polishScene=scene;polishSceneAt=SDL_GetTicks();
        }
        if(scene!=0 && scene!=3 && scene!=70 && SDL_GetTicks()-polishSceneAt<800) {
            char title[40];int world=polishWorldForScene(scene);
            SDL_snprintf(title,sizeof(title),"WORLD %d",world?world:1);
            jumpFade(renderer,SDL_GetTicks()-polishSceneAt,title);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    for (int c = 0; c < 2; c++) {
        for (int i = 0; i < SPRITE_COUNT; i++) {
            if (characterSprites[c][i]) SDL_DestroyTexture(characterSprites[c][i]);
        }
    }
    if (parkTexture) SDL_DestroyTexture(parkTexture);
    if (w22Backdrop) SDL_DestroyTexture(w22Backdrop);
    for (int i=0;i<6;i++) if (bossSprites[i]) SDL_DestroyTexture(bossSprites[i]);
    if (ssdTexture) SDL_DestroyTexture(ssdTexture);
if (sfxDevice) SDL_CloseAudioDevice(sfxDevice);
    for (int si=0;si<SFX_COUNT;++si) if (sfxData[si]) SDL_FreeWAV(sfxData[si]);
if (bootDevice) SDL_CloseAudioDevice(bootDevice);
    if (bootBuffer) SDL_FreeWAV(bootBuffer);
    if (audioDevice != 0) SDL_CloseAudioDevice(audioDevice);
    if (musicBuffer) SDL_FreeWAV(musicBuffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
