uniform sampler2D	g_nTexture;
uniform vec2		g_nSize;
uniform ivec2		g_nMin;
uniform ivec2		g_nMax;
uniform ivec2		g_nClamp;

float and(int a, int b)
{
	int m;
	int r = 0;
	int ha, hb;
	
	m = int(min(float(a), float(b)));
	
	for(int k = 1; k <= m; k *= 2)
	{
		ha = a / 2; 
		hb = b / 2;
		if(((a - ha * 2) != 0) && ((b - hb * 2) != 0))
		{
			r += k;
		}
		a = ha;
		b = hb;
	}
	
	return float(r);
}

float or(int a, int b)
{
	int m;
	int r = 0;
	int ha, hb;
	
	m = int(max(float(a), float(b)));
	
	for(int k = 1; k <= m; k *= 2)
	{
		ha = a / 2; 
		hb = b / 2;
		if(((a - ha * 2) != 0) || ((b - hb * 2) != 0))
		{
			r += k;
		}
		a = ha;
		b = hb;
	}
	
	return float(r);
}

float Clamp(int nClampMode, float nCoord, int nMin, int nMax)
{
	if(nClampMode == 1)
	{
		//Region Clamp
		nCoord = max(float(nMin), nCoord);
		nCoord = min(float(nMax), nCoord);
	}
	else if(nClampMode == 2)
	{
		//Repeat Clamp
		nCoord = or(int(and(int(nCoord), nMin)), nMax);
	}
	else if(nClampMode == 3)
	{
		//Repeat Clamp (more elegant)
		nCoord = mod(nCoord, float(nMin)) + float(nMax);
	}
		
	return nCoord;
}

void main()
{
	vec4 nTexCoord;

	nTexCoord = gl_TexCoord[0];
	
	nTexCoord.st *= g_nSize.st;

	nTexCoord.s = Clamp(g_nClamp.s, nTexCoord.s, g_nMin.s, g_nMax.s);
	nTexCoord.t = Clamp(g_nClamp.t, nTexCoord.t, g_nMin.t, g_nMax.t);	

	nTexCoord.st /= g_nSize.st;

	gl_FragColor = texture2D(g_nTexture, nTexCoord.st);
}
