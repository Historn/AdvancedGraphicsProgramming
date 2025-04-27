///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef SHOW_TEXTURED_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;

void main()
{
	vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vPosition = worldPosition.xyz;

	vTexCoord = aTexCoord;

	vNormal = mat3(transpose(inverse(uModel))) * aNormal; // For correct normal transformation

    // Set final position in clip space
    gl_Position = uProjection * uView * worldPosition;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main()
{
	oColor = texture(uTexture, vTexCoord);
}

#endif
#endif


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef GEOMETRY_PASS

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;

void main()
{
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vPosition = worldPosition.xyz;
    
    // For correct normal transformation
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    
    vTexCoord = aTexCoord;
    
    gl_Position = uProjection * uView * worldPosition;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;

uniform sampler2D uTexture;

// G-buffer outputs
layout(location = 0) out vec4 gAlbedo;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec3 gPosition;

void main()
{
    // Store albedo color (RGB) from texture
    gAlbedo = texture(uTexture, vTexCoord);
    
    // Store normalized normals
    gNormal = normalize(vNormal);
    
    // Store fragment position in world space
    gPosition = vPosition;
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef DEFERRED_LIGHTING

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uAlbedoTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uPositionTexture;
uniform sampler2D uDepthTexture;

// Light properties
#define MAX_LIGHTS 10
uniform int uNumDirLights;
uniform int uNumPointLights;
uniform vec3 uViewPos;

struct DirectionalLight 
{
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight 
{
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
};

uniform DirectionalLight uDirLights[MAX_LIGHTS];
uniform PointLight uPointLights[MAX_LIGHTS];

layout(location = 0) out vec4 oColor;

vec3 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 albedo) 
{
    // Diffuse light calculation
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light.color * light.intensity * albedo;
    
    // Specular light calculation
    float specularStrength = 0.5;
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * light.color * light.intensity;
    
    return diffuse + specular;
}

vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo) 
{
    // Calculate distance and attenuation
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    
    // Diffuse light calculation
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light.color * light.intensity * albedo;
    
    // Specular light calculation
    float specularStrength = 0.5;
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * light.color * light.intensity;
    
    // Apply attenuation
    diffuse *= attenuation;
    specular *= attenuation;
    
    // Apply light radius falloff
    float distanceFactor = 1.0 - smoothstep(light.radius * 0.5, light.radius, distance);
    
    return (diffuse + specular) * distanceFactor;
}

void main()
{
    // Retrieve data from G-buffer
    vec3 albedo = texture(uAlbedoTexture, vTexCoord).rgb;
    vec3 normal = texture(uNormalTexture, vTexCoord).rgb;
    vec3 fragPos = texture(uPositionTexture, vTexCoord).rgb;
    float depth = texture(uDepthTexture, vTexCoord).r;
    
    // Early exit if depth is 1.0 (skybox or background)
    if (depth == 1.0) 
    {
        oColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // Calculate view direction
    vec3 viewDir = normalize(uViewPos - fragPos);
    
    // Ambient light component
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * albedo;
    
    // Initialize lighting
    vec3 lighting = ambient;
    
    // Process directional lights
    for (int i = 0; i < uNumDirLights; i++) 
    {
        lighting += CalculateDirectionalLight(uDirLights[i], normal, viewDir, albedo);
    }
    
    // Process point lights
    for (int i = 0; i < uNumPointLights; i++) 
    {
        lighting += CalculatePointLight(uPointLights[i], normal, fragPos, viewDir, albedo);
    }
    
    // Final color
    oColor = vec4(lighting, 1.0);
}

#endif
#endif