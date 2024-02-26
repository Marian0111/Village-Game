#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec3 fPosition;

in vec4 fragPosLightSpace;

out vec4 fColor;

//lighting
uniform	vec3 lightDir;
uniform	vec3 lightColor;

//texture
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

//shadow map
uniform sampler2D shadowMap;

//skybox
uniform samplerCube skybox;

vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;

float shadow;

void computeLightComponents()
{		
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	vec3 normalEye = normalize(fNormal);	
	
	vec3 lightDirN = normalize(lightDir);
	
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
		
	ambient = ambientStrength * lightColor;
	
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
	specular = specularStrength * specCoeff * lightColor;
}


float computeShadow()
{

    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    if (normalizedCoords.z > 1.0f)
    {
        return 0.0f;
    }

    normalizedCoords = normalizedCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;

    float currentDepth = normalizedCoords.z;

    float bias = 0.001f;

    float shadow = 0.0f;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (float x = -1.0f; x <= 1.0f; x += 0.25f) {
        for (float y = -1.0f; y <= 1.0f; y += 0.25f) {
            float pcfDepth = texture(shadowMap, normalizedCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0f : 0.0f;
        }
    }
    shadow /= 81.0f;
    return shadow;
}

float computeFog() 
{
	 float fogDensity = 0.015f;
	 float fragmentDistance = length(fPosEye);
	 float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
	 return clamp(fogFactor, 0.0f, 1.0f);
}

void computePointLight(vec3 lightPosition, vec3 lightCol, float constant, float linear, float quadratic) {
    float dist = length(lightPosition - fPosition);
    float att = 1.0f / (constant + linear * dist + quadratic * (dist * dist));

    diffuse += att * diffuse * lightCol;
    ambient += att * ambient * lightCol;
    specular += att * specular * lightCol;
}

void main() 
{
	computeLightComponents();
	
	float fogFactor = computeFog();
	vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);

	vec4 colorFromTexture = texture(diffuseTexture, fTexCoords) * fogFactor; 
	if(colorFromTexture.a < 0.1) 
		discard; 
	
	ambient *= colorFromTexture.rgb;
	diffuse *= colorFromTexture.rgb;
	specular *= texture(specularTexture, fTexCoords).rgb;

	computePointLight(vec3(-25.9958f, 7.07429f, -47.3042f), vec3(0.7f, 0.7f, 0.7f), 0.9f, 0.0045f, 0.0075f);
	computePointLight(vec3(-20.709f, 7.07429f, -43.0924f), vec3(0.7f, 0.7f, 0.7f), 0.9f, 0.0045f, 0.0075f);
	computePointLight(vec3(-14.1701f, 7.07429f, -47.2926f), vec3(0.7f, 0.7f, 0.7f), 0.9f, 0.0045f, 0.0075f);
	computePointLight(vec3(-6.9415f, 7.07429f, -42.7389f), vec3(0.7f, 0.7f, 0.7f), 0.9f, 0.0045f, 0.0075f);
	computePointLight(vec3(-32.4952f, 7.07429f, -42.5996f), vec3(0.7f, 0.7f, 0.7f), 0.9f, 0.0045f, 0.0075f);
	computePointLight(vec3(-38.1291f, 7.07429f, -47.3362f), vec3(0.7f, 0.7f, 0.7f), 0.9f, 0.0045f, 0.0075f);
	computePointLight(vec3(-43.997516f, 7.07429f, -42.7697f), vec3(0.7f, 0.7f, 0.7f), 0.9f, 0.0045f, 0.0075f);

	shadow = computeShadow();
	
    vec3 color = min((ambient + (1.0f - shadow) * diffuse) + (1.0f - shadow) * specular, 1.0f);
   
	fColor = vec4(color, 1.0f);

	fColor = mix(fogColor, fColor, fogFactor);
}
