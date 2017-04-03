#ifndef Blur_H
#define Blur_H

float3 BlurPass(float2 texcoord, float3 input, float factor, int samples) 
{
	float2 bias = float2(.5f/1024.f,.5f/768.f);
	float2 scale = bias * factor;
	
	float2 o[48];
	o[0] = float2(-0.326212f, -0.405810f)*scale;
	o[1] = float2(-0.840144f, -0.073580f)*scale;
	o[2] = float2(-0.695914f, 0.457137f)*scale;
	o[3] = float2(-0.203345f, 0.620716f)*scale;
	o[4] = float2(0.962340f, -0.194983f)*scale;
	o[5] = float2(0.473434f, -0.480026f)*scale;
	o[6] = float2(0.519456f, 0.767022f)*scale;
	o[7] = float2(0.185461f, -0.893124f)*scale;
	o[8] = float2(0.507431f, 0.064425f)*scale;
	o[9] = float2(0.896420f, 0.412458f)*scale;
	o[10] = float2(-0.321940f, -0.932615f)*scale;
	o[11] = float2(-0.791559f, -0.597710f)*scale;
	
	float contrib = 1.h;
	int j = 0;
	
	for (int i = 0; i < samples; i++)
	{
		float2 tap = texcoord + o[j];
		float3 tap_color = tex2Dlod(s_image, float4(tap, 0, 0)).rgb;
		input += tap_color;
		contrib++;
		j++;
		if (j > 11)
			j = 0;
	}
	
	return input;
}

#endif // Blur_H