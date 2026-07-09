#include<SDL3/SDL.h>
#include<cmath>
#include<cstdio>
#include"Map.hpp"
#include"Player.hpp"
#include"Textures.hpp"
using namespace std;

const int SCREEN_WIDTH=1920;
const int SCREEN_HEIGHT=1080;
static Uint32 framebuffer[SCREEN_WIDTH*SCREEN_HEIGHT];

// Raycast hit info: distance, side (0=x, 1=y), wall type, texture x-coord
struct WallHit
{
    double perpDist; // Perpendicular distance to wall
    int side;        // 0 for x-side, 1 for y-side hit
    int mapValue;    // Wall texture index
    double wallX;    // Hit point on wall (0-1)
};

// Clamp integer to range
int clampInt(int v,int lo,int hi)
{
    if(v<lo)return lo;
    if(v>hi)return hi;
    return v;
}

// Clamp double to range
double clampDouble(double v,double lo,double hi)
{
    if(v<lo)return lo;
    if(v>hi)return hi;
    return v;
}

// DDA raycasting algorithm
WallHit castRay(const Player& player,double rayDirX,double rayDirY)
{
    int mapX=int(player.x);
    int mapY=int(player.y);

    // Avoid division by zero
    double deltaDistX=(rayDirX==0)?1e30:fabs(1.0/rayDirX);
    double deltaDistY=(rayDirY==0)?1e30:fabs(1.0/rayDirY);

    int stepX,stepY;
    double sideDistX,sideDistY;

    // Calculate step direction and initial side distances
    if(rayDirX<0)
    {
        stepX=-1;
        sideDistX=(player.x-mapX)*deltaDistX;
    }
    else
    {
        stepX=1;
        sideDistX=(mapX+1.0-player.x)*deltaDistX;
    }

    if(rayDirY<0)
    {
        stepY=-1;
        sideDistY=(player.y-mapY)*deltaDistY;
    }
    else
    {
        stepY=1;
        sideDistY=(mapY+1.0-player.y)*deltaDistY;
    }

    int side=0;
    int hitValue=0;

    // March until wall hit or max steps
    for(int i=0;i<256;i++)
    {
        if(sideDistX<sideDistY)
        {
            sideDistX+=deltaDistX;
            mapX+=stepX;
            side=0;
        }
        else
        {
            sideDistY+=deltaDistY;
            mapY+=stepY;
            side=1;
        }
        hitValue=GameMap::at(mapX,mapY);
        if(hitValue>0)break;
    }

    // Correct for fish-eye effect
    double perpDist=(side==0)?sideDistX-deltaDistX:sideDistY-deltaDistY;
    if(perpDist<1e-6)perpDist=1e-6;

    // Calculate texture x-coordinate
    double wallX=(side==0)?player.y+perpDist*rayDirY:player.x+perpDist*rayDirX;
    wallX=wallX-floor(wallX);

    return{perpDist,side,hitValue,wallX};
}

int main(int argc,char** argv)
{
    // SDL initialization
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("SDL_Init failed: %s\n",SDL_GetError());
        return 1;
    }

    // Window setup
    SDL_Window* window=SDL_CreateWindow("Raycaster Engine",SCREEN_WIDTH,SCREEN_HEIGHT,0);
    if(!window)
    {
        printf("SDL_CreateWindow failed: %s\n",SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Renderer setup
    SDL_Renderer* renderer=SDL_CreateRenderer(window,nullptr);
    if(!renderer)
    {
        printf("SDL_CreateRenderer failed: %s\n",SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Frame texture for rendering
    SDL_Texture* frameTexture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,SCREEN_WIDTH,SCREEN_HEIGHT);
    SDL_SetTextureScaleMode(frameTexture,SDL_SCALEMODE_NEAREST);

    // Load textures and initialize player
    auto textures=Textures::generateAll();
    Player player;
    SDL_SetWindowRelativeMouseMode(window,true);

    // Game state
    bool running=true;
    bool showMinimap=true;
    Uint64 lastTicks=SDL_GetPerformanceCounter();
    double freq=double(SDL_GetPerformanceFrequency());

    // FPS tracking
    double fpsTimer=0.0;
    int frameCount=0;
    char titleBuf[128];

    // Main game loop
    while(running)
    {
        // Frame timing
        Uint64 nowTicks=SDL_GetPerformanceCounter();
        double dt=double(nowTicks-lastTicks)/freq;
        lastTicks=nowTicks;
        if(dt>0.05)dt=0.05; // Cap frame time

        // Event handling
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            if(event.type==SDL_EVENT_QUIT)
            {
                running=false;
            }
            else if(event.type==SDL_EVENT_KEY_DOWN)
            {
                if(event.key.scancode==SDL_SCANCODE_ESCAPE)running=false;
                if(event.key.scancode==SDL_SCANCODE_M)showMinimap=!showMinimap;
            }
            else if(event.type==SDL_EVENT_MOUSE_MOTION)
            {
                player.rotate(event.motion.xrel*0.0025); // Mouse look
            }
        }

        // Update player from keyboard
        const bool* keys=SDL_GetKeyboardState(nullptr);
        player.update(keys,dt);

        // Render floor and ceiling
        for(int y=0;y<SCREEN_HEIGHT;y++)
        {
            bool isFloor=y>SCREEN_HEIGHT/2;
            // Ray directions for left/right edges
            double rayDirX0=player.dirX-player.planeX;
            double rayDirY0=player.dirY-player.planeY;
            double rayDirX1=player.dirX+player.planeX;
            double rayDirY1=player.dirY+player.planeY;

            // Distance from camera to floor plane
            int p=isFloor?(y-SCREEN_HEIGHT/2):(SCREEN_HEIGHT/2-y);
            if(p==0)p=1;
            double posZ=0.5*SCREEN_HEIGHT;
            double rowDistance=posZ/p;

            // Floor step vectors
            double floorStepX=rowDistance*(rayDirX1-rayDirX0)/SCREEN_WIDTH;
            double floorStepY=rowDistance*(rayDirY1-rayDirY0)/SCREEN_WIDTH;
            double floorX=player.x+rowDistance*rayDirX0;
            double floorY=player.y+rowDistance*rayDirY0;

            for(int x=0;x<SCREEN_WIDTH;x++)
            {
                // Texture coordinates
                int cellX=int(floorX);
                int cellY=int(floorY);
                int tx=int((floorX-cellX)*Textures::TEX_SIZE)&(Textures::TEX_SIZE-1);
                int ty=int((floorY-cellY)*Textures::TEX_SIZE)&(Textures::TEX_SIZE-1);
                floorX+=floorStepX;
                floorY+=floorStepY;

                // Sample texture and apply fog
                Uint32 color=isFloor?textures[4][ty][tx]:textures[2][ty][tx];
                double fog=clampDouble(1.4-rowDistance*0.06,0.25,1.0);
                int a=(color>>24)&0xFF;
                int r=int(((color>>16)&0xFF)*fog);
                int g=int(((color>>8)&0xFF)*fog);
                int b=int((color&0xFF)*fog);
                framebuffer[y*SCREEN_WIDTH+x]=(a<<24)|(r<<16)|(g<<8)|b;
            }
        }

        // Render walls
        for(int x=0;x<SCREEN_WIDTH;x++)
        {
            // Camera space
            double cameraX=2.0*x/double(SCREEN_WIDTH)-1.0;
            double rayDirX=player.dirX+player.planeX*cameraX;
            double rayDirY=player.dirY+player.planeY*cameraX;

            // Cast ray and get hit info
            WallHit hit=castRay(player,rayDirX,rayDirY);
            int lineHeight=int(SCREEN_HEIGHT/hit.perpDist);
            int drawStart=-lineHeight/2+SCREEN_HEIGHT/2;
            int drawEnd=lineHeight/2+SCREEN_HEIGHT/2;
            if(drawStart<0)drawStart=0;
            if(drawEnd>SCREEN_HEIGHT-1)drawEnd=SCREEN_HEIGHT-1;

            // Texture x-coordinate
            int texX=int(hit.wallX*Textures::TEX_SIZE);
            if((hit.side==0&&rayDirX>0)||(hit.side==1&&rayDirY<0))texX=Textures::TEX_SIZE-texX-1;
            texX=clampInt(texX,0,Textures::TEX_SIZE-1);

            // Shading based on wall orientation and distance
            double shade=(hit.side==1)?0.7:1.0;
            double fog=clampDouble(1.0-hit.perpDist*0.045,0.25,1.0);
            double totalShade=shade*fog;

            // Sample wall texture
            const auto& tex=textures[hit.mapValue];
            double step=double(Textures::TEX_SIZE)/lineHeight;
            double texPos=(drawStart-SCREEN_HEIGHT/2+lineHeight/2)*step;

            for(int y=drawStart;y<=drawEnd;y++)
            {
                int texY=clampInt(int(texPos),0,Textures::TEX_SIZE-1);
                texPos+=step;
                Uint32 color=tex[texY][texX];
                int a=(color>>24)&0xFF;
                int r=int(((color>>16)&0xFF)*totalShade);
                int g=int(((color>>8)&0xFF)*totalShade);
                int b=int((color&0xFF)*totalShade);
                framebuffer[y*SCREEN_WIDTH+x]=(a<<24)|(r<<16)|(g<<8)|b;
            }
        }

        // Update texture and render
        SDL_UpdateTexture(frameTexture,nullptr,framebuffer,SCREEN_WIDTH*sizeof(Uint32));
        SDL_RenderTexture(renderer,frameTexture,nullptr,nullptr);

        // Minimap overlay
        if(showMinimap)
        {
            int cell=6;
            int originX=12;
            int originY=12;

            // Draw map cells
            for(int y=0;y<GameMap::HEIGHT;y++)
            {
                for(int x=0;x<GameMap::WIDTH;x++)
                {
                    SDL_FRect rect={float(originX+x*cell),float(originY+y*cell),float(cell-1),float(cell-1)};
                    if(GameMap::isWall(x,y))SDL_SetRenderDrawColor(renderer,200,200,200,230);
                    else SDL_SetRenderDrawColor(renderer,40,40,40,180);
                    SDL_RenderFillRect(renderer,&rect);
                }
            }

            // Draw player
            SDL_SetRenderDrawColor(renderer,255,60,60,255);
            SDL_FRect playerRect={float(originX+player.x*cell-2),float(originY+player.y*cell-2),4,4};
            SDL_RenderFillRect(renderer,&playerRect);

            // Draw player direction
            SDL_SetRenderDrawColor(renderer,255,220,60,255);
            float px=float(originX+player.x*cell);
            float py=float(originY+player.y*cell);
            SDL_RenderLine(renderer,px,py,px+float(player.dirX*cell*1.5),py+float(player.dirY*cell*1.5));
        }

        // Present frame
        SDL_RenderPresent(renderer);

        // Update FPS counter
        fpsTimer+=dt;
        frameCount++;
        if(fpsTimer>=0.5)
        {
            snprintf(titleBuf,sizeof(titleBuf),"Raycaster Engine | %.0f FPS",frameCount/fpsTimer);
            SDL_SetWindowTitle(window,titleBuf);
            fpsTimer=0.0;
            frameCount=0;
        }
    }

    // Cleanup
    SDL_DestroyTexture(frameTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}