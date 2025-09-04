using GlmSharp;
using System;
using System.Collections.Generic;
using System.Data.SqlTypes;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace EngineScripting
{
    /*****************************************************************//*!
    \brief
        C# counterpart to the c++ TextComponent
    \return
        None
    *//******************************************************************/
    [StructLayout(LayoutKind.Sequential)]
    public struct Text : IComponent
    {
        private EntityHandle eid;
        private vec4 c;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        private string str;


        [DllImport("__Internal", EntryPoint = "CS_GetComp_Text")]
        public static extern Text RetrieveComp(EntityHandle entity);
        [DllImport("__Internal", EntryPoint = "CS_Text_SetText")]
        private static extern void SetText(EntityHandle entity, string str);
        [DllImport("__Internal", EntryPoint = "CS_Text_SetColor")]
        private static extern void SetColor(EntityHandle entity, vec4 str);

        public vec4 color
        {
            get => c;
            set
            {
                c = value;
                SetColor(eid, value);
            }
        }

        public string text
        {
            get => str;
            set
            {
                str = value;
                SetText(eid, value);
            }
        }

    }
}
