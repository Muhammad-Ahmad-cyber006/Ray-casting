#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cmath>
using namespace std;

namespace Textures
{
    const int TEX_SIZE=64;
    const int TEX_COUNT=7;

    typedef array<array<uint32_t,TEX_SIZE>,TEX_SIZE> TexBuffer;

    uint32_t packARGB(uint8_t a,uint8_t r,uint8_t g,uint8_t b)
    {
        return (uint32_t(a)<<24)|(uint32_t(r)<<16)|(uint32_t(g)<<8)|uint32_t(b);
    }

    int clampInt(int v,int lo,int hi)
    {
        if(v<lo)return lo;
        if(v>hi)return hi;
        return v;
    }

    uint8_t clampByte(int v){return uint8_t(clampInt(v,0,255));}

    void setPx(TexBuffer& tex,int x,int y,int r,int g,int b)
    {
        if(x<0||x>=TEX_SIZE||y<0||y>=TEX_SIZE)return;
        tex[y][x]=packARGB(255,clampByte(r),clampByte(g),clampByte(b));
    }

    void fillRect(TexBuffer& tex,int x0,int y0,int x1,int y1,int r,int g,int b)
    {
        int startY=clampInt(y0,0,TEX_SIZE-1);
        int endY=clampInt(y1,0,TEX_SIZE-1);
        int startX=clampInt(x0,0,TEX_SIZE-1);
        int endX=clampInt(x1,0,TEX_SIZE-1);
        for(int y=startY;y<=endY;y++)
        {
            for(int x=startX;x<=endX;x++)
            {
                tex[y][x]=packARGB(255,clampByte(r),clampByte(g),clampByte(b));
            }
        }
    }
    // simple hash for randomness
    double hash2(int x,int y)
    {
        uint32_t h=uint32_t(x)*374761393u+uint32_t(y)*668265263u;
        h=(h^(h>>13))*1274126177u;
        h^=(h>>16);
        return double(h&0xFFFFFF)/double(0xFFFFFF);
    }

    namespace Pal
    {
        const int MORTAR[3]={46,42,40};
        const int BRICK_A[3]={150,66,48};
        const int BRICK_B[3]={128,54,40};
        const int BRICK_HI[3]={176,90,64};
        const int STONE_LO[3]={70,68,72};
        const int STONE_MID[3]={92,90,96};
        const int STONE_HI[3]={114,112,118};
        const int MOSS[3]={64,108,54};
        const int MOSS_DARK[3]={40,74,38};
        const int WOOD_A[3]={132,92,54};
        const int WOOD_B[3]={110,76,44};
        const int WOOD_DARK[3]={70,48,28};
        const int IRON[3]={60,60,66};
        const int IRON_HI[3]={96,96,104};
        const int IRON_DARK[3]={36,36,40};
        const int FLAME_OUT[3]={196,90,30};
        const int FLAME_MID[3]={230,150,40};
        const int FLAME_TIP[3]={250,220,120};
    }

    void buildBrick(TexBuffer& tex)
    {
        int cellW=16,cellH=8,mortar=2;
        for(int y=0;y<TEX_SIZE;y++)
        {
            int row=y/cellH;
            int offset=(row%2)*(cellW/2);
            int by=y%cellH;
            for(int x=0;x<TEX_SIZE;x++)
            {
                int bx=(x+offset)%cellW;
                if(bx<mortar||by<mortar)
                {
                    setPx(tex,x,y,Pal::MORTAR[0],Pal::MORTAR[1],Pal::MORTAR[2]);
                    continue;
                }
                int brickIdxX=(x+offset)/cellW;
                const int* base=((brickIdxX+row)%2==0)?Pal::BRICK_A:Pal::BRICK_B;
                setPx(tex,x,y,(by==mortar)?Pal::BRICK_HI[0]:base[0],(by==mortar)?Pal::BRICK_HI[1]:base[1],(by==mortar)?Pal::BRICK_HI[2]:base[2]);
            }
        }
    }

    void buildStoneBlocks(TexBuffer& tex)
    {
        int cellW=32,cellH=16,mortar=2;
        for(int y=0;y<TEX_SIZE;y++)
        {
            int row=y/cellH;
            int offset=(row%2)*(cellW/2);
            int by=y%cellH;
            for(int x=0;x<TEX_SIZE;x++)
            {
                int bx=(x+offset)%cellW;
                if(bx<mortar||by<mortar)
                {
                    setPx(tex,x,y,Pal::MORTAR[0],Pal::MORTAR[1],Pal::MORTAR[2]);
                    continue;
                }
                if(bx==mortar||by==mortar)
                    setPx(tex,x,y,Pal::STONE_HI[0],Pal::STONE_HI[1],Pal::STONE_HI[2]);
                else if(bx==cellW-1||by==cellH-1)
                    setPx(tex,x,y,Pal::STONE_LO[0],Pal::STONE_LO[1],Pal::STONE_LO[2]);
                else
                    setPx(tex,x,y,Pal::STONE_MID[0],Pal::STONE_MID[1],Pal::STONE_MID[2]);
            }
        }
    }

    void buildMossyBrick(TexBuffer& tex)
    {
        buildBrick(tex);
        int cellW=16,cellH=8,mortar=2;
        for(int row=0;row<TEX_SIZE/cellH;row++)
        {
            int offset=(row%2)*(cellW/2);
            for(int brickIdxX=-1;brickIdxX*cellW-offset<TEX_SIZE;brickIdxX++)
            {
                double growChance=0.25+0.45*(double(row)/(TEX_SIZE/cellH));
                if(hash2(brickIdxX,row)>growChance)continue;
                int x0=brickIdxX*cellW-offset+mortar;
                int x1=x0+cellW-mortar-1;
                int y0=row*cellH+mortar;
                int y1=y0+cellH-1;
                fillRect(tex,x0,y0,x1,y1,Pal::MOSS[0],Pal::MOSS[1],Pal::MOSS[2]);
                fillRect(tex,x0,y1,x1,y1+1,Pal::MOSS_DARK[0],Pal::MOSS_DARK[1],Pal::MOSS_DARK[2]);
            }
        }
    }

    void drawDoorBand(TexBuffer& tex,int y0)
    {
        fillRect(tex,0,y0,TEX_SIZE-1,y0+5,Pal::IRON[0],Pal::IRON[1],Pal::IRON[2]);
        fillRect(tex,0,y0,TEX_SIZE-1,y0,Pal::IRON_HI[0],Pal::IRON_HI[1],Pal::IRON_HI[2]);
        fillRect(tex,0,y0+5,TEX_SIZE-1,y0+5,Pal::IRON_DARK[0],Pal::IRON_DARK[1],Pal::IRON_DARK[2]);
        for(int x=4;x<TEX_SIZE;x+=8)
        {
            fillRect(tex,x,y0+1,x+1,y0+2,Pal::IRON_HI[0],Pal::IRON_HI[1],Pal::IRON_HI[2]);
        }
    }

    void buildIronDoor(TexBuffer& tex)
    {
        fillRect(tex,0,0,TEX_SIZE-1,TEX_SIZE-1,Pal::WOOD_A[0],Pal::WOOD_A[1],Pal::WOOD_A[2]);
        for(int px=0;px<3;px++)
        {
            int x0=px*(TEX_SIZE/3);
            fillRect(tex,x0,0,x0+1,TEX_SIZE-1,Pal::WOOD_DARK[0],Pal::WOOD_DARK[1],Pal::WOOD_DARK[2]);
            if(px%2==1)
                fillRect(tex,x0+2,0,x0+(TEX_SIZE/3)-1,TEX_SIZE-1,Pal::WOOD_B[0],Pal::WOOD_B[1],Pal::WOOD_B[2]);
        }
        drawDoorBand(tex,12);
        drawDoorBand(tex,TEX_SIZE-20);
        int hx=44,hy=32;
        for(int y=-4;y<=4;y++)
        {
            for(int x=-4;x<=4;x++)
            {
                double d=sqrt(double(x*x+y*y));
                if(d<4.2&&d>2.2)
                    setPx(tex,hx+x,hy+y,Pal::IRON_HI[0],Pal::IRON_HI[1],Pal::IRON_HI[2]);
            }
        }
        fillRect(tex,hx-1,hy-1,hx+1,hy+1,Pal::IRON_DARK[0],Pal::IRON_DARK[1],Pal::IRON_DARK[2]);
    }

    void buildWoodWall(TexBuffer& tex)
    {
        int plankH=10;
        for(int y=0;y<TEX_SIZE;y++)
        {
            int plankIdx=y/plankH;
            int ly=y%plankH;
            const int* base=(plankIdx%2==0)?Pal::WOOD_A:Pal::WOOD_B;
            for(int x=0;x<TEX_SIZE;x++)
            {
                if(ly<2)
                    setPx(tex,x,y,Pal::WOOD_DARK[0],Pal::WOOD_DARK[1],Pal::WOOD_DARK[2]);
                else
                    setPx(tex,x,y,base[0],base[1],base[2]);
            }
        }
        fillRect(tex,TEX_SIZE/2-2,0,TEX_SIZE/2+1,TEX_SIZE-1,Pal::WOOD_DARK[0],Pal::WOOD_DARK[1],Pal::WOOD_DARK[2]);
        fillRect(tex,TEX_SIZE/2-2,0,TEX_SIZE/2-2,TEX_SIZE-1,Pal::IRON_DARK[0],Pal::IRON_DARK[1],Pal::IRON_DARK[2]);
        fillRect(tex,TEX_SIZE/2+1,0,TEX_SIZE/2+1,TEX_SIZE-1,Pal::IRON_DARK[0],Pal::IRON_DARK[1],Pal::IRON_DARK[2]);
    }

    void buildTorchStone(TexBuffer& tex)
    {
        buildStoneBlocks(tex);
        int cx=TEX_SIZE/2;
        fillRect(tex,cx-1,30,cx+1,44,Pal::IRON_DARK[0],Pal::IRON_DARK[1],Pal::IRON_DARK[2]);
        fillRect(tex,cx-6,42,cx+6,45,Pal::IRON[0],Pal::IRON[1],Pal::IRON[2]);
        fillRect(tex,cx-6,42,cx+6,42,Pal::IRON_HI[0],Pal::IRON_HI[1],Pal::IRON_HI[2]);
        fillRect(tex,cx-5,18,cx+5,29,Pal::FLAME_OUT[0],Pal::FLAME_OUT[1],Pal::FLAME_OUT[2]);
        fillRect(tex,cx-3,12,cx+3,20,Pal::FLAME_MID[0],Pal::FLAME_MID[1],Pal::FLAME_MID[2]);
        fillRect(tex,cx-1,8,cx+1,14,Pal::FLAME_TIP[0],Pal::FLAME_TIP[1],Pal::FLAME_TIP[2]);
    }

    array<TexBuffer,TEX_COUNT> generateAll()
    {
        array<TexBuffer,TEX_COUNT> textures={};
        buildBrick(textures[1]);
        buildStoneBlocks(textures[2]);
        buildMossyBrick(textures[3]);
        buildIronDoor(textures[4]);
        buildWoodWall(textures[5]);
        buildTorchStone(textures[6]);
        return textures;
    }
}