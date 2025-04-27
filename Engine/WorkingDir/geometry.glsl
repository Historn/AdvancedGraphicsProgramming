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