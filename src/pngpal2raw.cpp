/******************************************************************************/
// PNG and PAL to RAW/DAT/SPR files converter for KeeperFX
/******************************************************************************/
/** @file pngpal2raw.cpp
 *     Program code file.
 * @par Purpose:
 *     Contains code to read PNG files and convert them to 8bpp RAWs with
 *     special palette generated with Png2bestPal. Based on png2ico project.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis <listom@gmail.com>
 * @author   Matthias S. Benkmann <matthias@winterdrache.de>
 * @date     25 Jul 2012 - 18 Aug 2012
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/

#include <cstdio>
#include <vector>
#include <climits>
#include <getopt.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <fstream>
#include <sstream>

#include <png.h>

#include "ci_string.hpp"
#include "prog_options.hpp"
#include "imagedata.hpp"
#include "pngpal2raw_ver.h"

using namespace std;

int verbose = 0;

class WorkingSet
{
public:
    WorkingSet():alg(DfsAlg_FldStnbrg),lvl(0),requested_colors(0),requested_col_bits(0){}
    void requestedColors(unsigned reqColors)
    {
        requested_colors = reqColors;
        for (requested_col_bits=1; (1u<<requested_col_bits) < requested_colors; requested_col_bits++);
        paletteRemap.resize(requested_colors);
        for (unsigned i = 0; i < requested_colors; i++)
            paletteRemap[i] = i;
    }
    unsigned requestedColors(void) const
    { return requested_colors; }
    void ditherLevel(int level)
    {
        lvl = level;
        lvlCurve.resize(512);
        lvlCurve[256+0] = 0;
        if (level > 0) {
            for (int i=1; i < 256; i++) {
                lvlCurve[256+i] = pow(i,lvl/100.);
                lvlCurve[256-i] = -lvlCurve[256+i];
            }
        } else {
            for (int i=1; i < 256; i++) {
                lvlCurve[256+i] = 0;
                lvlCurve[256-i] = 0;
            }
        }
    }
    unsigned requestedColorBPP(void) const
    { return requested_col_bits; }
    void addPaletteQuad(RGBAQuad quad)
    {
        int palentry = palette.size();
        palette.push_back(RGBColor(quad));
        mapQuadToPalEntry[quad] = palentry;
    }
    //std::vector<ImageData> images;
    ColorPalette palette;
    std::vector<int> paletteRemap;
    DitherError mapErrorR;
    DitherError mapErrorG;
    DitherError mapErrorB;
    MapQuadToPal mapQuadToPalEntry;
    std::vector<float> lvlCurve;
    int alg;
    int lvl;
private:
    unsigned requested_colors;
    unsigned requested_col_bits;
};

/* to avoid indices below 0 in dithering error array */
#define SHIFT 3

float dif[11][3][6] =
{
   {
      {0,        0,        0,        0,        7.0/16.0, 0},
      {0,        0,        3.0/16.0, 5.0/16.0, 1.0/16.0, 0},
      {0,        0       , 0,        0,        0,        0}
   },
   {
      {0,        0,        0,        0,        7.0/48.0, 5.0/48.0},
      {0,        3.0/48.0, 5.0/48.0, 7.0/48.0, 5.0/48.0, 3.0/48.0},
      {0,        1.0/48.0, 3.0/48.0, 5.0/48.0, 3.0/48.0, 1.0/48.0}
   },
   {
      {0,        0,        0,        0,        8.0/42.0, 4.0/42.0},
      {0,        2.0/42.0, 4.0/42.0, 8.0/42.0, 4.0/42.0, 2.0/42.0},
      {0,        1.0/42.0, 2.0/42.0, 4.0/42.0, 2.0/42.0, 1.0/42.0}
   },
   {
      {0,        0,        0,        0,        8.0/32.0, 4.0/32.0},
      {0,        2.0/32.0, 4.0/32.0, 8.0/32.0, 4.0/32.0, 2.0/32.0},
      {0,        0       , 0,        0,        0,        0}
   },
   {
      {0,        0,        0,        0,        7.0/16.0, 0},
      {0,        1.0/16.0, 3.0/16.0, 5.0/16.0, 0,        0},
      {0,        0       , 0,        0,        0,        0}
   },
   {
      {0,        0,        0,        0,        5.0/32.0, 3.0/32.0},
      {0,        2.0/32.0, 4.0/32.0, 5.0/32.0, 4.0/32.0, 2.0/32.0},
      {0,        0       , 2.0/32.0, 3.0/32.0, 2.0/32.0, 0}
   },
   {
      {0,        0,        0,        0,        4.0/16.0, 3.0/16.0},
      {0,        1.0/16.0, 2.0/16.0, 3.0/16.0, 2.0/16.0, 1.0/16.0},
      {0,        0       , 0,        0,        0,        0}
   },
   {
      {0,        0,        0,        0,        2.0/4.0,  0},
      {0,        0,        1.0/4.0,  1.0/4.0,  0,        0},
      {0,        0       , 0,        0,        0,        0}
   },
   {
      {0,        0,        0,        0,        1.0/8.0,  1.0/8.0},
      {0,        0,        1.0/8.0,  1.0/8.0,  1.0/8.0,  0},
      {0,        0,        0,        1.0/8.0,  0,        0}
   },
   {
      {0,        0,        0,        0,        4.0/8.0,  0},
      {0,        1.0/8.0,  1.0/8.0,  2.0/8.0,  0,        0},
      {0,        0,        0,        0,        0,        0}
   },
   {
      {0,        0,        0,        0,        8.0/16.0, 0},
      {1.0/16.0, 1.0/16.0, 2.0/16.0, 4.0/16.0, 0,        0},
      {0,        0,        0,        0,        0,        0}
   }
};

/**
 * Cuts given value to color range (0..255).
 * @param x Value to be verified and clipped.
 * @return Gives value clipped to valid color range.
 */
int clipIntensity(long x)
{
   if (x > 255) return 255;
   if (x < 0) return 0;
   return x;
}


void writeByte(FILE* f, int byte)
{
    char data[1];
    data[0]=(byte&255);
    if (fwrite(data,1,1,f)!=1) {perror("Write error"); exit(1);}
}

int andMaskLineLen(const ImageData& img)
{
    int len=(img.width+7)>>3;
    return (len+3)&~3;
}

int xorMaskLineLen(const ImageData& img)
{
    int pixelsPerByte = (8 / img.colorBPP());
    return ((img.width+pixelsPerByte-1)/pixelsPerByte+3)&~3;
}

typedef bool (*checkTransparent_t)(png_bytep, ImageData&);

bool checkTransparent1(png_bytep data, ImageData& img)
{
    return (data[3] < img.transparency_threshold);
}

bool checkTransparent3(png_bytep, ImageData&)
{
    return false;
}

int nearest_palette_color_index(const ColorPalette& palette, const RGBAQuad quad)
{
    int red=quad&255;  //must be signed
    int green=(quad>>8)&255;
    int blue=(quad>>16)&255;
    int minDist=INT_MAX;
    int bestIndex=0;
    ColorPalette::const_iterator paliter;
    for (paliter = palette.begin(); paliter != palette.end(); paliter++)
    {
        int dist=(red - paliter->red);
        dist*=dist;
        int temp=(green - paliter->green);
        dist+=temp*temp;
        temp=(blue - paliter->blue);
        dist+=temp*temp;
        if (dist<minDist) {
            minDist=dist;
            bestIndex = (paliter - palette.begin());
        }
    }
    return bestIndex;
}

/**
 * Propagates an error into adjacent cells.
 * @param alg Diffusion algorithm index.
 * @param lvl Diffusion level.
 * @param wd Error delta value.
 * @param e The error array.
 * @param i Error central coordinate.
 * @param j Error central coordinate.
 */
void propagateError(int alg, float w, DitherError &e, int i, int j)
{
    e[i+1+SHIFT][j  ] = e[i+1+SHIFT][j  ] + (w*dif[alg][0][4]);
    e[i+2+SHIFT][j  ] = e[i+2+SHIFT][j  ] + (w*dif[alg][0][5]);

    e[i-3+SHIFT][j+1] = e[i-3+SHIFT][j+1] + (w*dif[alg][1][0]);
    e[i-2+SHIFT][j+1] = e[i-2+SHIFT][j+1] + (w*dif[alg][1][1]);
    e[i-1+SHIFT][j+1] = e[i-1+SHIFT][j+1] + (w*dif[alg][1][2]);
    e[i  +SHIFT][j+1] = e[i  +SHIFT][j+1] + (w*dif[alg][1][3]);
    e[i+1+SHIFT][j+1] = e[i+1+SHIFT][j+1] + (w*dif[alg][1][4]);
    e[i+2+SHIFT][j+1] = e[i+2+SHIFT][j+1] + (w*dif[alg][1][5]);

    e[i-3+SHIFT][j+2] = e[i-3+SHIFT][j+2] + (w*dif[alg][2][0]);
    e[i-2+SHIFT][j+2] = e[i-2+SHIFT][j+2] + (w*dif[alg][2][1]);
    e[i-1+SHIFT][j+2] = e[i-1+SHIFT][j+2] + (w*dif[alg][2][2]);
    e[i  +SHIFT][j+2] = e[i  +SHIFT][j+2] + (w*dif[alg][2][3]);
    e[i+1+SHIFT][j+2] = e[i+1+SHIFT][j+2] + (w*dif[alg][2][4]);
    e[i+2+SHIFT][j+2] = e[i+2+SHIFT][j+2] + (w*dif[alg][2][5]);
}

int dithered_palette_color_index(WorkingSet& ws, const ColorPalette& palette, unsigned int x, unsigned int y, RGBAQuad quad)
{
    int red=(quad&255);  //must be signed
    int green=(quad>>8)&255;
    int blue=(quad>>16)&255;
    int alpha=(quad>>24)&255;
    red = clipIntensity(red + (ws.mapErrorR[x+SHIFT][y]+0.5));
    green = clipIntensity(green + (ws.mapErrorG[x+SHIFT][y]+0.5));
    blue = clipIntensity(blue + (ws.mapErrorB[x+SHIFT][y]+0.5));

    int bestIndex = nearest_palette_color_index(palette,(red)|(green<<8)|(blue<<16)|(alpha<<24));

    // Add dither error only for non-transparent pixels
    if (alpha > 192) {
        propagateError(ws.alg, ws.lvlCurve[256 + red - palette[bestIndex].red] , ws.mapErrorR, x, y);
        propagateError(ws.alg, ws.lvlCurve[256 + green - palette[bestIndex].green] , ws.mapErrorG, x, y);
        propagateError(ws.alg, ws.lvlCurve[256 + blue - palette[bestIndex].blue] , ws.mapErrorB, x, y);
    }

    return bestIndex;
}

short convert_rgb_to_indexed(WorkingSet& ws, ImageData& img, bool hasAlpha)
{
    checkTransparent_t checkTrans=checkTransparent1;
    int bytesPerPixel = (img.colorBPP()+7) >> 3;
    if (!hasAlpha)
    {
        checkTrans=checkTransparent3;
    }

    png_bytep* row_pointers=png_get_rows(img.png_ptr, img.info_ptr);

    img.transMap.resize2d(img.width,img.height);
    img.transMap.zeroize2d();
    ws.mapErrorR.resize2d(img.crop_height+2*SHIFT,img.crop_width+2*SHIFT);
    ws.mapErrorR.zeroize2d();
    ws.mapErrorG.resize2d(img.crop_height+2*SHIFT,img.crop_width+2*SHIFT);
    ws.mapErrorG.zeroize2d();
    ws.mapErrorB.resize2d(img.crop_height+2*SHIFT,img.crop_width+2*SHIFT);
    ws.mapErrorB.zeroize2d();

    //second pass: convert RGB to palette entries
    //for (int y=img.height-1; y>=0; --y)
    for (int y = 0; y < img.crop_height; y++)
    {
        png_bytep row = row_pointers[y];
        png_bytep pixel = row_pointers[img.crop_y+y] + img.crop_x*bytesPerPixel;
        ColorTranparency::Column& transPtr = img.transMap[y];

        for (int x = 0; x < img.crop_width; x++)
        {
            bool trans = (*checkTrans)(pixel,img);
            unsigned int quad = pixel[0] + (pixel[1]<<8) + (pixel[2]<<16);
            if (!trans) quad += (255<<24); //NOTE: alpha channel has already been set to 255 for non-transparent pixels, so this is correct even for images with alpha channel

            transPtr[x] = trans;

            int palentry = dithered_palette_color_index(ws, ws.palette, x, y, quad);
            row[x] = palentry;
            pixel += bytesPerPixel;
        }
    }

    img.col_bits = ws.requestedColorBPP();
    img.crop_x = 0;
    img.crop_y = 0;

    return ERR_OK;
}

/**
 * Writes 2-byte little-endian number to given FILE.
 */
inline void write_int16_le_file (FILE *fp, unsigned short x)
{
    fputc ((int) (x&255), fp);
    fputc ((int) ((x>>8)&255), fp);
}

/**
 * Writes 4-byte little-endian number to given FILE.
 */
inline void write_int32_le_file (FILE *fp, unsigned long x)
{
    fputc ((int) (x&255), fp);
    fputc ((int) ((x>>8)&255), fp);
    fputc ((int) ((x>>16)&255), fp);
    fputc ((int) ((x>>24)&255), fp);
}

/**
 * Packs a line of width pixels (1 byte per pixel) in row, with 8/nbits pixels packed into each byte.
 * @return the new number of bytes in row
 */
int raw_pack(png_bytep row,int width,int nbits)
{
    int pixelsPerByte=8/nbits;
    if (pixelsPerByte<=1) return width;
    int ander=(1<<nbits)-1;
    int outByte=0;
    int count=0;
    int outIndex=0;
    for (int i=0; i<width; ++i)
    {
        outByte+=(row[i]&ander);
        if (++count==pixelsPerByte)
        {
            row[outIndex]=outByte;
            count=0;
            ++outIndex;
            outByte=0;
        }
        outByte<<=nbits;
    }

    if (count>0)
    {
        outByte<<=nbits*(pixelsPerByte-count);
        row[outIndex]=outByte;
        ++outIndex;
    }

    return outIndex;
}

/**
 * Packs a line of pixels (1 byte per pixel) so that transparent bytes are RLE-encoded into HugeSprite.
 * @return the new number of bytes in row
 */
int hspr_pack(png_bytep out_row, const png_bytep inp_row, const ColorTranparency::Column& inp_trans, int width, const ColorPalette& palette)
{
    int area;
    int outIndex=0;
    int i=0;
    while (i < width)
    {
        // Filled
        area = 0;
        while ( (i+area < width) && (!inp_trans[i+area]) )
            area++;
        *(long *)(out_row+outIndex) = area;
        outIndex += sizeof(long);
        memcpy(out_row+outIndex, inp_row+i, area);
        outIndex += area;
        i += area;
        // Transparent
        area = 0;
        while ( (i+area < width) && inp_trans[i+area] )
            area++;
        *(long *)(out_row+outIndex) = area;
        outIndex += sizeof(long);
        i += area;
    }
    return outIndex;
}

#pragma pack(1)

/**
 * Structure defining TAB file entry for Small Sprite format DAT/TAB files.
 */
struct SmallSpriteV1 {
    /** Offset of the sprite data in DAT file. */
    uint32_t Data;
    /** Width of the sprite data. */
    unsigned char SWidth;
    /** Height of the sprite data. */
    unsigned char SHeight;
};

/**
 * Structure defining TAB file entry for Jonty Sprite format JTY/TAB files.
 */
struct JontySpriteV1 {
    /** Offset of the sprite data in DAT file. */
    uint32_t Data;
    /** Width of the sprite data. */
    unsigned char SWidth;
    /** Height of the sprite data. */
    unsigned char SHeight;
    /** Width of the animation frame (same for whole animation). */
    unsigned char FrameWidth;
    /** Height of the animation frame (same for whole animation). */
    unsigned char FrameHeight;
    /** Flags informing whether the animation is rotable; 0 - non-rotable, 2 - rotable. */
    unsigned char Rotable;
    /** Amount of frames making up the animation (same for whole animation). */
    unsigned char FramesCount;
    /** Offset of the sprite within frame; width shift. */
    char FrameOffsW;
    /** Offset of the sprite within frame; height shift. */
    char FrameOffsH;
    /** Unidentified negative value A. */
    signed short unkn6;
    /** Unidentified negative value B. */
    signed short unkn8;
};

/**
 * Structure defining TAB file entry for Small Sprite format Ver2 DAT/TAB files.
 */
struct SmallSpriteV2 {
    /** Offset of the sprite data in DAT file. */
    uint32_t Data;
    /** Width of the sprite data. */
    unsigned short SWidth;
    /** Height of the sprite data. */
    unsigned short SHeight;
};

/**
 * Structure defining TAB file entry for Jonty Sprite format Ver2 JTY/TAB files.
 */
struct JontySpriteV2 {
    /** Offset of the sprite data in DAT file. */
    uint32_t Data;
    /** Width of the sprite data. */
    unsigned short SWidth;
    /** Height of the sprite data. */
    unsigned short SHeight;
    /** Width of the animation frame (same for whole animation). */
    unsigned short FrameWidth;
    /** Height of the animation frame (same for whole animation). */
    unsigned short FrameHeight;
    /** Flags informing whether the animation is rotable; 0 - non-rotable, 2 - rotable. */
    unsigned char Rotable;
    /** Amount of frames making up the animation (same for whole animation). */
    unsigned char FramesCount;
    /** Offset of the sprite within frame; width shift. */
    short FrameOffsW;
    /** Offset of the sprite within frame; height shift. */
    short FrameOffsH;
    /** Unidentified negative value A. */
    signed short unkn6;
    /** Unidentified negative value B. */
    signed short unkn8;
};

#pragma pack()

/**
 * Packs a line of pixels (1 byte per pixel) so that transparent bytes are RLE-encoded into SmallSprite.
 * @return the new number of bytes in row.
 */
int sspr_pack(png_bytep out_row, const png_bytep inp_row, const ColorTranparency::Column& inp_trans, int width, int wskip, const ColorPalette& palette)
{
    int area;
    int outIndex=0;
    int i=0;
    while (i < width)
    {
        // Filled
        area = 0;
        while ( (i+area < width) && (!inp_trans[wskip+i+area]) ) {
            area++;
        }
        LogDbg("fill area %d",area);
        while (area > 0) {
            int part_area;
            if (area > 127) {
                part_area = 127;
                area -= 127;
            } else {
                part_area = area;
                area = 0;
            }
            *(char *)(out_row+outIndex) = (char)(part_area);
            outIndex += sizeof(char);
            memcpy(out_row+outIndex, inp_row+wskip+i, part_area);
            outIndex += part_area;
            i += part_area;
        }
        // Transparent
        area = 0;
        while ( (i+area < width) && inp_trans[wskip+i+area] ) {
            area++;
        }
        LogDbg("trans area %d",area);
        if (i+area >= width) {
            i += area;
            area = 0;
        }
        while (area > 0) {
            int part_area;
            if (area > 127) {
                part_area = 127;
                area -= 127;
            } else {
                part_area = area;
                area = 0;
            }
            *(char *)(out_row+outIndex) = (char)(-part_area);
            outIndex += sizeof(char);
            i += part_area;
        }
    }
    { // End a line with 0
        *(char *)(out_row+outIndex) = 0;
        outIndex += sizeof(char);
    }
    return outIndex;
}

std::string file_name_get_path(const std::string &fname_inp)
{
    size_t tmp1,tmp2;
    tmp1 = fname_inp.find_last_of('/');
    tmp2 = fname_inp.find_last_of('\\');
    if ((tmp1 == std::string::npos) || ((tmp2 != std::string::npos) && (tmp1 < tmp2)))
        tmp1 = tmp2;
    if (tmp1 != std::string::npos)
        return fname_inp.substr(0,tmp1);
    return "";
}

std::string file_name_strip_path(const std::string &fname_inp)
{
    size_t tmp1,tmp2;
    tmp1 = fname_inp.find_last_of('/');
    tmp2 = fname_inp.find_last_of('\\');
    if ((tmp1 == std::string::npos) || ((tmp2 != std::string::npos) && (tmp1 < tmp2)))
        tmp1 = tmp2;
    if (tmp1 != std::string::npos)
        return fname_inp.substr(tmp1+1);
    return fname_inp;
}

std::string file_name_change_extension(const std::string &fname_inp, const std::string &ext)
{
    std::string fname = fname_inp;
    size_t tmp2;
    tmp2 = fname.find_last_of('.');
    if (tmp2 != std::string::npos)
    {
        fname.replace(tmp2+1,fname.length()-tmp2,ext);
    } else
    {
        fname += "." + ext;
    }
    return fname;
}

int load_imagelist(ProgramOptions &opts, const std::string &fname, int anum = -1)
{
    std::ifstream infile;
    std::string lstpath = file_name_get_path(fname);
    infile.open(fname.c_str(), ifstream::in);
    if (infile.fail()) {
        perror(fname.c_str());
        return false;
    }
    int fd[4] = {0,0,0,0};
    {
        // Initial line - animation name and format-specific parameters
        std::string str;
        std::getline(infile, str, '\n');
        istringstream iss(str);
        iss >> str >> fd[0] >> fd[1] >> fd[2] >> fd[3];
    }
    while (infile.good()) {
        std::string str;
        int dm[4] = {-1,-1,-1,-1};
        std::getline(infile, str, '\n');
        istringstream iss(str);
        iss >> str >> dm[0] >> dm[1] >> dm[2] >> dm[3];
        if (!str.empty()) {
            opts.inp.push_back(ImageArea(lstpath+"/"+str,anum,dm[0],dm[1],dm[2],dm[3],fd[0],fd[1],fd[2],fd[3]));
            LogDbg("%s anim=%d dm(%d %d %d %d) fd(%d %d %d %d)\n",str.c_str(),anum,dm[0],dm[1],dm[2],dm[3],fd[0],fd[1],fd[2],fd[3]);
        }
    }
    return true;
}

int load_animlist(ProgramOptions &opts, const std::string &fname)
{
    std::ifstream infile;
    std::string lstpath = file_name_get_path(fname);
    infile.open(fname.c_str(), ifstream::in);
    if (infile.fail()) {
        perror(fname.c_str());
        return false;
    }
    int i = 0;
    while (infile.good()) {
        std::string str;
        std::getline(infile, str, '\n');
        str.erase(str.find_last_not_of(" \n\r\t")+1);
        if (!str.empty()) {
            load_imagelist(opts, lstpath+"/"+str, i);
            i++;
        }
    }
    return (i > 0);
}

int count_img_unused_lines_top(ImageData& img, ProgramOptions& opts, bool hasAlpha)
{
    checkTransparent_t checkTrans=checkTransparent1;
    int bytesPerPixel = (img.colorBPP()+7) >> 3;
    if (!hasAlpha)
    {
        checkTrans=checkTransparent3;
    }

    png_bytep* row_pointers=png_get_rows(img.png_ptr, img.info_ptr);

    for (unsigned y = 0; y < img.height; y++)
    {
        png_bytep row = row_pointers[y];
        png_bytep pixel = row;

        for (unsigned x = 0; x < img.width; x++)
        {
            bool trans = (*checkTrans)(pixel,img);
            if (!trans) {
                return y;
            }
            pixel += bytesPerPixel;
        }
    }
    // If all transparent, return top half of the size
    return img.height/2;
}

int count_img_unused_lines_bottom(ImageData& img, ProgramOptions& opts, bool hasAlpha)
{
    checkTransparent_t checkTrans=checkTransparent1;
    int bytesPerPixel = (img.colorBPP()+7) >> 3;
    if (!hasAlpha)
    {
        checkTrans=checkTransparent3;
    }

    png_bytep* row_pointers=png_get_rows(img.png_ptr, img.info_ptr);

    for (int y = img.height - 1; y >= 0; y--)
    {
        png_bytep row=row_pointers[y];
        png_bytep pixel=row;

        for (unsigned x = 0; x < img.width; x++)
        {
            bool trans = (*checkTrans)(pixel,img);
            if (!trans) {
                return img.height - 1 - y;
            }
            pixel += bytesPerPixel;
        }
    }
    // If all transparent, return bottom half of the size
    return img.height - img.height/2;
}

int count_img_unused_lines_left(ImageData& img, ProgramOptions& opts, bool hasAlpha)
{
    checkTransparent_t checkTrans=checkTransparent1;
    int bytesPerPixel = (img.colorBPP()+7) >> 3;
    if (!hasAlpha)
    {
        checkTrans=checkTransparent3;
    }

    png_bytep* row_pointers=png_get_rows(img.png_ptr, img.info_ptr);

    for (unsigned x = 0; x < img.width; x++)
    {
        for (unsigned y = 0; y < img.height; y++)
        {
            png_bytep row = row_pointers[y];
            png_bytep pixel = row + x*bytesPerPixel;
            bool trans = (*checkTrans)(pixel,img);
            if (!trans) {
                return x;
            }
        }
    }
    // If all transparent, return left half of the size
    return img.width / 2;
}

int count_img_unused_lines_right(ImageData& img, ProgramOptions& opts, bool hasAlpha)
{
    checkTransparent_t checkTrans=checkTransparent1;
    int bytesPerPixel = (img.colorBPP()+7) >> 3;
    if (!hasAlpha)
    {
        checkTrans=checkTransparent3;
    }

    png_bytep* row_pointers=png_get_rows(img.png_ptr, img.info_ptr);

    for (unsigned x = img.width - 1; x >= 0; x--)
    {
        for (unsigned y = 0; y < img.height; y++)
        {
            png_bytep row = row_pointers[y];
            png_bytep pixel = row + x*bytesPerPixel;
            bool trans = (*checkTrans)(pixel,img);
            if (!trans) {
                return img.width - 1 - x;
            }
        }
    }
    // If all transparent, return right half of the size
    return img.width - img.width/2;
}

short load_inp_additional_data(ImageData& img, const ImageArea& inp, ProgramOptions& opts)
{
    struct JontySpriteV1 *jtab1;
    struct JontySpriteV2 *jtab2;
    int ntop,nbottom,nright,nleft;
    if ((inp.x > 0) && (inp.x < (int)img.width)) {
        img.crop_x = inp.x;
    } else {
        img.crop_x = 0;
    }
    if ((inp.y > 0) && (inp.y < (int)img.height)) {
        img.crop_y = inp.y;
    } else {
        img.crop_y = 0;
    }
    if ((inp.w > 0) && (img.crop_x + inp.w < (int)img.width)) {
        img.crop_width = inp.w;
    } else {
        img.crop_width = img.width - img.crop_x;
    }
    if ((inp.h > 0) && (img.crop_y + inp.h < (int)img.height)) {
        img.crop_height = inp.h;
    } else {
        img.crop_height = img.height - img.crop_y;
    }
    switch (opts.fmt)
    {
    case OutFmt_JSPR:
        ntop = count_img_unused_lines_top(img, opts, (img.color_type & PNG_COLOR_MASK_ALPHA) != 0);
        nbottom = count_img_unused_lines_bottom(img, opts, (img.color_type & PNG_COLOR_MASK_ALPHA) != 0);
        nright = count_img_unused_lines_right(img, opts, (img.color_type & PNG_COLOR_MASK_ALPHA) != 0);
        nleft = count_img_unused_lines_left(img, opts, (img.color_type & PNG_COLOR_MASK_ALPHA) != 0);
        jtab1 = (struct JontySpriteV1 *)img.additional_data;
        jtab1->Data = -1; // To be correctly set later
        jtab1->SWidth = img.width-nright-nleft;
        jtab1->SHeight = img.height-ntop-nbottom;
        jtab1->FrameWidth = img.width;
        jtab1->FrameHeight = img.height;
        jtab1->FrameOffsW = nleft;
        jtab1->FrameOffsH = ntop;
        jtab1->Rotable = inp.fd[0];
        jtab1->FramesCount = inp.fd[1];
        jtab1->unkn6 = inp.fd[2];
        jtab1->unkn8 = inp.fd[3];
        break;
    case OutFmt_JSPR2:
        ntop = count_img_unused_lines_top(img, opts, (img.color_type & PNG_COLOR_MASK_ALPHA) != 0);
        nbottom = count_img_unused_lines_bottom(img, opts, (img.color_type & PNG_COLOR_MASK_ALPHA) != 0);
        nright = count_img_unused_lines_right(img, opts, (img.color_type & PNG_COLOR_MASK_ALPHA) != 0);
        nleft = count_img_unused_lines_left(img, opts, (img.color_type & PNG_COLOR_MASK_ALPHA) != 0);
        jtab2 = (struct JontySpriteV2 *)img.additional_data;
        jtab2->Data = -1; // To be correctly set later
        jtab2->SWidth = img.width-nright-nleft;
        jtab2->SHeight = img.height-ntop-nbottom;
        jtab2->FrameWidth = img.width;
        jtab2->FrameHeight = img.height;
        jtab2->FrameOffsW = nleft;
        jtab2->FrameOffsH = ntop;
        jtab2->Rotable = inp.fd[0];
        jtab2->FramesCount = inp.fd[1];
        jtab2->unkn6 = inp.fd[2];
        jtab2->unkn8 = inp.fd[3];
        break;
    case OutFmt_SSPR:
    case OutFmt_SSPR2:
    default:
        break;
    }
    return ERR_OK;
}

int load_command_line_options(ProgramOptions &opts, int argc, char *argv[])
{
    opts.clear();
    while (1)
    {
        static struct option long_options[] = {
            {"verbose", no_argument,       0, 'v'},
            {"batchlist",no_argument,      0, 'b'},
            {"framelist",no_argument,      0, 'm'},
            {"format",  required_argument, 0, 'f'},
            {"diffuse", required_argument, 0, 'd'},
            {"dflevel", required_argument, 0, 'l'},
            {"output",  required_argument, 0, 'o'},
            {"outtab",  required_argument, 0, 't'},
            {"palette", required_argument, 0, 'p'},
            {"range",   required_argument, 0, 'r'},
            {NULL,      0,                 0,'\0'}
        };
        /* getopt_long stores the option index here. */
        int c;
        int option_index = 0;
        c = getopt_long(argc, argv, "vbmf:d:l:o:t:p:r:", long_options, &option_index);
        /* Detect the end of the options. */
        if (c == -1)
            break;
        switch (c)
        {
        case 0:
               /* If this option set a flag, do nothing else now. */
               if (long_options[option_index].flag != 0)
                   break;
               if (optarg) {
                   LogDbg("option %s with arg %s", long_options[option_index].name, optarg);
               } else {
                   LogDbg("option %s with no arg", long_options[option_index].name);
               }
               break;
        case 'v':
            verbose++;
            break;
        case 'b':
            opts.batch = Batch_FILELIST;
            break;
        case 'm':
            opts.batch = Batch_ANIMLIST;
            break;
        case 'f':
            if (ci_string(optarg).compare("HSPR") == 0)
                opts.fmt = OutFmt_HSPR;
            else if (ci_string(optarg).compare("SSPR") == 0)
                opts.fmt = OutFmt_SSPR;
            else if (ci_string(optarg).compare("JSPR") == 0)
                opts.fmt = OutFmt_JSPR;
            else if (ci_string(optarg).compare("SSPR2") == 0)
                opts.fmt = OutFmt_SSPR2;
            else if (ci_string(optarg).compare("JSPR2") == 0)
                opts.fmt = OutFmt_JSPR2;
            else if (ci_string(optarg).compare("RAW") == 0)
                opts.fmt = OutFmt_RAW;
            else if (ci_string(optarg).compare("BMP") == 0)
                opts.fmt = OutFmt_BMP;
            else
                return false;
            break;
        case 'd':
            if (ci_string(optarg).compare("FldStnbrg") == 0)
                opts.alg = DfsAlg_FldStnbrg;
            else if (ci_string(optarg).compare("JrvJdcNnk") == 0)
                opts.alg = DfsAlg_JrvJdcNnk;
            else if (ci_string(optarg).compare("Stucki") == 0)
                opts.alg = DfsAlg_Stucki;
            else if (ci_string(optarg).compare("Burkes") == 0)
                opts.alg = DfsAlg_Burkes;
            else if (ci_string(optarg).compare("Fan") == 0)
                opts.alg = DfsAlg_Fan;
            else if (ci_string(optarg).compare("Sierra3") == 0)
                opts.alg = DfsAlg_Sierra3;
            else if (ci_string(optarg).compare("Sierra2") == 0)
                opts.alg = DfsAlg_Sierra2;
            else if (ci_string(optarg).compare("Sierra24A") == 0)
                opts.alg = DfsAlg_Sierra24A;
            else if (ci_string(optarg).compare("Atkinson") == 0)
                opts.alg = DfsAlg_Atkinson;
            else if (ci_string(optarg).compare("ShiauFan4") == 0)
                opts.alg = DfsAlg_ShiauFan4;
            else if (ci_string(optarg).compare("ShiauFan5") == 0)
                opts.alg = DfsAlg_ShiauFan5;
            else
                return false;
            break;
        case 'l':
            opts.lvl = atol(optarg);
            break;
        case 'o':
            opts.fname_out = optarg;
            break;
        case 't':
            opts.fname_tab = optarg;
            break;
        case 'p':
            opts.fname_pal = optarg;
            break;
        case 'r':
            opts.pal_range = atol(optarg);
            break;
        case '?':
               // unrecognized option
               // getopt_long already printed an error message
        default:
            return false;
        }
    }
    // remaining command line arguments (not options)
    while (optind < argc)
    {
        if (opts.batch != Batch_NONE) {
            // In batch mode, file name is not an image but text file with list
            opts.fname_lst = argv[optind++];
            break;
        }
        opts.inp.push_back(ImageArea(argv[optind++]));
    }
    // Load the files list, if it's provided
    if (!opts.fname_lst.empty())
    {
        int listret;
        if (opts.batch == Batch_ANIMLIST) {
            listret = load_animlist(opts, opts.fname_lst);
        } else {
            listret = load_imagelist(opts, opts.fname_lst);
        }
        if (!listret) {
            LogErr("Couldn't load a list of input images.");
            return false;
        }
    }
    if ((optind < argc) || (opts.inp.empty() && opts.fname_lst.empty()))
    {
        LogErr("Incorrectly specified input file name.");
        return false;
    }
    if ((opts.fmt != OutFmt_SSPR) && (opts.fmt != OutFmt_JSPR) && (opts.fmt != OutFmt_SSPR2) && (opts.fmt != OutFmt_JSPR2) &&
        (opts.fmt != OutFmt_RAW)  && (opts.fmt != OutFmt_BMP) && (opts.inp.size() != 1))
    {
        LogErr("This format supports only one input file name.");
        return false;
    }
    // fill names that were not set by arguments
    if (opts.fname_out.length() < 1)
    {
        switch (opts.fmt)
        {
        case OutFmt_HSPR:
        case OutFmt_SSPR:
        case OutFmt_SSPR2:
            opts.fname_out = file_name_change_extension(file_name_strip_path(opts.inp[0].fname),"dat");
            break;
        case OutFmt_JSPR:
        case OutFmt_JSPR2:
            opts.fname_out = file_name_change_extension(file_name_strip_path(opts.inp[0].fname),"jty");
            break;
        case OutFmt_BMP:
            opts.fname_out = file_name_change_extension(file_name_strip_path(opts.inp[0].fname),"bmp");
            break;
        case OutFmt_RAW:
        default:
            opts.fname_out = file_name_change_extension(file_name_strip_path(opts.inp[0].fname),"raw");
            break;
        }
    }
    if (opts.fname_tab.length() < 1)
    {
        opts.fname_tab = file_name_change_extension(opts.fname_out,"tab");
    }
    if (opts.fname_pal.length() < 1)
    {
        opts.fname_pal = file_name_change_extension(opts.fname_out,"pal");
    }
    return true;
}

short show_head(void)
{
    printf("\n%s (%s) %s\n",PROGRAM_FULL_NAME,PROGRAM_NAME,FILE_VERSION);
    printf("  Created by %s; %s\n",PROGRAM_AUTHORS,LEGAL_COPYRIGHT);
    printf("----------------------------------------\n");
    return ERR_OK;
}

/** Displays information about how to use this tool. */
short show_usage(const std::string &fname)
{
    std::string xname = file_name_strip_path(fname.c_str());
    printf("usage:\n");
    printf("    %s [options] <filename>\n", xname.c_str());
    printf("where <filename> should be the input PNG file, and [options] are:\n");
    printf("    -v,--verbose             Verbose console output mode\n");
    printf("    -d<alg>,--diffuse<alg>   Diffusion algorithm used for bpp conversion\n");
    printf("    -l<num>,--dflevel<num>   Diffusion level, 1..100\n");
    printf("    -f<fmt>,--format<fmt>    Output file format; RAW, HSPR, SSPR, JSPR, SSPR2, JSPR2\n");
    printf("    -p<file>,--palette<file> Input PAL file name\n");
    printf("    -r<num>,--range<num>     Color values range in input PAL file, 1..255\n");
    printf("    -o<file>,--output<file>  Output image file name\n");
    printf("    -t<file>,--outtab<file>  Output tabulation file name\n");
    printf("    -b,--batchlist           Batch, input file is not an image but contains a list of PNGs\n");
    printf("    -m,--framelist           Batch, input file is a list of animations which consist of PNGs\n");
    return ERR_OK;
}

short load_inp_palette_file(WorkingSet& ws, const std::string& fname_pal, ProgramOptions& opts)
{
    std::fstream f;

    /* Load the .pal file: */
    f.open(fname_pal.c_str(), std::ios::in | std::ios::binary);
    if (f.fail()) {
        perror(fname_pal.c_str());
        return ERR_CANT_OPEN;
    }

    while (!f.eof())
    {
        unsigned char col[3];
        unsigned char r,g,b;
        // read next color
        f.read((char *)col, 3);

        if (!f.good())
        {
            break;
        }
        r = (col[0] * 255) / opts.pal_range;
        g = (col[1] * 255) / opts.pal_range;
        b = (col[2] * 255) / opts.pal_range;
        ws.addPaletteQuad((r)|(g<<8)|(b<<16)|(255<<24));
    }

    if (ws.palette.size() != ws.requestedColors())
    {
        LogErr("Incomplete file %s, got %d colors from it while expected %d.", fname_pal.c_str(),(int)ws.palette.size(),ws.requestedColors());
        return ERR_FILE_READ;
    }

    f.close();

    return ERR_OK;
}

short save_raw_file(WorkingSet& ws, std::vector<ImageData>& imgs, const std::string& fname_out, ProgramOptions& opts)
{
    // Open and write the RAW file
    FILE* rawfile;
    {
        rawfile = fopen(fname_out.c_str(),"wb");
        if (rawfile == NULL) {
            perror(fname_out.c_str());
            return ERR_CANT_OPEN;
        }
    }
    if (opts.batch == Batch_FILELIST)
    {
        int tile_num_x, tile_width, tile_height;
        {
            tile_num_x = opts.inp[0].fd[0];
            tile_width = opts.inp[0].fd[2];
            tile_height = opts.inp[0].fd[3];
        }
        for (int i = 0; i < (int)imgs.size(); i += tile_num_x)
        {
            if (i + tile_num_x - 1 >= (int)imgs.size()) {
                LogErr("Amount of images does not allow to completely fill whole line of RAW file");
                break;
            }
            std::vector<png_bytep *> row_pointers;
            row_pointers.resize(tile_num_x);
            // For every row of tile images, get all row pointers
            for (int k = 0; k < tile_num_x; k++)
            {
                ImageData &img = imgs[i+k];
                row_pointers[k] = png_get_rows(img.png_ptr, img.info_ptr);
            }
            // Now, write output lines wchich merge the tiles
            std::vector<png_byte> out_row;
            out_row.resize(tile_num_x*tile_width);
            for (int y=0; y<tile_height; y++)
            {
                for (int k = 0; k < tile_num_x; k++)
                {
                    ImageData &img = imgs[i+k];
                    png_bytep inp_row = row_pointers[k][img.crop_y+y];
                    int newLength = raw_pack(inp_row,img.crop_width,img.colorBPP());
                    memcpy(&out_row.front()+k*tile_width,inp_row,newLength);
                }
                if (fwrite(&out_row.front(),out_row.size(),1,rawfile) != 1)
                { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
            }
        }
    } else
    {
        ImageData & img = imgs[0];
        png_bytep * row_pointers = png_get_rows(img.png_ptr, img.info_ptr);
        for (unsigned y = 0; y < img.height; y++)
        {
            png_bytep row = row_pointers[y];
            int newLength = raw_pack(row,img.width,img.colorBPP());
            if (fwrite(row, newLength, 1, rawfile) != 1)
            { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
            for (int i = 0; i < xorMaskLineLen(img) - newLength; i++)
                writeByte(rawfile,0);
        }
    }
    fclose(rawfile);
    return ERR_OK;
}

short save_bmp_file(WorkingSet& ws, std::vector<ImageData>& imgs, const std::string& fname_out, ProgramOptions& opts)
{
    // Open and write the BMP file
    FILE* bmpfile;
    {
        bmpfile = fopen(fname_out.c_str(),"wb");
        if (bmpfile == NULL) {
            perror(fname_out.c_str());
            return ERR_CANT_OPEN;
        }
    }
    // Write zero-filled header
    {
        std::vector<unsigned char> head;
        head.resize(0x36);
        if (fwrite(&head.front(),head.size(),1,bmpfile) != 1)
        { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
    }
    // Write palette
    {
        unsigned i;
        for (i = 0; i < ws.palette.size(); i++)
        {
            unsigned int cval;
            cval=(unsigned int)ws.palette[i].blue;
            if (cval>255) cval=255;
            fputc(cval, bmpfile);
            cval=(unsigned int)ws.palette[i].green;
            if (cval>255) cval=255;
            fputc(cval, bmpfile);
            cval=(unsigned int)ws.palette[i].red;
            if (cval>255) cval=255;
            fputc(cval, bmpfile);
            fputc(0, bmpfile);
        }
        for (; i < 256; i++)
        {
            fputc(0, bmpfile);
            fputc(0, bmpfile);
            fputc(0, bmpfile);
            fputc(0, bmpfile);
        }

    }
    int full_width, full_height;
    if (opts.batch == Batch_FILELIST)
    {
        int tile_num_x, tile_width, tile_height;
        {
            tile_num_x = opts.inp[0].fd[0];
            tile_width = opts.inp[0].fd[2];
            tile_height = opts.inp[0].fd[3];
        }
        full_width = tile_num_x * tile_width;
        full_height = (imgs.size() / tile_num_x) * tile_height;
        for (int i = 0; i < (int)imgs.size(); i += tile_num_x)
        {
            if (i + tile_num_x - 1 >= (int)imgs.size()) {
                LogErr("Amount of images does not allow to completely fill whole line of RAW file");
                break;
            }
            std::vector<png_bytep *> row_pointers;
            row_pointers.resize(tile_num_x);
            // For every row of tile images, get all row pointers
            for (int k = 0; k < tile_num_x; k++)
            {
                ImageData &img = imgs[i + k];
                row_pointers[k] = png_get_rows(img.png_ptr, img.info_ptr);
            }
            // Now, write output lines which merge the tiles
            std::vector<png_byte> out_row;
            out_row.resize(tile_num_x * tile_width);
            for (int y = 0; y < tile_height; y++)
            {
                for (int k = 0; k < tile_num_x; k++)
                {
                    ImageData &img = imgs[i+k];
                    png_bytep inp_row = row_pointers[k][img.crop_y+y];
                    int newLength = raw_pack(inp_row,img.crop_width,img.colorBPP());
                    memcpy(&out_row.front()+k*tile_width,inp_row,newLength);
                }
                if (fwrite(&out_row.front(),out_row.size(),1,bmpfile) != 1)
                { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
            }
        }
    } else
    {
        ImageData & img = imgs[0];
        png_bytep * row_pointers = png_get_rows(img.png_ptr, img.info_ptr);
        full_width = img.width;
        full_height = img.height;
        for (unsigned y = 0; y < img.height; y++)
        {
            png_bytep row = row_pointers[y];
            int newLength = raw_pack(row,img.width,img.colorBPP());
            if (fwrite(row,newLength,1,bmpfile) != 1)
            { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
            for(int i=0; i<xorMaskLineLen(img)-newLength; ++i) writeByte(bmpfile,0);
        }
    }
    {
        long data_len,pal_len;
        // Length of data
        int padding_size = 4-(full_width&3);
        data_len = (full_width+padding_size)*full_height;
        // Length of palette
        pal_len = 256*4;
        fseek(bmpfile, 0, SEEK_SET);
        fputs("BM",bmpfile);
        write_int32_le_file(bmpfile, data_len+pal_len+0x36);
        write_int32_le_file(bmpfile, 0);
        write_int32_le_file(bmpfile, pal_len+0x36);
        write_int32_le_file(bmpfile, 40);
        write_int32_le_file(bmpfile, full_width);
        write_int32_le_file(bmpfile, -full_height);
        write_int16_le_file(bmpfile, 1);
        write_int16_le_file(bmpfile, 8);
    }
    fclose(bmpfile);
    return ERR_OK;
}

short save_hugspr_file(WorkingSet& ws, ImageData& img, const std::string& fname_out, ProgramOptions& opts)
{
    // Open and write the HugeSprite file
    {
        FILE* rawfile = fopen(fname_out.c_str(),"wb");
        if (rawfile == NULL) {
            perror(fname_out.c_str());
            return ERR_CANT_OPEN;
        }
        long * row_shifts = new long[img.height];
        if (fwrite(row_shifts,img.height*sizeof(long),1,rawfile)!=1) {perror(fname_out.c_str()); return ERR_FILE_WRITE; }
        long base_pos = ftell(rawfile);
        png_bytep out_row = new png_byte[img.width*3];
        png_bytep * row_pointers = png_get_rows(img.png_ptr, img.info_ptr);
        for (unsigned y = 0; y < img.height; y++)
        {
            row_shifts[y] = ftell(rawfile) - base_pos;
            png_bytep inp_row = row_pointers[y];
            ColorTranparency::Column& inp_trans = img.transMap[y];
            int newLength = hspr_pack(out_row,inp_row,inp_trans,img.width,ws.palette);
            if (fwrite(out_row,newLength,1,rawfile)!=1) {perror(fname_out.c_str()); return ERR_FILE_WRITE; }
            //writeByte(rawfile,0);
        }
        delete[] out_row;
        fseek(rawfile,0,SEEK_SET);
        if (fwrite(row_shifts,img.height*sizeof(long),1,rawfile)!=1) {perror(fname_out.c_str()); return ERR_FILE_WRITE; }
        delete[] row_shifts;
        fclose(rawfile);
    }
    return ERR_OK;
}

short save_smallspr_v1_file(WorkingSet& ws, std::vector<ImageData>& imgs, const std::string& fname_out, const std::string& fname_tab, ProgramOptions& opts)
{
    std::vector<SmallSpriteV1> spr_shifts;
    // Open and write the SmallSprite file
    {
        FILE* rawfile = fopen(fname_out.c_str(),"wb");
        if (rawfile == NULL) {
            perror(fname_out.c_str());
            return ERR_CANT_OPEN;
        }
        // Shifts start with index 1; the 0 is empty and unused
        {
            spr_shifts.resize(imgs.size()+1);
            spr_shifts[0].Data = 0;
            spr_shifts[0].SWidth = 0;
            spr_shifts[0].SHeight = 0;
        }
        long base_pos = ftell(rawfile);
        {
            unsigned short spr_count;
            spr_count = imgs.size()+1;
            if (fwrite(&spr_count,sizeof(spr_count),1,rawfile) != 1)
            { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
        }
        for (unsigned i = 0; i < imgs.size(); i++)
        {
            ImageData &img = imgs[i];
            spr_shifts[i+1].Data = ftell(rawfile) - base_pos;
            spr_shifts[i+1].SWidth = img.crop_width;
            spr_shifts[i+1].SHeight = img.crop_height;
            std::vector<png_byte> out_row;
            out_row.resize(img.crop_width*3);
            png_bytep * row_pointers = png_get_rows(img.png_ptr, img.info_ptr);
            for (int y=0; y<img.crop_height; y++)
            {
                png_bytep inp_row = row_pointers[img.crop_y+y];
                ColorTranparency::Column& inp_trans = img.transMap[img.crop_y+y];
                int newLength = sspr_pack(&out_row.front(),inp_row,inp_trans,img.crop_width,img.crop_x,ws.palette);
                if (fwrite(&out_row.front(),newLength,1,rawfile) != 1)
                { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
            }
            {
                // End an image with (-128) - it's available in u2 encoding, and only values -127..127
                //are used for defining size of data and transparency, so this special value wasn't used before
                char endofimg = -128;
                if (fwrite(&endofimg,sizeof(char),1,rawfile) != 1)
                { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
            }
        }
        fclose(rawfile);
    }
    // Open and write the TAB file
    {
        FILE* tabfile = fopen(fname_tab.c_str(),"wb");
        if (tabfile == NULL) {
            perror(fname_tab.c_str());
            return ERR_CANT_OPEN;
        }
        if (fwrite(&spr_shifts.front(),sizeof(SmallSpriteV1),spr_shifts.size(),tabfile) != spr_shifts.size())
        { perror(fname_tab.c_str()); return ERR_FILE_WRITE; }
        fclose(tabfile);
    }
    return ERR_OK;
}

short save_smallspr_v2_file(WorkingSet& ws, std::vector<ImageData>& imgs, const std::string& fname_out, const std::string& fname_tab, ProgramOptions& opts)
{
    std::vector<SmallSpriteV2> spr_shifts;
    // Open and write the SmallSprite file
    {
        FILE* rawfile = fopen(fname_out.c_str(),"wb");
        if (rawfile == NULL) {
            perror(fname_out.c_str());
            return ERR_CANT_OPEN;
        }
        // Shifts start with index 1; the 0 is empty and unused
        {
            spr_shifts.resize(imgs.size()+1);
            spr_shifts[0].Data = 0;
            spr_shifts[0].SWidth = 0;
            spr_shifts[0].SHeight = 0;
        }
        long base_pos = ftell(rawfile);
        {
            unsigned short spr_count;
            spr_count = imgs.size()+1;
            if (fwrite(&spr_count,sizeof(spr_count),1,rawfile) != 1)
            { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
        }
        for (unsigned i = 0; i < imgs.size(); i++)
        {
            ImageData &img = imgs[i];
            spr_shifts[i+1].Data = ftell(rawfile) - base_pos;
            spr_shifts[i+1].SWidth = img.crop_width;
            spr_shifts[i+1].SHeight = img.crop_height;
            std::vector<png_byte> out_row;
            out_row.resize(img.crop_width*3);
            png_bytep * row_pointers = png_get_rows(img.png_ptr, img.info_ptr);
            for (int y=0; y<img.crop_height; y++)
            {
                png_bytep inp_row = row_pointers[img.crop_y+y];
                ColorTranparency::Column& inp_trans = img.transMap[img.crop_y+y];
                int newLength = sspr_pack(&out_row.front(),inp_row,inp_trans,img.crop_width,img.crop_x,ws.palette);
                if (fwrite(&out_row.front(),newLength,1,rawfile) != 1)
                { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
            }
            {
                // End an image with (-128) - it's available in u2 encoding, and only values -127..127
                //are used for defining size of data and transparency, so this special value wasn't used before
                char endofimg = -128;
                if (fwrite(&endofimg,sizeof(char),1,rawfile) != 1)
                { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
            }
        }
        fclose(rawfile);
    }
    // Open and write the TAB file
    {
        FILE* tabfile = fopen(fname_tab.c_str(),"wb");
        if (tabfile == NULL) {
            perror(fname_tab.c_str());
            return ERR_CANT_OPEN;
        }
        if (fwrite(&spr_shifts.front(),sizeof(SmallSpriteV2),spr_shifts.size(),tabfile) != spr_shifts.size())
        { perror(fname_tab.c_str()); return ERR_FILE_WRITE; }
        fclose(tabfile);
    }
    return ERR_OK;
}

short save_jontyspr_v1_file(WorkingSet& ws, std::vector<ImageData>& imgs, const std::string& fname_out, const std::string& fname_tab, ProgramOptions& opts)
{
    std::vector<JontySpriteV1> spr_shifts;
    // Open and write the JontySprite file
    {
        FILE* rawfile = fopen(fname_out.c_str(),"wb");
        if (rawfile == NULL) {
            perror(fname_out.c_str());
            return ERR_CANT_OPEN;
        }
        // Shifts start with index 0, and there's additional entry at end
        spr_shifts.resize(imgs.size()+1);
        long base_pos = ftell(rawfile);
        for (unsigned i = 0; i < imgs.size(); i++)
        {
            ImageData &img = imgs[i];
            JontySpriteV1 &spr = spr_shifts[i];
            memcpy(&spr, &img.additional_data, sizeof(JontySpriteV1));
            spr.Data = ftell(rawfile) - base_pos;
            std::vector<png_byte> out_row;
            out_row.resize(spr.SWidth*3);
            png_bytep * row_pointers = png_get_rows(img.png_ptr, img.info_ptr);
            for (int y=0; y<spr.SHeight; y++)
            {
                png_bytep inp_row = row_pointers[spr.FrameOffsH+y];
                ColorTranparency::Column& inp_trans = img.transMap[spr.FrameOffsH+y];
                int newLength = sspr_pack(&out_row.front(),inp_row,inp_trans,spr.SWidth,spr.FrameOffsW,ws.palette);
                if (fwrite(&out_row.front(),newLength,1,rawfile) != 1)
                { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
            }
            {
                // End an image with (-128) - it's available in u2 encoding, and only values -127..127
                //are used for defining size of data and transparency, so this special value wasn't used before
                char endofimg = -128;
                if (fwrite(&endofimg,sizeof(char),1,rawfile) != 1)
                { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
            }
        }
        // Add the entry at end
        {
            int i = imgs.size();
            memset(&spr_shifts[i], 0, sizeof(JontySpriteV1));
            spr_shifts[i].Data = ftell(rawfile) - base_pos;
            spr_shifts[i].SWidth = 0;
            spr_shifts[i].SHeight = 0;
        }
        fclose(rawfile);
    }
    // Open and write the TAB file
    {
        FILE* tabfile = fopen(fname_tab.c_str(),"wb");
        if (tabfile == NULL) {
            perror(fname_tab.c_str());
            return ERR_CANT_OPEN;
        }
        if (fwrite(&spr_shifts.front(),sizeof(JontySpriteV1),spr_shifts.size(),tabfile) != spr_shifts.size())
        { perror(fname_tab.c_str()); return ERR_FILE_WRITE; }
        fclose(tabfile);
    }
    return ERR_OK;
}

short save_jontyspr_v2_file(WorkingSet& ws, std::vector<ImageData>& imgs, const std::string& fname_out, const std::string& fname_tab, ProgramOptions& opts)
{
    std::vector<JontySpriteV2> spr_shifts;
    // Open and write the JontySprite file
    {
        FILE* rawfile = fopen(fname_out.c_str(),"wb");
        if (rawfile == NULL) {
            perror(fname_out.c_str());
            return ERR_CANT_OPEN;
        }
        // Shifts start with index 0, and there's additional entry at end
        spr_shifts.resize(imgs.size()+1);
        long base_pos = ftell(rawfile);
        for (unsigned i = 0; i < imgs.size(); i++)
        {
            ImageData &img = imgs[i];
            JontySpriteV2 &spr = spr_shifts[i];
            memcpy(&spr, &img.additional_data, sizeof(JontySpriteV2));
            spr.Data = ftell(rawfile) - base_pos;
            std::vector<png_byte> out_row;
            out_row.resize(spr.SWidth*3);
            png_bytep * row_pointers = png_get_rows(img.png_ptr, img.info_ptr);
            for (int y=0; y<spr.SHeight; y++)
            {
                png_bytep inp_row = row_pointers[spr.FrameOffsH+y];
                ColorTranparency::Column& inp_trans = img.transMap[spr.FrameOffsH+y];
                int newLength = sspr_pack(&out_row.front(),inp_row,inp_trans,spr.SWidth,spr.FrameOffsW,ws.palette);
                if (fwrite(&out_row.front(),newLength,1,rawfile) != 1)
                { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
            }
            {
                // End an image with (-128) - it's available in u2 encoding, and only values -127..127
                //are used for defining size of data and transparency, so this special value wasn't used before
                char endofimg = -128;
                if (fwrite(&endofimg,sizeof(char),1,rawfile) != 1)
                { perror(fname_out.c_str()); return ERR_FILE_WRITE; }
            }
        }
        // Add the entry at end
        {
            int i = imgs.size();
            memset(&spr_shifts[i], 0, sizeof(JontySpriteV2));
            spr_shifts[i].Data = ftell(rawfile) - base_pos;
            spr_shifts[i].SWidth = 0;
            spr_shifts[i].SHeight = 0;
        }
        fclose(rawfile);
    }
    // Open and write the TAB file
    {
        FILE* tabfile = fopen(fname_tab.c_str(),"wb");
        if (tabfile == NULL) {
            perror(fname_tab.c_str());
            return ERR_CANT_OPEN;
        }
        if (fwrite(&spr_shifts.front(),sizeof(JontySpriteV2),spr_shifts.size(),tabfile) != spr_shifts.size())
        { perror(fname_tab.c_str()); return ERR_FILE_WRITE; }
        fclose(tabfile);
    }
    return ERR_OK;
}

int main(int argc, char* argv[])
{
    static ProgramOptions opts;
    if (!load_command_line_options(opts, argc, argv))
    {
        show_head();
        show_usage(argv[0]);
        return 11;
    }
    // start the work
    if (verbose)
        show_head();

    static WorkingSet ws;

    std::vector<ImageData> imgs;
    imgs.resize(opts.inp.size());
    {
        for (unsigned i = 0; i < opts.inp.size(); i++)
        {
            if (verbose)
                LogMsg("Loading image \"%s\".",opts.inp[i].fname.c_str());
            ImageData& img = imgs[i];
            if (load_inp_png_file(img, opts.inp[i].fname, opts) != ERR_OK) {
                return 2;
            }
            if (load_inp_additional_data(img, opts.inp[i], opts) != ERR_OK) {
                return 2;
            }
        }
    }

    ws.alg = opts.alg;
    ws.ditherLevel(opts.lvl);
    ws.requestedColors(256);
    if (verbose)
        LogMsg("Loading palette file \"%s\".",opts.fname_pal.c_str());
    if (load_inp_palette_file(ws, opts.fname_pal, opts) != ERR_OK) {
        LogErr("Loading palette failed.");
        return 4;
    }

    {
        std::vector<ImageData>::iterator iter;
        for (iter = imgs.begin(); iter != imgs.end(); iter++)
        {
            if (verbose)
                LogMsg("Converting image %d colors to indexes...",(int)(iter-imgs.begin()));
            ImageData& img = *iter;
            if (convert_rgb_to_indexed(ws, img, (img.color_type & PNG_COLOR_MASK_ALPHA) != 0) != ERR_OK) {
                LogErr("Converting colors failed.");
                return 6;
            }
        }
    }

    switch (opts.fmt)
    {
    case OutFmt_RAW:
        LogMsg("Saving RAW file \"%s\".",opts.fname_out.c_str());
        if (save_raw_file(ws, imgs, opts.fname_out, opts) != ERR_OK) {
            return 8;
        }
        break;
    case OutFmt_BMP:
        LogMsg("Saving BMP file \"%s\".",opts.fname_out.c_str());
        if (save_bmp_file(ws, imgs, opts.fname_out, opts) != ERR_OK) {
            return 8;
        }
        break;
    case OutFmt_HSPR:
        LogMsg("Saving HSPR file \"%s\".",opts.fname_out.c_str());
        if (save_hugspr_file(ws, imgs[0], opts.fname_out, opts) != ERR_OK) {
            return 8;
        }
        break;
    case OutFmt_SSPR:
        LogMsg("Saving SSPR1 file \"%s\".",opts.fname_out.c_str());
        if (save_smallspr_v1_file(ws, imgs, opts.fname_out, opts.fname_tab, opts) != ERR_OK) {
            return 8;
        }
        break;
    case OutFmt_SSPR2:
        LogMsg("Saving SSPR2 file \"%s\".",opts.fname_out.c_str());
        if (save_smallspr_v2_file(ws, imgs, opts.fname_out, opts.fname_tab, opts) != ERR_OK) {
            return 8;
        }
        break;
    case OutFmt_JSPR:
        LogMsg("Saving JSPR1 file \"%s\".",opts.fname_out.c_str());
        if (save_jontyspr_v1_file(ws, imgs, opts.fname_out, opts.fname_tab, opts) != ERR_OK) {
            return 8;
        }
        break;
    case OutFmt_JSPR2:
        LogMsg("Saving JSPR2 file \"%s\".",opts.fname_out.c_str());
        if (save_jontyspr_v2_file(ws, imgs, opts.fname_out, opts.fname_tab, opts) != ERR_OK) {
            return 8;
        }
        break;
    }

    return 0;
}


