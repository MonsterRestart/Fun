////////////////////////////////////////////////////////////////////////////////
// Filename: colorVS.fx
////////////////////////////////////////////////////////////////////////////////


/////////////
// GLOBALS //
/////////////
cbuffer cb0
{
    row_major float4x4 worldMatrix      : packoffset(c0);     // World * View * Projection transformation
    row_major float4x4 viewMatrix       : packoffset(c4);     // World * View * Projection transformation
    row_major float4x4 projectionMatrix : packoffset(c8);     // World * View * Projection transformation
};
//matrix worldMatrix;
//matrix viewMatrix;
//matrix projectionMatrix;


//////////////
// TYPEDEFS //
//////////////
struct VertexInputType
{
    float4 position : POSITION;
    float4 color : COLOR;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};


////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
PixelInputType ColorVertexShader(VertexInputType input)
{
    PixelInputType output;
    
    
    // Change the position vector to be 4 units for proper matrix calculations.
    input.position.w = 1.0f;

    // Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    // Store the input color for the pixel shader to use.
    output.color = input.color;
    
    return output;
}
