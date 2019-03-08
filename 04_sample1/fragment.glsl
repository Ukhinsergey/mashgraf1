#version 330
#define float2 vec2
#define float3 vec3
#define float4 vec4
#define float4x4 mat4
#define float3x3 mat3

const int MAX_MARCHING_STEPS = 500;
const float MIN_DIST = 0.0;
const float MAX_DIST = 100.0;
const float EPSILON = 0.00001;



in float2 fragmentTexCoord;

layout(location = 0) out vec4 fragColor;

uniform int g_screenWidth;
uniform int g_screenHeight;

uniform vec3 ray_pos;
uniform vec3 ray_dir;
//uniform float time;

uniform float3 g_bBoxMin   = float3(-1,-1,-1);
uniform float3 g_bBoxMax   = float3(+1,+1,+1);

uniform float4x4 g_rayMatrix;

uniform float4   g_bgColor = float4(1,1,1,1);
uniform samplerCube box;



float3 EyeRayDir(float x, float y, float w, float h)
{
	float fov = 3.141592654f/(3.0f); 
  float3 ray_dir;
  
	ray_dir.x = x - (w/2.0f);	
	ray_dir.y = y - (h/2.0f);
	ray_dir.z = -(w)/tan(fov/2.0f);
	
  return normalize(ray_dir);
}

float sphereSDF1(vec3 samplePoint) {
    vec3 pos = vec3(-2.5, 1.0, 1.0);
    float radius = 1.0;
    return length(samplePoint - pos) - radius;
}

float sphereSDF2(vec3 samplePoint) {
    vec3 pos = vec3(2.5, 1.0, 1.0);
    float radius = 1.0;
    return length(samplePoint - pos) - radius;
}

float planeSDF(vec3 p) {
    vec3 b = vec3(4, 0.02, 6);
    vec3 pos = vec3(0,0,1);
    return length(max(abs(p - pos)-b,0.0));
}

float mirrorCubeSDF(vec3 p) {
    vec3 b = vec3(4, 2, 0.02);
    vec3 pos = vec3(0,2,8);
    return length(max(abs(p - pos)-b,0.0));
}



float sdTorus(vec3 p)
{
    vec3 pos = vec3(0,4,1);
    p = p - pos;
    vec2 t = vec2(0.875,0.2);
    vec2 q = vec2(length(p.xz)-t.x,p.y);
    return length(q)-t.y;
}

float sdCapsule(vec3 p)
{
    vec3 pos = vec3(2,0,4);
    p = p - pos;
    vec3 a = vec3(1.75,1,1);
    vec3 b = vec3(1.75,1,1);
    float r = 0.5;
    vec3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h ) - r;
}



float sceneSDF(vec3 p) {
    float d = MAX_DIST;
    d = min(d, sphereSDF1(p));
    d = min(d, sphereSDF2(p));
    d = min(d, planeSDF(p));
    d = min(d, mirrorCubeSDF(p));
    d = min(d, sdTorus(p));
    return d;
}

vec3 getNormal(vec3 p) {
    return normalize(vec3(
        sceneSDF(vec3(p.x + EPSILON, p.y, p.z)) - sceneSDF(vec3(p.x - EPSILON, p.y, p.z)),
        sceneSDF(vec3(p.x, p.y + EPSILON, p.z)) - sceneSDF(vec3(p.x, p.y - EPSILON, p.z)),
        sceneSDF(vec3(p.x, p.y, p.z  + EPSILON)) - sceneSDF(vec3(p.x, p.y, p.z - EPSILON))
    ));
}


float RayMarch(vec3 eye, vec3 marchingDirection, float start, float end) {
    float depth = start;
    for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
        float dist = sceneSDF(eye + depth * marchingDirection);
        if (dist < EPSILON) {
					return depth;
        }
        depth += dist;
        if (depth >= end) {
            return end;
        }
    }
    return end;
}

vec3 phongContrib(vec3 diffuse, vec3 specular,float shininess, vec3 p, vec3 ray_pos, vec3 lightpos, vec3 lightIntens) {
  vec3 normal = getNormal(p);
  vec3 lightvec = normalize(lightpos - p);
  vec3 v = normalize(ray_pos - p);
  vec3 reflectvec = normalize(reflect(-lightvec, normal));
  float dotLN = dot(lightvec, normal);
  float dotRV = dot(reflectvec, v);
  if (dotLN < 0 ) {
    return vec3(0,0,0);
  }
  if (dotRV < 0) {
    return lightIntens * (diffuse * dotLN);
  }
  return lightIntens * (diffuse * dotLN + specular * pow(dotRV, shininess));

}


vec3 phongIllumination(vec3 ambient, vec3 diffuse, vec3 specular,float shininess, vec3 p, vec3 ray_pos) {
  const vec3 ambientLight = 0.5 * vec3(1.0, 1.0, 1.0);
  vec3 color = ambientLight * ambient;
  vec3 light1Pos = vec3(-4,9,1);
  vec3 light1Intens = vec3(0.4, 0.4, 0.4);
  vec3 light2Pos = vec3(3.0, 7, -1.0);
  vec3 light2Intens = vec3(0.4, 0.4, 0.4);
  float d1 = RayMarch(p + 2 * EPSILON * getNormal(p), normalize(light1Pos - p), MIN_DIST, MAX_DIST);
  float d2 = RayMarch(p + 2 * EPSILON * getNormal(p), normalize(light2Pos - p), MIN_DIST, MAX_DIST);
  if (d1 >= length(light1Pos - p) - EPSILON) {
    color +=  phongContrib(diffuse, specular, shininess, p, ray_pos, light1Pos, light1Intens);   
  }
  if (d2 >= length(light2Pos - p) - EPSILON) {
    color +=  phongContrib(diffuse, specular, shininess, p, ray_pos, light2Pos, light1Intens);   
  } 
  return color;   

}


void main(void)
{	
  float w = float(g_screenWidth);
  float h = float(g_screenHeight);
  
  // get curr pixelcoordinates
  //
  float x = fragmentTexCoord.x*w; 
  float y = fragmentTexCoord.y*h;
  
  // generate initial ray
  //
  float3 ray_pos = float3(0,0,0); 
  float3 ray_dir = EyeRayDir(x,y,w,h);
 
  // transorm ray with matrix
  //
  ray_pos = (g_rayMatrix*float4(ray_pos,1)).xyz;
  ray_dir = float3x3(g_rayMatrix)*ray_dir;
  float dist = RayMarch(ray_pos, ray_dir, MIN_DIST, MAX_DIST);
  if (dist > MAX_DIST - EPSILON) {
        // Didn't hit anything
    //fragColor = vec4(0.0,0.0, 0.0, 0.0);
    fragColor = texture(box, -ray_dir);
		return;
  }
  vec3 K_a, K_d, K_s;
  vec3 point = ray_pos + ray_dir * dist;
  float shininess;
  if (mirrorCubeSDF(point) <=  2 * EPSILON){
    K_a = vec3(0., 0., 0.);
    K_d = vec3(0., 0., 0.);
    K_s = vec3(0., 0., 0.);
    shininess = 76.8;
    vec3 normal = getNormal(point);
    vec3 reflectvec = normalize(reflect(ray_dir, normal));
    float d = RayMarch(point , reflectvec, MIN_DIST + 2 * EPSILON, MAX_DIST);
    point += d * reflectvec;
    if (d > MAX_DIST - EPSILON) {
        // Didn't hit anything
      //fragColor = vec4(0.0,0.0, 0.0, 0.0);
      fragColor = texture(box, -reflectvec);
      return;
    }
  } 
  if ( length(sdTorus(point))  <= 2 * EPSILON) {
    K_a = vec3(0.0215, 0.1745, 0.0215);
    K_d = vec3(0.07568, 0.61424, 0.07568);
    K_s = vec3(0.633, 0.727811, 0.633);
    shininess = 76.8;
  } else if ( length(planeSDF(point)) <= 2 * EPSILON) {
    K_a = vec3(0.1745, 0.01175, 0.01175);
    K_d = vec3(0.61424, 0.04136, 0.04136);
    K_s = vec3(0.727811, 0.626959, 0.626959);
    shininess = 76.8;
  } else if (length(sphereSDF1(point)) <= 2 * EPSILON){
    K_a = vec3(0.05375, 0.05, 0.06625);
    K_d = vec3(0.18275, 0.17, 0.22525 );
    K_s = vec3(0.332741, 0.328634, 0.346435 );
    shininess = 38.4;
  } else if (length(sphereSDF2(point)) <= 2 * EPSILON){
    K_a = vec3(0.05375, 0.05, 0.06625);
    K_d = vec3(0.18275, 0.17, 0.22525 );
    K_s = vec3(0.332741, 0.328634, 0.346435 );
    shininess = 38.4;
  } else {
    K_a = vec3(0.05375, 0.05, 0.06625);
    K_d = vec3(0.18275, 0.17, 0.22525 );
    K_s = vec3(0.332741, 0.328634, 0.346435 );
    shininess = 38.4;
  }
  


  vec3 col = phongIllumination(K_a, K_d, K_s, shininess, point, point);;
  fragColor = vec4(col, 1.0);
  
}


