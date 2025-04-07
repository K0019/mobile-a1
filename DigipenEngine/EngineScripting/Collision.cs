using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EngineScripting
{
    public class Collision
    {
        public Vector2 normal;
        public Vector2 point;
        public float depth;

        public UInt64 eid;
        public UInt64 other;
    }

    public class Raycasting
    {
        public static void TestRaycast(Vector2 origin, Vector2 direction, out RaycastHit hit)
        {
            InternalCalls.Raycast(origin, direction, out hit);
        }
        public static void TestRaycast(Vector2 origin, Vector2 direction,int layerMask, out RaycastHit hit)
        {
            InternalCalls.Raycast(origin, direction, layerMask, out hit);
        }
    }
}
