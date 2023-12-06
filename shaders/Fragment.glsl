#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform vec3 ViewPos;//

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float     shininess;
};

struct DirLight{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
vec4 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);  

struct PointLight{
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
vec4 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);


struct SpotLight {
    vec3 position;
    vec3 direction;

    float constant;
    float linear;
    float quadratic;

    float cutOff;
    float outerCutOff;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

vec4 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

uniform Material material;
uniform DirLight dirLight; 
#define NR_POINT_LIGHTS 4  
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;

float near = 0.1f;
float far = 100;

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}
//float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far for demonstration
//FragColor = vec4(vec3(depth), 1.0);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(ViewPos - FragPos);

    //Directional Lighting
    vec4 result = CalcDirLight(dirLight, norm, viewDir);
    //Point Lighting
    for (int i = 0; i < NR_POINT_LIGHTS; i++){
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    }
    // Spotlight Lighting
    //result += CalcSpotLight(spotLight, norm, FragPos, viewDir);
    if (result.a < 0.5f) discard;

    FragColor = result;
    
}

vec4 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // combine results
    vec4 ambient  = vec4(light.ambient, 1.0f)  * texture(material.diffuse, TexCoords);
    vec4 diffuse  = vec4(light.diffuse, 1.0f)  * diff * texture(material.diffuse, TexCoords);
    vec4 specular = vec4(light.specular, 1.0f) * spec * texture(material.specular, TexCoords);
    return (ambient + diffuse + specular);
}  

vec4 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			                    light.quadratic * (distance * distance));    
    // combine results
    vec4 ambient  = vec4(light.ambient, 1.0f)  * texture(material.diffuse, TexCoords);
    vec4 diffuse  = vec4(light.diffuse, 1.0f)  * diff * texture(material.diffuse, TexCoords);
    vec4 specular = vec4(light.specular, 1.0f) * spec * texture(material.specular, TexCoords);
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
} 

vec4 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    //diffuse
    float diff = max(dot(normal, lightDir), 0.0f);

    //specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec      = pow(max(dot(viewDir, reflectDir),0.0f), material.shininess);

    //attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			            light.quadratic * (distance * distance));    

    //cutoff
    float theta     = dot(lightDir, normalize(-light.direction)); // cos of angle between light direction and direction to light
    float epsilon   = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0); 

    // combine results
    vec4 ambient  = vec4(light.ambient, 1.0f)  * texture(material.diffuse, TexCoords);
    vec4 diffuse  = vec4(light.diffuse, 1.0f)  * diff * texture(material.diffuse, TexCoords);
    vec4 specular = vec4(light.specular, 1.0f) * spec * texture(material.specular, TexCoords);

    ambient  *= attenuation;
    diffuse  *= attenuation * intensity;
    specular *= attenuation * intensity;

    return (ambient + diffuse + specular);
}