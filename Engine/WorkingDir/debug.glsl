///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef DEBUG_GBUFFER

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform int uTextureType; // 0: Albedo, 1: Normal, 2: Position, 3: Depth
uniform float uZnear;
uniform float uZfar;


layout(location = 0) out vec4 oColor;

// Function to linearize depth
float LinearizeDepth(float depth, float near, float far) 
{
    float z = depth * 2.0 - 1.0; // Convert to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main()
{
    vec4 texColor = texture(uTexture, vTexCoord);

    if (uTextureType == 3) 
    {
        float depth = texColor.r;

        // Linearize depth for better visualization
        float linearDepth = LinearizeDepth(depth, uZnear, uZfar);
        // Normalize to [0,1] range for display
        linearDepth = linearDepth / uZfar; // Scale by far plane
        
        oColor = vec4(vec3(1.0 - linearDepth), 1.0); // Invert so closer is brighter
        return;
    }
    oColor = texColor;
}

#endif
#endif