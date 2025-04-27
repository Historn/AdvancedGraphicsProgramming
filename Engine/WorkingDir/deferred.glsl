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
    vec3 albedo = texture(uAlbedoTexture, vTexCoord).rgb;
    vec3 normal = texture(uNormalTexture, vTexCoord).rgb;
    vec3 fragPos = texture(uPositionTexture, vTexCoord).rgb;
    float depth = texture(uDepthTexture, vTexCoord).r;
    
    if (depth > 0.999) 
    {
        oColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    vec3 viewDir = normalize(uViewPos - fragPos);
    
    // Ambient light component
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * albedo;
    
    vec3 lighting = ambient;
    
    for (int i = 0; i < uNumDirLights; i++) 
    {
        lighting += CalculateDirectionalLight(uDirLights[i], normal, viewDir, albedo);
    }
    
    for (int i = 0; i < uNumPointLights; i++) 
    {
        lighting += CalculatePointLight(uPointLights[i], normal, fragPos, viewDir, albedo);
    }
    
    oColor = vec4(lighting, 1.0);
}

#endif
#endif