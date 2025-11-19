layout(location = 0) flat in uint inDrawID;
layout(location = 0) out uint outVisibility;

uint packVisibility(uint drawID, uint primID)
{
    const uint DRAW_ID_MASK = (1u << 20) - 1;
    const uint PRIM_ID_MASK = (1u << 12) - 1;
    return (drawID & DRAW_ID_MASK) | ((primID & PRIM_ID_MASK) << 20);
}

void main()
{
    outVisibility = packVisibility(inDrawID, uint(gl_PrimitiveID));
}
