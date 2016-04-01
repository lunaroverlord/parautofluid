#pragma OPENCL EXTENSION cl_amd_printf : enable
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

/*
Code that tests if the ray intersects the bounding box. Not used in current implementation, 
as it came straight from the GPUTracer project and I haven't tested it. But it should make
its way into the project, as this is one of the improvements I mentioned for the raycaster.
*/
bool intersectBBox(float4 rayStart, float4 rayDirection, float4 bboxMin, float4 bboxMax, float* tmin, float* tmax)
{
	float t0 = FLT_MIN;
	float t1 = FLT_MAX;
	
	float invRayDir = 1.0f / rayDirection.S0;
	float tNear = (bboxMin.S0 - rayStart.S0) * invRayDir;
	float  tFar = (bboxMax.S0 - rayStart.S0) * invRayDir;
	
	if (tNear > tFar)
	{
		invRayDir = tFar;
		tFar = tNear;
		tNear = invRayDir;
	}
	t0 = tNear > t0 ? tNear : t0;
	t1 = tFar < t1 ? tFar : t1;
	if (t0 > t1)
		return false;
				
	invRayDir = 1.0f / rayDirection.S1;
	tNear = (bboxMin.S1 - rayStart.S1) * invRayDir;
	tFar = (bboxMax.S1 - rayStart.S1) * invRayDir;
	
	if (tNear > tFar)
	{
		invRayDir = tFar;
		tFar = tNear;
		tNear = invRayDir;
	}
	t0 = tNear > t0 ? tNear : t0;
	t1 = tFar < t1 ? tFar : t1;
	if (t0 > t1)
		return false;

	invRayDir = 1.0f / rayDirection.S2;
	tNear = (bboxMin.S2 - rayStart.S2) * invRayDir;
	tFar = (bboxMax.S2 - rayStart.S2) * invRayDir;
	
	if (tNear > tFar)
	{
		invRayDir = tFar;
		tFar = tNear;
		tNear = invRayDir;
	}
	t0 = tNear > t0 ? tNear : t0;
	t1 = tFar < t1 ? tFar : t1;
	if (t0 > t1)
		return false;

	*tmin = t0;
	*tmax = t1;

	return true;
}

/*
Gets the value at coords x, y, z
*/
float getVolumeValue(float x, float y, float z, global float* volume, int blocksize)
{
	x *= blocksize;
	y *= blocksize;
	z *= blocksize;

	if(x >= blocksize || x < 0 ||
	   y >= blocksize || y < 0 ||
	   z >= blocksize || z < 0)
	{
	   return 0;
	}

	int ix = (int)x;
	int iy = (int)y;
	int iz = (int)z;

	if(ix == 0 && iy == 0 && iz == 0)
		return -2.0; //special value for the origin

	if((iy == 0 && iz == 0) || (iy == 0 && ix == 0) || (ix == 0 && iz == 0)
		|| (iy == blocksize && iz == blocksize) || (iy == blocksize && ix == blocksize) || (ix == blocksize && iz == blocksize))
		return -1.0; //special value for the axes

	return volume[((int)z) * blocksize * blocksize + ((int)y) * blocksize + ((int)x)];	
}

//Sets pixel in the 2D result texture
void setPixel(global uchar4* line, size_t x, uchar r, uchar g, uchar b)
{
	global uchar* pixel = (global uchar*)(line + x);

	pixel[0] = r;	//blue
	pixel[1] = g;	//green
	pixel[2] = b;	//green
	pixel[3] = 255;	//alpha
}

//Maps a volume value to a color value, the transfer function basically
float4 getColor(float4 pos, float4 norm, float4 lightPosition,global float* volume, int blocksize,float4 boxmin, float4 boxmax,short colormin, short colormax)
{
	float value = getVolumeValue(pos.s0,pos.s1,pos.s2,volume,blocksize);
	
	float4 color;
	if(value > colormin && value < colormax){
		color=(float4)(1,0.5,0.5,0);
	}else{
		color=(float4)(1,1,1,0);
	}

	return color;

/* Illumination (future improvement):
	float4 livec = fast_normalize(lightPosition -  pos);
	float illum = dot(livec,norm);
	bool haslight = false;

	if(illum > 0)
	{
		return illum * color;		
		haslight = true;
	}
	return (float4)(0.0f,0.0f,0.0f,0.0f);
*/
}

/*
Shoots a ray, samples colors at intervals
*/
float4 traceRay(float4 raySource, float4 rayDirection, global float* volume, int blocksize)
{
		
	float tmin = 2;
	float tmax = 9;
	
/* TODO: test this is working correctly, the bounding box intersection
	if(!intersectBBox(raySource, rayDirection, boxmin, boxmax, &tmin, &tmax))
	{
		return (float4)(0.0f,0.0f,0.0f,0.0f);
	}
*/
	
	float4 actualPoint;
	const float bigstep = 0.03;
	const float smallstep = 0.01;

	for(float t=tmin;t<tmax;t+=bigstep) //start with big intervals
	{
		actualPoint = raySource + t * rayDirection;		
		if(getVolumeValue(actualPoint.s0,actualPoint.s1,actualPoint.s2,volume,blocksize) != 0) //found something
		{
			float4 accumulatedColor;
			for(float u=t-bigstep;u<tmax;u+=smallstep) //retry with smaller intervals
			{																								  					
				actualPoint = raySource + u * rayDirection;
				float volumeValue = 0;
				if((volumeValue = getVolumeValue(actualPoint.s0,actualPoint.s1,actualPoint.s2,
						   volume,blocksize)) != 0)
				{
					if(volumeValue < -1)
					{
						accumulatedColor.x = 0.5;
					}
					else if(volumeValue == -1)
					{
						accumulatedColor.y = 0.5;
					}
					accumulatedColor.z += volumeValue;	
				}
			}
			return accumulatedColor;
		}		
	}
	return (float4)(0.0f,0.0f,0.0f,0.0f);
}

/*
Each pixel will have slightly varied ray directions
This is used to know the ray direction for a specific pixel on the screen plane
*/
float4 getRayDirection(int width, int height, int x, int y, float4 forward, float4 right, float4 up)
{
	float ratio = (float)width / height;
	float recenteredX = (x - (width/2)) / (2.0f * width) * ratio;
	float recenteredY = (y - (height/2)) / (2.0f * height) ;
	return fast_normalize(forward + (recenteredX * -right) + (recenteredY * up));
}
 
//Kernel entry point
kernel void RayCaster (int width, 
                       int height, 
			   global uchar* pOutput, int outputStride, float4 cameraPosition, 
			   float4 cameraForward, float4 cameraRight, float4 cameraUp, 
			   global float* volume, int blocksize)
{
	size_t x = get_global_id(0);
	size_t y = get_global_id(1);	

	//Construct and shoot the ray for one pixel
	global uchar4* pO = (global uchar4*)(pOutput+y*outputStride*4);
	float4 rayDirection = getRayDirection(width, height, x, y, cameraForward, cameraRight, cameraUp);
	float4 color = traceRay(cameraPosition, rayDirection, volume, blocksize);
	setPixel(pO, x, (int)(color.s0 > 1 ? 255 : color.s0 * 255), 
					(int)(color.s1 > 1 ? 255 : color.s1 * 255), 
					(int)(color.s2 > 1 ? 255 : color.s2 * 255));
}
