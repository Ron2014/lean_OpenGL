#version 330 core
struct DirectLight {
  vec3 direction;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

struct PointLight {
  vec3 position;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

  float constant;
  float linear;
  float quadratic;
};

struct SpotLight {
  vec3 position;
  vec3 direction;
  float cutoff;
  float cutoff_outter;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

  float constant;
  float linear;
  float quadratic;
};

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D emission;
    float shininess;
}; 

// normal 用来计算漫反射
// viewDir 用来计算高光
vec3 CalcDirectLight(DirectLight light, vec3 normal, vec3 viewDir);
// 拥有衰减属性的光, 需要片段坐标计算距离和角度
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir, vec3 FragPos);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 viewDir, vec3 FragPos);

out vec4 FragColor;

in VS_OUT {
    vec3 Normal;
    vec3 FragPos;
    vec2 TexCoord;
    vec4 FragPosLightSpace;
} fs_in;

uniform vec3 viewPos;
uniform Material material;      // == objColor
uniform sampler2D shadowMap;

uniform DirectLight directLight;
#define NR_POINT_LIGHTS 4
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;

void main() {
    // 属性
    vec3 norm = normalize(fs_in.Normal);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);

    // 第一阶段：定向光照
    vec3 result = CalcDirectLight(directLight, norm, viewDir);
    // 第二阶段：点光源
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, viewDir, fs_in.FragPos);
    // 第三阶段：聚光
    result += CalcSpotLight(spotLight, norm, viewDir, fs_in.FragPos);    

    FragColor = vec4(result, 1.0);
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    if (dot(normal, lightDir) <=0 ) return 1.0f;
    // 执行透视除法
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float currentDepth = projCoords.z;
    // if(currentDepth > 1.0) return 0.0f;
    // float bias = 0.0f;
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float shadow = ((currentDepth - bias) > closestDepth)  ? 1.0 : 0.0;
    // float shadow = 0.0;
    // vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    // for(int x = -1; x <= 1; ++x) {
    //     for(int y = -1; y <= 1; ++y) {
    //         float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
    //         shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
    //     }    
    // }
    // shadow /= 9.0;
    return shadow;
}

vec3 CalcDirectLight(DirectLight light, vec3 normal, vec3 viewDir) {
  vec2 texCoord = fs_in.TexCoord;
  vec3 ambient = light.ambient * vec3(texture(material.diffuse, texCoord));
  
  vec3 fromLight = light.direction;
  vec3 toLight = -fromLight;
  float diff = max(dot(toLight, normal), 0.0);
  vec3 diffuse = light.diffuse * (diff * vec3(texture(material.diffuse, texCoord)));

  vec3 halfwayDir = normalize(viewDir + toLight);
  float spec = pow(max(dot(normal, halfwayDir), 0.0f), material.shininess);
  // vec3 reflectDir = reflect(fromLight, normal);
  // float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  vec3 specular = light.specular * (spec * vec3(texture(material.specular, texCoord)));

  float shadow = ShadowCalculation(fs_in.FragPosLightSpace, normal, toLight);
  return ambient + (diffuse + specular) * (1.0f - shadow);
  // return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir, vec3 fragPos) {
  vec2 texCoord = fs_in.TexCoord;
  float distance = length(light.position - fragPos);
  float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

  vec3 ambient = light.ambient * vec3(texture(material.diffuse, texCoord));
  
  vec3 fromLight = normalize(fragPos-light.position);
  vec3 toLight = -fromLight;
  float diff = max(dot(toLight, normal), 0.0);
  vec3 diffuse = light.diffuse * (diff * vec3(texture(material.diffuse, texCoord)));

  vec3 reflectDir = reflect(fromLight, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  vec3 specular = light.specular * (spec * vec3(texture(material.specular, texCoord)));

  return (ambient + diffuse + specular) * attenuation;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 viewDir, vec3 fragPos) {
  vec2 texCoord = fs_in.TexCoord;
  float distance = length(light.position - fragPos);
  float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

  vec3 fromLight = normalize(fragPos - light.position);
  vec3 toLight = -fromLight;

  vec3 ambient = light.ambient * vec3(texture(material.diffuse, texCoord));
  
  float intensity = clamp((dot(fromLight, light.direction)-light.cutoff_outter) / (light.cutoff-light.cutoff_outter), 0.0f, 1.0f);

  float diff = max(dot(toLight, normal), 0.0f);
  vec3 diffuse = light.diffuse * (diff * vec3(texture(material.diffuse, texCoord)));
  diffuse *= intensity;

  vec3 reflectDir = reflect(fromLight, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
  vec3 specular = light.specular * (spec * vec3(texture(material.specular, texCoord)));
  specular *= intensity;

  return (ambient + diffuse + specular) * attenuation;
}
