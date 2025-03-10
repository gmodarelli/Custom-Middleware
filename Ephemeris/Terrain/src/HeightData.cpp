/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Ephemeris.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#include "HeightData.h"

#include <exception>

#include "../../../../The-Forge/Common_3/Utilities/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/Utilities/Interfaces/ILog.h"

// Creates data source from the specified raw data file
HeightData::HeightData(const char* fileName):
    colCount(0), rowCount(0), colOffset(1356), rowOffset(924), levels(0), patchSize(128), data(nullptr)
{
    // open file
    FileStream modelFile0FH = {};

    if (!fsOpenStreamFromPath(RD_TEXTURES, fileName, FM_READ, &modelFile0FH))
    {
        LOGF(LogLevel::eERROR, "Image File not found: %s\n", fileName);
        return;
    }

    // load file into memory
    uint length = (uint)fsGetStreamFileSize(&modelFile0FH);
    if (length == 0)
    {
        LOGF(LogLevel::eERROR, "Image File not found: %s\n", fileName);
        fsCloseStream(&modelFile0FH);
        return;
    }

    // read and close file.
    char* heightMap = (char*)tf_malloc(length);

    fsReadFromStream(&modelFile0FH, heightMap, fsGetStreamFileSize(&modelFile0FH));
    fsCloseStream(&modelFile0FH);

    uint32_t width = (uint32_t)sqrtf((float)length / 4.0f);
    uint32_t height = width;

    // Calculate minimal number of columns and rows
    // in the form 2^n+1 that encompass the data
    colCount = 1;
    rowCount = 1;
    while (colCount + 1 < width || rowCount + 1 < height)
    {
        colCount *= 2;
        rowCount *= 2;
    }

    levels = 1;
    while ((patchSize << (levels - 1)) < (int)colCount || (patchSize << (levels - 1)) < (int)rowCount)
        levels++;

    colCount++;
    rowCount++;

    // Load the data
    data = (float*)tf_malloc(4 * colCount * rowCount);

    for (uint32_t row = 0; row < height; ++row)
    {
        memcpy(data + colCount * row, heightMap + row * width * 4, width * 4);
    }

    // Duplicate the last row and column
    for (uint32_t r = 0; r < height; r++)
        for (uint32_t c = width; c < colCount; c++)
            data[c + r * colCount] = data[(width - 1) + r * colCount];
    for (uint32_t c = 0; c < colCount; c++)
        for (uint32_t r = height; r < rowCount; r++)
            data[c + r * colCount] = data[c + (height - 1) * colCount];

    tf_free(heightMap);
}

HeightData::~HeightData(void) { tf_free(data); }

int mirrorCoord(int coord, int dim)
{
    coord = abs(coord);
    int period = coord / dim;
    coord = coord % dim;
    if (period & 0x01)
    {
        coord = (dim - 1) - coord;
    }
    return coord;
}

float HeightData::getInterpolatedHeight(float col, float row, int step) const
{
    int    row0 = ((int)floorf(row) / step) * step;
    int    col0 = ((int)floorf(col) / step) * step;
    float2 weights = float2((col - (float)col0) / (float)step, (row - (float)row0) / (float)step);
    col0 += colOffset;
    row0 += rowOffset;

    int col1 = col0 + step;
    int row1 = row0 + step;

    col0 = mirrorCoord(col0, colCount);
    col1 = mirrorCoord(col1, colCount);
    row0 = mirrorCoord(row0, rowCount);
    row1 = mirrorCoord(row1, rowCount);

    float H00 = data[col0 + row0 * colCount];
    float H10 = data[col1 + row0 * colCount];
    float H01 = data[col0 + row1 * colCount];
    float H11 = data[col1 + row1 * colCount];
    // float interpolatedHeight = (H00 * (1 - weights.mX) + H10 * weights.mX) * (1 - weights.mY) +    (H01 * (1 - weights.mX) + H11 *
    // weights.mX) * weights.mY;
    float interpolatedHeight =
        (H00 * (1 - weights.x) + H10 * weights.x) * (1 - weights.y) + (H01 * (1 - weights.x) + H11 * weights.x) * weights.y;
    return interpolatedHeight;
}
