#pragma once
#include"Map.hpp"
#include<cmath>

struct Player
{
    double x=3.5;// player x pos
    double y=3.5; // player y pos
    double dirX=1.0; // view dir x
    double dirY=0.0; // view dir y
    double planeX=0.0; // camera plane x
    double planeY=0.66; // camera plane y (FOV)

    static constexpr double MOVE_SPEED=3.2;// move speed
    static constexpr double ROT_SPEED=2.2;// rotate speed
    static constexpr double PLAYER_RADIUS=0.2; // collision radius

    // try move with wall collision
    void tryMove(double dx,double dy)
    {
        double nx=x+dx;
        double ny=y+dy;
        if(!GameMap::isWall(int(nx+(dx>0?PLAYER_RADIUS:-PLAYER_RADIUS)),int(y)))x=nx;
        if(!GameMap::isWall(int(x),int(ny+(dy>0?PLAYER_RADIUS:-PLAYER_RADIUS))))y=ny;
    }

    // rotate view and camera plane
    void rotate(double angle)
    {
        double oldDirX=dirX;
        dirX=dirX*std::cos(angle)-dirY*std::sin(angle);
        dirY=oldDirX*std::sin(angle)+dirY*std::cos(angle);
        double oldPlaneX=planeX;
        planeX=planeX*std::cos(angle)-planeY*std::sin(angle);
        planeY=oldPlaneX*std::sin(angle)+planeY*std::cos(angle);
    }

    // update player from input
    void update(const bool* keys,double dt)
    {
        double moveStep=MOVE_SPEED*dt;
        double rotStep=ROT_SPEED*dt;
        double forward=0.0;
        double strafe=0.0;

        // get movement input
        if(keys[SDL_SCANCODE_W]||keys[SDL_SCANCODE_UP])forward+=1.0;
        if(keys[SDL_SCANCODE_S]||keys[SDL_SCANCODE_DOWN])forward-=1.0;
        if(keys[SDL_SCANCODE_D])strafe+=1.0;
        if(keys[SDL_SCANCODE_A])strafe-=1.0;

        // move if input exist
        if(forward!=0.0||strafe!=0.0)
        {
            double len=std::sqrt(forward*forward+strafe*strafe);
            forward/=len;
            strafe/=len;
            double dx=(dirX*forward+planeX*strafe)*moveStep;
            double dy=(dirY*forward+planeY*strafe)*moveStep;
            tryMove(dx,dy);
        }

        // rotate if input exist
        if(keys[SDL_SCANCODE_LEFT])rotate(-rotStep);
        if(keys[SDL_SCANCODE_RIGHT])rotate(rotStep);
    }
};