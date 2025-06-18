struct CoarseVertex {
    float2 uv;
    float3 worldPos;
    float3 worldNormal;
    float3 camPos;
};

[[vk::binding(1)]] Sampler2D texSampler;



float4 main(CoarseVertex v) : SV_Target {
    float3 lightPos = float3(6.0, 10.0, -1.0);
    float3 lightColor = float3(1.0, 1.0, 1.0);
    float3 ambientColor = float3(0.1, 0.1, 0.1);

    

    float3 n = normalize(v.worldNormal);
    float3 L = normalize(lightPos - v.worldPos);
    float3 V = normalize(v.camPos - v.worldPos);
    float3 H = normalize(L + V);

    float diff = max(dot(n, L), 0.0);
    float spec = pow(max(dot(n, H), 0.0), 32.0);

    float3 albedo = texSampler.Sample(v.uv).xyz;

    float3 finalColor = ambientColor * albedo +
                        diff * albedo +
                        spec * lightColor;

    return float4(finalColor, 1.0);
}
