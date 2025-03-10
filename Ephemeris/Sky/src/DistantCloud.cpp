/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Ephemeris.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#include "DistantCloud.h"

#include "../../../../The-Forge/Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h"
#include "../../../../The-Forge/Common_3/Utilities/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/Utilities/Interfaces/ILog.h"

#include "../../../../The-Forge/Common_3/Utilities/Interfaces/IMemory.h"

DistantCloud::DistantCloud(const mat4& Transform, Texture* tex): m_Texture(tex), m_Transform(Transform) {}

void DistantCloud::moveCloud(const vec3& direction)
{
    float temp = m_Transform.getRow(0).getW();
    temp += direction.getX();
    temp += direction.getY();
    temp += direction.getZ();

    // vec4 &row0 = m_Transform.getRow(0).setW();
    m_Transform[3][0] += temp;
}
