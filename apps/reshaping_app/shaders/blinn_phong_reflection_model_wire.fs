#version 410

in vec3 GPosition;
in vec3 GNormal;
//flat in vec3 GNormal;

// Forcing linear-interpolation
noperspective in vec3 GEdgeDistance;

layout(location = 0) out vec4 FragColor;

uniform struct LightInfo {
    vec4 Position;
    vec3 La;       // Ambient light intensity
    vec3 Ld;       // Diffuse light intensity
    vec3 Ls;       // Specular light intensity
} Light;

uniform struct MaterialInfo {
    vec3 Ka;    // Ambient reflectivity
    vec3 Kd;    // Diffuse reflectivity
    vec3 Ks;    // Specular reflectivity
    float Shininess; // Specular shininess factor
} Material;


uniform struct WireInfo {
    float EdgeWidth;
    float BlendWidth;
    vec4 Color;
    bool Enabled;
} Wire;

uniform struct SurfaceInfo {
    float Width;
    vec4 Color;
    bool Enabled;
} Surface;

// Position: given in the view coordinate system
vec3 blinnPhongModel(vec3 position, vec3 n)
{
    // Ambient component
    vec3 ambient = Light.La * Material.Ka;

    // Check if it needs to be treated as directional light
    vec3 s;
    if(Light.Position.w == 0.0)
        s = normalize(Light.Position.xyz);
    else
        s = normalize(Light.Position.xyz - position);

    // Diffuse component
    float sDotN = max(dot(s,n), 0.0);
    vec3 diffuse = Light.Ld * Material.Kd * sDotN;

    // Specular component
    vec3 spec;
    if(sDotN > 0.0) {
        vec3 v = normalize(-position.xyz);
        vec3 h = normalize(s + v);
        spec = Light.Ls *
               Material.Ks *
               pow(max(dot(h,n), 0.0), Material.Shininess);
    }

    return ambient + diffuse + spec;
}

vec4 double_sided_shading(vec3 position, vec3 normal) {
    vec3 n = normalize(normal);
    vec3 v = normalize(-position.xyz);

    float vDotN = dot(v, n);
    if(vDotN >= 0.0)
        return vec4(blinnPhongModel(position, n), 1.0);
    else
        return vec4(blinnPhongModel(position, -n), 1.0);
}

vec4 wired_double_sided_shading(vec3 position, vec3 normal, out bool to_discard) {
    // Shaded surface color
    vec4 surface_color = double_sided_shading(GPosition, GNormal);

    // Find the smallest distance to an edge
    float d = min(GEdgeDistance.x, GEdgeDistance.y);
    d = min(d, GEdgeDistance.z);

    // Determine the mix factor with the line color
    float mixVal = smoothstep(Wire.EdgeWidth - Wire.BlendWidth,
                              Wire.EdgeWidth + Wire.BlendWidth, d);

    to_discard = mixVal >= 1.0;
    
    // Mix the surface color with the line color
    return mix(Wire.Color, surface_color, mixVal);
}

void main() {
    bool to_discard = false;

    if(Wire.Enabled)
        FragColor = wired_double_sided_shading(GPosition, GNormal, to_discard);
    else
        FragColor = double_sided_shading(GPosition, GNormal);

    if(to_discard && !Surface.Enabled)
        discard;
}