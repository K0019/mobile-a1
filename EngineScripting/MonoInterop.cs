using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

public static class MonoInterop
{
    // P/Invoke to get Mono class from MonoObject*
    [DllImport("mono-2.0-sgen.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr mono_object_get_class(IntPtr obj);

    // P/Invoke to get number of fields in a class
    [DllImport("mono-2.0-sgen.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int mono_class_num_fields(IntPtr klass);

    // P/Invoke to get a field at a given index
    [DllImport("mono-2.0-sgen.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr mono_class_get_field(IntPtr klass, int index);

    [DllImport("mono-2.0-sgen.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr mono_object_unbox(IntPtr obj);

    // P/Invoke to get the field name
    [DllImport("mono-2.0-sgen.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr mono_field_get_name(IntPtr field);

    // P/Invoke to get the value of a field
    [DllImport("mono-2.0-sgen.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr mono_field_get_value(IntPtr obj, IntPtr field);

    // Convert IntPtr to C# string
    public static string PtrToString(IntPtr ptr)
    {
        return Marshal.PtrToStringAnsi(ptr);
    }

    public static IntPtr GetMonoClass(IntPtr monoObjectPtr)
    {
        return mono_object_get_class(monoObjectPtr);
    }

    public static int GetFieldCount(IntPtr monoClassPtr)
    {
        return mono_class_num_fields(monoClassPtr);
    }

    public static IntPtr GetFieldAtIndex(IntPtr monoClassPtr, int index)
    {
        return mono_class_get_field(monoClassPtr, index);
    }

    public static string GetFieldName(IntPtr fieldPtr)
    {
        return PtrToString(mono_field_get_name(fieldPtr));
    }

    public static IntPtr GetFieldValue(IntPtr monoObjectPtr, IntPtr fieldPtr)
    {
        return mono_field_get_value(monoObjectPtr, fieldPtr);
    }
}