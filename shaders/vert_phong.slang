struct AssembledVertex {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : UV;
};

struct CoarseVertex {
    float2 uv;
    float3 worldPos;
    float3 worldNormal;
    float3 camPos;
};

struct VertexStageOutput {
    CoarseVertex coarseVertex : CoarseVertex;
    float4 sv_position : SV_Position;
};

struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float3 cameraPos;
};

ParameterBlock<UniformBufferObject> gParams;

VertexStageOutput main(AssembledVertex v)
 {
    VertexStageOutput o;

    float4 worldPos = mul(gParams.model, float4(v.position, 1.0));
    float3 worldNormal = mul((float3x3)gParams.model, v.normal);

    o.coarseVertex.uv = v.uv;
    o.coarseVertex.worldPos = worldPos.xyz;
    o.coarseVertex.worldNormal = normalize(worldNormal);
    float4 viewPos = mul(gParams.view, worldPos);
    o.sv_position = mul(gParams.proj, viewPos);
    o.coarseVertex.camPos = gParams.cameraPos;

    return o;
}
