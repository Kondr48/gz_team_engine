#ifndef VSD_MOUNT
#define VSD_MOUNT
#include "common.h"
#include "noise_2D.h"
#include "ShaderSettingsVisor.txt"

float2 l_jh2 (float2 f, float4 s, float l)
{ 
     float2 x = s.xy, V = s.zw;
	 float y = snoise_2D(f * float2(VSD_TURBSIZE, VSD_TURBSIZE)) * .5;
	 float4 r = float4(y, y, y, 1);
	 r.xy = float2(r.x + r.z / 4, r.y + r.x / 2);
	 r -= .5;
	 r *= l;
	 return saturate(f + (x + V) * r.xy);
}

float3 bt9_q(float2 f, float2 y, float s, float3 V, float3 l, float3 r)
{
	float3 x = V;
	float p = s / 2.;
	float2 a = (f.xy - y)/(s / 2);
	float i = sqrt(p * p - a.x * a.x - a.y * a.y);
	i *= p;
	float3 E = normalize(float3(a.x, a.y, i));
	float e = clamp(pow(max(0., dot(E, normalize(l))), 2), .4, .95), z= clamp(pow(max(0., dot(E, normalize(r))), 2), .4, .95);
	e = abs(e - .68) < abs(z-.68) ? e : z;
	float3 S = float3(e, e, e);
	V = lerp(V, V * S * 1.35, .5);
	return lerp(x, V, VSD_LIGHT);
}

float2 z6g(float2 s, float2 f, float y)
{
	float2 l = f - s,V = l;
	V.y /= VSD_ASPECT;
	float x = length(V), S = saturate(1 - x / (abs(-sin(VSD_RADIUS * y * 8) * VSD_RADIUS * y) + 1e-08F));
	float2 z = s + l * S * VSD_STRENGTH;
	return z;
}

float3 ud__(float4 f, float4 s, float2 y, float2 l, float2 V, float2 S, float3 e, float3 r)
{
	float2 x = z6g(l, V, S);
	if(length(x - l)>0)
	{
		float3 E = s.xyz - f.xyz;
		float z = VSD_RADIUS * S * .601, a = smoothstep(0, 1, saturate(length(x - V) / z));
		float3 p = tex2Dlod(s_image, float4(x, 0, 0)).xyz;
		return!(f.x! = p.x||f.y! = p.y||f.z! = p.z) ? s : lerp(s, bt9_q(x, V - (x - y), S, p + E, e, r), lerp(1, a, VSD_SMOOTH) * VSD_MIX);
	}
	else return s;
}

float2 wavewarp(float2 f)
{
	return wavewarp(f, VSD_WSIZE, VSD_WTIME, VSD_WVAL1, VSD_WVAL2, VSD_WVAL3, VSD_WVAL4);
}

float ff35(float f)
{
	return frac(sin(f) * 43758.6);
}

float4 visor_drops(float2 f, float4 s, float4 V, float3 y, float3 x, float l)
{
	float4 z = s;
	f = lerp(f, l_jh2 (f, VSD_TURBSHIFT, VSD_TURBTIME), VSD_TURBCOF);
	s = tex2Dlod(s_image, float4(f, 0, 0));
	float2 e = f, a = 0;
	float r = 0;
	float2 p = 0, S = float2(48, 36), i = 1./S;
	float E = 0;
	1.25;
	float2 m = f;
	bool T = floor(m.y / i.y)%2. >= 1.;
	
	if (T)
		m.x += E*i.x / 2.;
	
	float2 w = floor(m * S) / S + i * .5f, t = wavewarp(w);
	float F = pow(length(t - float2(.5,.5)), 3) * 10, n = (.5 + length(t - w) * 20 * F) * 1.2f;
	
	if (abs(t.x - w.x) * n > .00475 || abs(t.y - w.y) * n > .00361)
		n = .5f;
	
	float2 h = t;
	h.x -= .5;
	
	float H = h.y * h.x + h.x , d = ff35(H);
	
	if(d >= 1-l)
		V.xyz = ud__(s, V, f, e, t, n, y, VSD_FIXLIGHT);
	
	return V;
}
#endif