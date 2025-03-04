/*
* Copyright (c) 2017-2024 The Forge Interactive Inc.
*
* This is a part of Aura.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#ifndef	LPV_SH_MATHS
#define LPV_SH_MATHS

#define PI      3.14159265358979323846f
#define SQRT_PI 1.7724538509055160272974217986853f
#define SQRT_3	1.7320508075688772935274463415059f

#ifdef NO_FSL_DEFINITIONS
//	Correct ones?
static const float  SHBand1 = SQRT_3 / (2.0f * SQRT_PI);
#define SHBasis aura::float4(1.0f / (2.0f * SQRT_PI), -SHBand1, SHBand1, -SHBand1)
static const float  SHProjectionScale = SQRT_PI;
#endif

#ifndef NO_FSL_DEFINITIONS
STATIC const float  SHBand1 = SQRT_3 / (2.0f * SQRT_PI);
STATIC const float4 SHBasis = float4(1.0f / (2.0f * SQRT_PI), -SHBand1, SHBand1, -SHBand1);
STATIC const float  SHProjectionScale = SQRT_PI;

//	Igor: need this to preserve precision in 16 bit FP buffer
//	Works only for normalized sint RT
STATIC const float storeOccluderScaler   = 500.0f;
STATIC const float restoreOccluderScaler = 1.0f / storeOccluderScaler;

#define SHCoeffs float4

STRUCT(SHSpectralCoeffs)
{
	DATA(SHCoeffs, c[3], None); // SV_Target
};

// Set to register value to avoid conflicts when using shaders compiled for permutations
RES(Tex3D(float4), LPVGrid[3],             UPDATE_FREQ_NONE, t100, binding = 80);
RES(Tex3D(float4), LPVGridCascades[3 * 3], UPDATE_FREQ_NONE, t103, binding = 83);
RES(Tex3D(float4), GridOccluder,           UPDATE_FREQ_NONE, t112, binding = 92);
RES(Tex3D(float4), GridOccluderSecondary,  UPDATE_FREQ_NONE, t113, binding = 93);

#ifdef VULKAN

#define SHSampleGrid(s, gridPos, offset) \
	SHSpectralCoeffs(SHCoeffs[3]( \
		SampleLvlOffsetTex3D(Get(LPVGrid)[0], s, gridPos, 0, offset), \
		SampleLvlOffsetTex3D(Get(LPVGrid)[1], s, gridPos, 0, offset), \
		SampleLvlOffsetTex3D(Get(LPVGrid)[2], s, gridPos, 0, offset)) \
	)

#define SHSampleGridForCascade(cascadeIndex, s, gridPos, offset) \
	SHSpectralCoeffs(SHCoeffs[3]( \
		SampleLvlOffsetTex3D(Get(LPVGridCascades)[3 * (cascadeIndex)], s, gridPos, 0, offset),     \
		SampleLvlOffsetTex3D(Get(LPVGridCascades)[3 * (cascadeIndex) + 1], s, gridPos, 0, offset), \
		SampleLvlOffsetTex3D(Get(LPVGridCascades)[3 * (cascadeIndex) + 2], s, gridPos, 0, offset)) \
	)

#define SHSampleOccluder(s, gridPos, offset) SampleLvlOffsetTex3D(Get(GridOccluder), s, gridPos, 0, offset) * restoreOccluderScaler

#define SHSampleOccluderSecondary(s, gridPos, offset) SampleLvlOffsetTex3D(Get(GridOccluderSecondary), s, gridPos, 0, offset) * restoreOccluderScaler

#else

SHSpectralCoeffs SHSampleGrid(SamplerState s, const float3 gridPos, const int3 offset)
{
	SHSpectralCoeffs res;
	res.c[0] = SampleLvlOffsetTex3D(Get(LPVGrid)[0], s, gridPos, 0, offset);
	res.c[1] = SampleLvlOffsetTex3D(Get(LPVGrid)[1], s, gridPos, 0, offset);
	res.c[2] = SampleLvlOffsetTex3D(Get(LPVGrid)[2], s, gridPos, 0, offset);

	return res;
}

SHSpectralCoeffs SHSampleGridForCascade(const uint cascadeIndex, SamplerState s, const float3 gridPos, const int3 offset)
{
	SHSpectralCoeffs res;
	res.c[0] = SampleLvlOffsetTex3D(Get(LPVGridCascades)[3 * cascadeIndex], s, gridPos, 0, offset);
	res.c[1] = SampleLvlOffsetTex3D(Get(LPVGridCascades)[3 * cascadeIndex + 1], s, gridPos, 0, offset);
	res.c[2] = SampleLvlOffsetTex3D(Get(LPVGridCascades)[3 * cascadeIndex + 2], s, gridPos, 0, offset);

	return res;
}

SHCoeffs SHSampleOccluder(SamplerState s, const float3 gridPos, const int3 offset)
{
	return SampleLvlOffsetTex3D(Get(GridOccluder), s, gridPos, 0, offset) * restoreOccluderScaler;
}

SHCoeffs SHSampleOccluderSecondary(SamplerState s, const float3 gridPos, const int3 offset)
{
	return SampleLvlOffsetTex3D(Get(GridOccluderSecondary), s, gridPos, 0, offset) * restoreOccluderScaler;
}

#endif

float3 SHDot(const SHSpectralCoeffs spectralCoeffs, const SHCoeffs coeffs)
{
	return float3(dot(spectralCoeffs.c[0], coeffs), dot(spectralCoeffs.c[1], coeffs), dot(spectralCoeffs.c[2], coeffs));
}

SHSpectralCoeffs SHAdd(const SHSpectralCoeffs a, const SHSpectralCoeffs b)
{
	SHSpectralCoeffs res;

	UNROLL
	for (int i = 0; i < 3; ++i)
		res.c[i] = a.c[i] + b.c[i];

	return res;
}

SHSpectralCoeffs SHScale(const SHCoeffs coeffs, const float3 scale)
{
	SHSpectralCoeffs res;

	UNROLL
	for (int i = 0; i < 3; ++i)
		res.c[i] = coeffs * scale[i];

	return res;
}

SHSpectralCoeffs SHLerp(const SHSpectralCoeffs a, const SHSpectralCoeffs b, const float param)
{
	SHSpectralCoeffs res;

	UNROLL
	for (int i = 0; i < 3; ++i)
		res.c[i] = lerp(a.c[i], b.c[i], param);

	return res;
}

float4 SHRotate(float3 vcDir, const float2 vZHCoeffs)
{
	//	Igor: added this to fix singularity problem.
	if (1.0f - abs(vcDir.z) < 0.001f)
		vcDir.xy = float2(0.0f, 1.0f);

	// compute sine and cosine of theta angle 
	// beware of singularity when both x and y are 0 (no need to rotate at all) 
	float2 theta12_cs = normalize(vcDir.xy);

	// compute sine and cosine of phi angle 
	float2 phi12_cs;
	phi12_cs.x = sqrt(1.0f - vcDir.z * vcDir.z);
	phi12_cs.y = vcDir.z;

	float4 vResult;
	// The first band is rotation-independent 
	vResult.x = vZHCoeffs.x;
	// rotating the second band of SH 
	vResult.y = -vZHCoeffs.y * phi12_cs.x * theta12_cs.y;
	vResult.z =  vZHCoeffs.y * phi12_cs.y;
	vResult.w = -vZHCoeffs.y * phi12_cs.x * theta12_cs.x;

	return vResult;
}

float4 SHProjectCone_angle(const float3 vcDir, const float angle)
{
	const float2 vZHCoeffs = SHProjectionScale * float2(0.5f * (1.0f - cos(angle)), 0.75f * sin(angle) * sin(angle));

	return SHRotate(vcDir, vZHCoeffs);
}

float4 SHProjectConeCos(const float3 vcDir, const float angleCos)
{
	const float2 vZHCoeffs = SHProjectionScale * float2(0.5f * (1.0f - angleCos), 0.75f * (1.0f - angleCos * angleCos));

	return SHRotate(vcDir, vZHCoeffs);
}

//	Cosine lobe
float4 SHProjectCone(const float3 vcDir)
{
	const float2 vZHCoeffs = SHProjectionScale * float2(0.25f, 0.5f);

	return SHRotate(vcDir, vZHCoeffs);
}

SHCoeffs Cone90Degree(const float3 vcDir)
{
	return SHProjectCone_angle(vcDir, float(PI / 4.0f));
}

//	Calculate data
float SHEvaluateFunction_float4(const float3 vcDir, const float4 data)
{
	return dot(data, float4(1.0f, vcDir.yzx) * SHBasis);
}

float3 SHEvaluateFunction(const float3 vcDir, const SHSpectralCoeffs data)
{
	float x = dot(data.c[0], float4(1.0f, vcDir.yzx) * SHBasis);
	float y = dot(data.c[1], float4(1.0f, vcDir.yzx) * SHBasis);
	float z = dot(data.c[2], float4(1.0f, vcDir.yzx) * SHBasis);

	return float3(x, y, z);
}

float3 SHEvaluateMax(const SHSpectralCoeffs data)
{
	float x = dot(data.c[0], float4(1.0f, normalize(data.c[0].yzw)) * abs(SHBasis));
	float y = dot(data.c[1], float4(1.0f, normalize(data.c[1].yzw)) * abs(SHBasis));
	float z = dot(data.c[2], float4(1.0f, normalize(data.c[2].yzw)) * abs(SHBasis));

	return float3(x, y, z);
}

#endif // NO_FSL_DEFINITIONS

#endif // LPV_SH_MATHS
